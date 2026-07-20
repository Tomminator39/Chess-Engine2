#include "BoardRepresentation/board.h"
#include "Search/search.h"

void PlayEngineMove(Board& board, Searcher& searcher, int timeLimitMS){
    Move move = searcher.startSearch(board, timeLimitMS);

    if (move.data == 0){
        return;
    }
    
    board.makeMove(move);
}