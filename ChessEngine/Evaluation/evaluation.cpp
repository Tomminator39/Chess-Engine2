#include "evaluation.h"

void InitEvaluation() {
    // Array of pointers to your existing camelCase PSTs
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

int Evaluate(const Board& board) {
    int midgameEval = board.midgameScore[WHITE] - board.midgameScore[BLACK];
    int endgameEval = board.endgameScore[WHITE] - board.endgameScore[BLACK];

    // Tapered Eval
    int finalPhase = std::min(256, (board.gamePhase * 256 + 12) / 24);

    int evaluation = ((midgameEval * finalPhase) + (endgameEval * (256 - finalPhase))) / 256;

    return (board.turn == WHITE) ? evaluation : -evaluation;
}