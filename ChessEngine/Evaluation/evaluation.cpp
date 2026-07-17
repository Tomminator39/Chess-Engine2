#include "evaluation.h"

int CountMaterial(const Board& board, Color color){
    int material = 0;
    for(int piece = 0; piece < 6; piece++){
        material += pieceValues[piece] * __builtin_popcountll(board.pieceBB[color][piece]);
    }
    return material;
}

int Evaluate(const Board& board){
    int whiteEval = CountMaterial(board, WHITE);
    int blackEval = CountMaterial(board, BLACK);

    int evaluation = whiteEval - blackEval;
    int perspective = (board.turn == WHITE) ? 1 : -1;

    return evaluation * perspective;
    
}