#include "evaluation.h"

struct MgEgScore { int mg; int eg; };

constexpr int MOPUP_EDGE_WEIGHT = 4;
constexpr int MOPUP_DISTANCE_WEIGHT = 2;

static constexpr PieceType MOBILITY_PIECES[4] = { KNIGHT, BISHOP, ROOK, QUEEN };

// Optimized Chebyshev distance using bitwise math instead of div/mod
inline int GetChebyshevDistance(int sq1, int sq2) {
    int fileDiff = std::abs((sq1 & 7) - (sq2 & 7));
    int rankDiff = std::abs((sq1 >> 3) - (sq2 >> 3));
    return std::max(fileDiff, rankDiff);
}

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

            mgPst[piece][WHITE][whiteSq] = pieceValues[piece] + mgPsts[piece][square];
            mgPst[piece][BLACK][blackSq] = pieceValues[piece] + mgPsts[piece][square];

            egPst[piece][WHITE][whiteSq] = pieceValues[piece] + egPsts[piece][square];
            egPst[piece][BLACK][blackSq] = pieceValues[piece] + egPsts[piece][square];
        }
    }
}

int CountMaterial(const Board& board, Color color) {
    int material = 0;
    for (int piece = 0; piece < 6; piece++) {
        material += pieceValues[piece] * __builtin_popcountll(board.pieceBB[color][piece]);
    }
    return material;
}

MgEgScore calculateBishopPair(const Board& board, Color color) {
    if (__builtin_popcountll(board.pieceBB[color][BISHOP]) >= 2) {
        return { BISHOP_PAIR_MG, BISHOP_PAIR_EG };
    }
    return { 0, 0 };
}

MgEgScore CalculateMobility(const Board& board, Color color) {
    MgEgScore score{ 0, 0 };
    Color enemy = (color == WHITE) ? BLACK : WHITE;
    uint64_t enemyPawnAttacks = getPawnAttackBitboard(board, enemy);

    for (PieceType piece : MOBILITY_PIECES) {
        uint64_t pieces = board.pieceBB[color][piece];

        while (pieces) {
            int square = __builtin_ctzll(pieces);
            uint64_t mobility = getMobilityBitboard(board, square, piece, color) & ~enemyPawnAttacks;
            int count = std::min(__builtin_popcountll(mobility), 27);

            score.mg += mg_Mobility[piece][count];
            score.eg += eg_Mobility[piece][count];

            pieces &= pieces - 1;
        }
    }

    return score;
}

int CalculateMopUpScore(const Board& board, Color strongSide, int strongKingSq, int weakKingSq) {
    Color weakSide = (strongSide == WHITE) ? BLACK : WHITE;

    int materialDiff = CountMaterial(board, strongSide) - CountMaterial(board, weakSide);
    if (materialDiff < 400) return 0;

    if ((board.pieceBB[weakSide][QUEEN] | board.pieceBB[weakSide][ROOK]) != 0) {
        return 0;
    }

    if (__builtin_popcountll(board.pieceBB[weakSide][KNIGHT] | board.pieceBB[weakSide][BISHOP]) > 1) {
        return 0;
    }

    int pushToEdge = centerDistanceTable[weakKingSq] * MOPUP_EDGE_WEIGHT;

    int closeProximity = (7 - GetChebyshevDistance(strongKingSq, weakKingSq)) * MOPUP_DISTANCE_WEIGHT;

    return pushToEdge + closeProximity;
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

    endgameEval += CalculateMopUpScore(board, WHITE, whiteKingSquare, blackKingSquare);
    endgameEval -= CalculateMopUpScore(board, BLACK, blackKingSquare, whiteKingSquare);

    // Tapered eval
    int finalPhase = std::min(256, (board.gamePhase * 256 + 12) / 24);
    int evaluation = ((midgameEval * finalPhase) + (endgameEval * (256 - finalPhase))) / 256;

    return (board.turn == WHITE) ? evaluation : -evaluation;
}