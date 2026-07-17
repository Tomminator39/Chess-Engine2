#include <iostream>
#include "BoardRepresentation/board.h"
#include "BoardRepresentation/movegenerator.h"

int main() {
    PrecomputeMoveData();

    Board board;
    board.LoadPositionFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    printBoard(board);

    RunPerftTest(board, 6);

    return 0;
}