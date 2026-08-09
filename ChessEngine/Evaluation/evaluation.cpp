#include "evaluation.h"



struct MobilityScore { int mg; int eg; };

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

MobilityScore CalculateMobility(const Board& board, Color color) { // Does not account for pins, I think it'd be too expensive. 
    MobilityScore score{0, 0};                                     // I do think I should wire in an updated attack table at some point for safe mobility

    for (PieceType piece : {KNIGHT, BISHOP, ROOK, QUEEN}) {
        uint64_t pieces = board.pieceBB[color][piece];

        while (pieces) {
            int square = __builtin_ctzll(pieces);
            uint64_t mobility = getMobilityBitboard(board, square, piece, color);
            int count = __builtin_popcountll(mobility);
            count = std::min(count, 27);

            score.mg += mg_Mobility[piece][count];
            score.eg += eg_Mobility[piece][count];

            pieces &= pieces - 1;
        }
    }

    return score;
}

int Evaluate(const Board& board) {
    MobilityScore whiteMobility = CalculateMobility(board, WHITE);
    MobilityScore blackMobility = CalculateMobility(board, BLACK);   

    int midgameEval = board.midgameScore[WHITE] - board.midgameScore[BLACK] + (whiteMobility.mg - blackMobility.mg);
    int endgameEval = board.endgameScore[WHITE] - board.endgameScore[BLACK] + (whiteMobility.eg - blackMobility.eg);;

    // Tapered Eval
    int finalPhase = std::min(256, (board.gamePhase * 256 + 12) / 24);

    int evaluation = ((midgameEval * finalPhase) + (endgameEval * (256 - finalPhase))) / 256;

    return (board.turn == WHITE) ? evaluation : -evaluation;
}