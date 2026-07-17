#pragma once
#include "board.h"

struct MoveList{
    Move moves[256];
    int count = 0;
};

void PrecomputeMoveData();

bool isInCheck(const Board& board, Color side);

void RunPerftTest(Board& board, int depth);