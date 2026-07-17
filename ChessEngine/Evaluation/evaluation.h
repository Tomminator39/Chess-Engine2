#pragma once
#include "../BoardRepresentation\board.h"

constexpr int pieceValues[] = {100, 300, 310, 525, 950, 0}; // pawn, knight, bishop, rook, queen, king

int Evaluate(const Board& board);
int CountMaterial(const Board& board, Color color);