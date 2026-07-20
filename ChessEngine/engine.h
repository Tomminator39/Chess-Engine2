#pragma once
#include "BoardRepresentation/board.h"
#include "Search/search.h"
#include <thread>
#include <algorithm>

void PlayEngineMove(Board& board, Searcher& searcher, int timeLimitMS);

void readCommand(const std::string& command, Board& board, std::istringstream& ss, Searcher& searcher, std::thread& searchThread);