#include <iostream>
#include "BoardRepresentation/board.h"
#include "BoardRepresentation/movegenerator.h"
#include "BoardRepresentation/zobrist.h"
#include "Evaluation/evaluation.h"

int main() {
    InitZobristKeys();
    PrecomputeMoveData();

    Board board;
    board.LoadPositionFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    printBoard(board);

    return 0;
}