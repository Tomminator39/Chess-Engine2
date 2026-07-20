#include <iostream>
#include <thread>
#include "BoardRepresentation/board.h"
#include "BoardRepresentation/movegenerator.h"
#include "BoardRepresentation/zobrist.h"
#include "Evaluation/evaluation.h"
#include "Search/search.h"
#include "engine.h"

int main() {
    InitZobristKeys();
    PrecomputeMoveData();

    Board board;
    board.LoadPositionFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Searcher searcher(128);

    std::thread searchThread;
    std::string line;

    while(std::getline(std::cin, line)){
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if(token == "quit"){
            if(searchThread.joinable()){
                searcher.Stop();
                searchThread.join();
            }
            break;
        }

        readCommand(token, board, ss, searcher, searchThread);
    }
    return 0;
}