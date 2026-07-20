#pragma once
#include "board.h"

struct MoveList{
    Move moves[256];
    int count = 0;
};

struct CheckInfo{
    bool inCheck;
    uint64_t pinned = 0;
    uint64_t checkers = 0; // If this is bigger or equal than 2 the king must move
};

void PrecomputeMoveData();

bool isInCheck(const Board& board, Color side);

CheckInfo getCheckInfo(const Board& board);

MoveList generateMoves(Board& board, bool onlyGenerateCaptures = false);

void RunPerftTest(Board& board, int depth);