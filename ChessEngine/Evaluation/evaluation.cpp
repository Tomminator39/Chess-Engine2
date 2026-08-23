#include "evaluation.h"

struct MgEgScore { int mg; int eg; };

struct PieceAttacks {
    uint64_t attacks[10];   // one bitboard per piece KNIGHT/BISHOP/ROOK/QUEEN 
    int square[10];         // matching square for each entry
    PieceType type[10];
    int count;             // how many entries are actually filled
};

void InitEvaluation() {
    const int* mgPsts[6] = {
        mgPawnTable, mgKnightTable, mgBishopTable,
        mgRookTable, mgQueenTable,  mgKingTable
    };

    const int* egPsts[6] = {
        egPawnTable, egKnightTable, egBishopTable,
        egRookTable, egQueenTable,  egKingTable
    };

    for (int piece = 0; piece < 6; piece++) {
        for (int square = 0; square < 64; square++) {
            int whiteSq = square;
            int blackSq = square ^ 56; // Bitwise XOR flips rank for Black

            // Combine Material Value + PST Value directly into final lookup tables
            mgPst[piece][WHITE][whiteSq] = pieceValues[piece] + mgPsts[piece][square];
            mgPst[piece][BLACK][blackSq] = pieceValues[piece] + mgPsts[piece][square];

            egPst[piece][WHITE][whiteSq] = pieceValues[piece] + egPsts[piece][square];
            egPst[piece][BLACK][blackSq] = pieceValues[piece] + egPsts[piece][square];
        }
    }
}

PieceAttacks computePieceAttacks(const Board& board, Color color){
    PieceAttacks result{};
    result.count = 0;

    for(PieceType piece : {KNIGHT, BISHOP, ROOK, QUEEN}){
        uint64_t pieces = board.pieceBB[color][piece];
        while(pieces){
            int square = __builtin_ctzll(pieces);
            result.attacks[result.count] = getMobilityBitboard(board, square, piece, color);
            result.square[result.count] = square;
            result.type[result.count] = piece;
            result.count++;
            pieces &= pieces - 1;
        }
    }
    return result;
}

MgEgScore calculateBishopPair(const Board& board, Color color){
    if(__builtin_popcountll(board.pieceBB[color][BISHOP]) >= 2){
        return { BISHOP_PAIR_MG, BISHOP_PAIR_EG};
    }
    return {0, 0};
}

MgEgScore CalculateMobility(const Board& board, Color color, const PieceAttacks& myAttacks){
    MgEgScore score{0, 0};
    Color enemy = (color == WHITE) ? BLACK : WHITE;
    uint64_t enemyPawnAttacks = getPawnAttackBitboard(board, enemy);

    for(int i = 0; i < myAttacks.count; i++){
        uint64_t mobility = myAttacks.attacks[i] & ~enemyPawnAttacks;
        int count = std::min(__builtin_popcountll(mobility), 27);
        score.mg += mg_Mobility[myAttacks.type[i]][count];
        score.eg += eg_Mobility[myAttacks.type[i]][count];
    }
    return score;
}

int calculateKingPawnShieldScore(const Board& board, Color color){
    int kingSquare = __builtin_ctzll(board.pieceBB[color][KING]);
    int kingFile = kingSquare & 7;
    int kingRank = kingSquare / 8;

    bool onBackRank = (color == WHITE) ? (kingRank == 0) : (kingRank == 7);
    bool committedToSide = (kingFile <= 2 || kingFile >= 5);  // roughly queenside or kingside castled territory
    if(!onBackRank || !committedToSide) return 0;

    int kingSafetyScore = 0;
    for (int i = -1; i <= 1; i++){
        int file = kingFile + i;
        if(file < 0 || file > 7) continue;
        uint64_t pawnsOnFile = fileMasks[file] & board.pieceBB[color][PAWN];
        if(pawnsOnFile == 0){
            kingSafetyScore -= 40;
        }
        else{
            int pawnSquare = (color == WHITE) ? __builtin_ctzll(pawnsOnFile) : (63 - __builtin_clzll(pawnsOnFile));
            int pawnRank = pawnSquare / 8;
            int rankDistanceFromKing = std::abs(pawnRank - kingRank);
            kingSafetyScore += pawnShieldScores[rankDistanceFromKing];
        }
    }

    return kingSafetyScore;
}

int calculateKingZoneAttackScore(const Board& board, Color kingColor, const PieceAttacks& enemyAttacks){
    int kingSquare = __builtin_ctzll(board.pieceBB[kingColor][KING]);
    uint64_t kingZone = kingAttacks[kingSquare] | (1ULL << kingSquare);

    int totalWeight = 0;
    int attackerCount = 0;

    for(int i = 0; i < enemyAttacks.count; i++){
        uint64_t attacksInZone = enemyAttacks.attacks[i] & kingZone;
        if(attacksInZone != 0){
            attackerCount++;
            totalWeight += attackWeight[enemyAttacks.type[i]] * __builtin_popcountll(attacksInZone);
        }
    }

    if(attackerCount < 2 || totalWeight < 80) return 0;

    int weightBucket = std::min(totalWeight / 40, 7);
    int attackerBucket = std::min(attackerCount, 7);
    return -kingDangerTable[attackerBucket][weightBucket];
}

int Evaluate(const Board& board) {
    PieceAttacks whiteAttacks = computePieceAttacks(board, WHITE);
    PieceAttacks blackAttacks = computePieceAttacks(board, BLACK);

    MgEgScore whiteMobility = CalculateMobility(board, WHITE, whiteAttacks);
    MgEgScore blackMobility = CalculateMobility(board, BLACK, blackAttacks);

    int whiteKingDanger = calculateKingZoneAttackScore(board, WHITE, blackAttacks); // note: blackAttacks, since it's Blacks pieces attacking Whites king zone
    int blackKingDanger = calculateKingZoneAttackScore(board, BLACK, whiteAttacks);

    MgEgScore whiteBishopPair = calculateBishopPair(board, WHITE);
    MgEgScore blackBishopPair = calculateBishopPair(board, BLACK);

    int midgameEval = board.midgameScore[WHITE] - board.midgameScore[BLACK]
    + (whiteMobility.mg - blackMobility.mg) + (whiteBishopPair.mg - blackBishopPair.mg)
    + (calculateKingPawnShieldScore(board, WHITE) - calculateKingPawnShieldScore(board, BLACK))
    + (whiteKingDanger - blackKingDanger);

    int endgameEval = board.endgameScore[WHITE] - board.endgameScore[BLACK] 
        + (whiteMobility.eg - blackMobility.eg) + (whiteBishopPair.eg - blackBishopPair.eg);

    // Tapered Eval
    int finalPhase = std::min(256, (board.gamePhase * 256 + 12) / 24);

    int evaluation = ((midgameEval * finalPhase) + (endgameEval * (256 - finalPhase))) / 256;

    return (board.turn == WHITE) ? evaluation : -evaluation;
}