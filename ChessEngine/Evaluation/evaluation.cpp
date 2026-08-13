#include "evaluation.h"

constexpr int MOPUP_PHASE_THRESHOLD = 8;
constexpr int MOPUP_EDGE_WEIGHT = 10;
constexpr int MOPUP_DISTANCE_WEIGHT = 5;

struct MgEgScore { int mg; int eg; };

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

int CountMaterial(const Board& board, Color color){
    int material = 0;
    for(int piece = 0; piece < 6; piece++){
        material += pieceValues[piece] * __builtin_popcountll(board.pieceBB[color][piece]);
    }
    return material;
}

MgEgScore calculateBishopPair(const Board& board, Color color){
    if(__builtin_popcountll(board.pieceBB[color][BISHOP]) >= 2){
        return { BISHOP_PAIR_MG, BISHOP_PAIR_EG};
    }
    return {0, 0};
}

MgEgScore CalculateMobility(const Board& board, Color color) {
    MgEgScore score{0, 0};
    Color enemy = (color == WHITE) ? BLACK : WHITE;
    uint64_t enemyPawnAttacks = getPawnAttackBitboard(board, enemy);
    for (PieceType piece : {KNIGHT, BISHOP, ROOK, QUEEN}) {
        uint64_t pieces = board.pieceBB[color][piece];

        while (pieces) {
            int square = __builtin_ctzll(pieces);
            uint64_t mobility = getMobilityBitboard(board, square, piece, color) & ~enemyPawnAttacks; // I'm counting on SEE in search to check against other pieces since it's quite expensive. 
            int count = __builtin_popcountll(mobility);                                               // Getting the pawn attacks is very cheap though
            count = std::min(count, 27);

            score.mg += mg_Mobility[piece][count];
            score.eg += eg_Mobility[piece][count];

            pieces &= pieces - 1;
        }
    }

    return score;
}

int kingDistanceBonus(int myKingSquare, int enemyKingSquare) {
    int myFile = myKingSquare % 8;
    int myRank = myKingSquare / 8;
    int enemyFile = enemyKingSquare % 8;
    int enemyRank = enemyKingSquare / 8;

    int distance = abs(myFile - enemyFile) + abs(myRank - enemyRank);
    return 14 - distance;
}

bool isMopUpPhase(const Board& board, Color strongSide){
    if(board.gamePhase > MOPUP_PHASE_THRESHOLD) return false; // early cheap exit so the bottom doesn't run each time

    Color weakSide = (strongSide == WHITE) ? BLACK : WHITE;
    bool weakSideHasNoMajors = (board.pieceBB[weakSide][QUEEN] | board.pieceBB[weakSide][ROOK]) == 0;
    bool weakSideHasFewMinors = __builtin_popcountll(board.pieceBB[weakSide][KNIGHT] | board.pieceBB[weakSide][BISHOP]) <= 1;
    if(!weakSideHasNoMajors || !weakSideHasFewMinors) return false;

    int materialDiff = CountMaterial(board, strongSide) - CountMaterial(board, weakSide);
    return materialDiff > 400;
}

int Evaluate(const Board& board) {
    int whiteKingSquare = __builtin_ctzll(board.pieceBB[WHITE][KING]);
    int blackKingSquare = __builtin_ctzll(board.pieceBB[BLACK][KING]);

    MgEgScore whiteMobility = CalculateMobility(board, WHITE);
    MgEgScore blackMobility = CalculateMobility(board, BLACK);

    MgEgScore whiteBishopPair = calculateBishopPair(board, WHITE);
    MgEgScore blackBishopPair = calculateBishopPair(board, BLACK);

    int midgameEval = board.midgameScore[WHITE] - board.midgameScore[BLACK] + (whiteMobility.mg - blackMobility.mg) + (whiteBishopPair.mg - blackBishopPair.mg);
    int endgameEval = board.endgameScore[WHITE] - board.endgameScore[BLACK] + (whiteMobility.eg - blackMobility.eg) + (whiteBishopPair.eg - blackBishopPair.eg);

    if(isMopUpPhase(board, WHITE)){
        endgameEval += centerDistanceTable[blackKingSquare] * MOPUP_EDGE_WEIGHT + kingDistanceBonus(whiteKingSquare, blackKingSquare) * MOPUP_DISTANCE_WEIGHT;
    }
    if(isMopUpPhase(board, BLACK)){
        endgameEval -= centerDistanceTable[whiteKingSquare] * MOPUP_EDGE_WEIGHT + kingDistanceBonus(blackKingSquare, whiteKingSquare) * MOPUP_DISTANCE_WEIGHT;
    }

    // Tapered Eval
    int finalPhase = std::min(256, (board.gamePhase * 256 + 12) / 24);

    int evaluation = ((midgameEval * finalPhase) + (endgameEval * (256 - finalPhase))) / 256;

    return (board.turn == WHITE) ? evaluation : -evaluation;
}