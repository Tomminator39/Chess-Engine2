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

Move stringToMove(std::string moveString, Board& board);

bool isEnPassantCapturable(const Board& board);

bool isInCheck(const Board& board, Color side);

CheckInfo getCheckInfo(const Board& board);

uint64_t attackersTo(const Board& board, int square, uint64_t occupied); // Gets all attackers from both sides on a square. Mainly used for SEE
uint64_t getPawnAttackBitboard(const Board& board, Color color);
uint64_t getMobilityBitboard(const Board& board, int square, PieceType piece, Color sideToMove);

MoveList generateMoves(Board& board, bool onlyGenerateCaptures = false);

void RunPerftTest(Board& board, int depth);