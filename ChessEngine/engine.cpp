#include "engine.h"

void PlayEngineMove(Board& board, Searcher& searcher, int timeLimitMS){
    Move move = searcher.startSearch(board, timeLimitMS);

    if (move.data == 0){
        return;
    }
    
    board.makeMove(move);
}

void stopAndJoinIfRunning(std::thread& searchThread, Searcher& searcher){
    if(searchThread.joinable()){
        searcher.Stop();
        searchThread.join();

    }
}

std::string moveToString(const Move& move) {
    if (move.data == 0) {
        return "0000";
    }

    int fsq = move.fromSquare();
    int tsq = move.targetSquare();

    char fromFile = 'a' + (fsq % 8);
    char fromRank = '1' + (fsq / 8);
    char toFile   = 'a' + (tsq % 8);
    char toRank   = '1' + (tsq / 8);

    std::string str = "";
    str += fromFile;
    str += fromRank;
    str += toFile;
    str += toRank;

    if (move.isPromotion()) {
        int flags = move.flags();
        if (flags == FLAG_PROMOTION_KNIGHT || flags == FLAG_PROM_CAPT_KNIGHT) {
            str += 'n';
        } else if (flags == FLAG_PROMOTION_BISHOP || flags == FLAG_PROM_CAPT_BISHOP) {
            str += 'b';
        } else if (flags == FLAG_PROMOTION_ROOK || flags == FLAG_PROM_CAPT_ROOK) {
            str += 'r';
        } else if (flags == FLAG_PROMOTION_QUEEN || flags == FLAG_PROM_CAPT_QUEEN) {
            str += 'q';
        }
    }

    return str;
}

void parsePosition(Board& board, std::istringstream& ss) {
    std::string token;
    if (!(ss >> token)) return;

    if (token == "startpos") {
        board = Board();
        board.LoadPositionFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ss >> token; 
    } else if (token == "fen") {
        std::string fen = "";
        while (ss >> token && token != "moves") {
            fen += token + " ";
        }
        board = Board();
        board.LoadPositionFromFen(fen);
    }

    while (ss >> token) {
        MoveList moves = generateMoves(board);
        for (int i = 0; i < moves.count; i++) {
            Move candidate = moves.moves[i];
            
            if (moveToString(candidate) == token) {
                board.makeMove(candidate);
                break;
            }
        }
    }
}

int parseGo(Board& board, std::istringstream& ss){
    std::string token;
    int depth = 64; // effectively infinite, time will stop it
    int wtime = 0, btime = 0, winc = 0, binc = 0;
    int movestogo = 0; 
    int movetime = 0;

    while(ss >> token){
        if(token == "depth") ss >> depth; // TODO: Actually implement this
        else if(token == "movetime") ss >> movetime;
        else if(token == "wtime") ss >> wtime;
        else if(token == "btime") ss >> btime;
        else if(token == "winc") ss >> winc;
        else if(token == "binc") ss >> binc;
        else if(token == "movestogo") ss >> movestogo; //TODO: Actually implement this
    }

    int timeToSpend = 0;
    int myTime = (board.turn == WHITE) ? wtime : btime;
    int myInc = (board.turn == WHITE) ? winc : binc;

    if(movetime > 0){
        timeToSpend = movetime;
    } else if(myTime > 0){
        timeToSpend = myTime / 20 + myInc / 2;
        timeToSpend = std::min(timeToSpend, myTime / 2);
    } else {
        timeToSpend = 1000;
    }

    return timeToSpend;
}

void readCommand(const std::string& command, Board& board, std::istringstream& ss, Searcher& searcher, std::thread& searchThread){
    if(command == "uci"){
        std::cout << "id name TomminatorCE" << std::endl;
        std::cout << "id author Tom Hillen" << std::endl;
        std::cout << "uciok" << std::endl;
    }
    else if(command == "isready"){
        std::cout << "readyok" << std::endl;;
    }
    else if(command == "ucinewgame"){
        stopAndJoinIfRunning(searchThread, searcher);
        board = Board();
        board.LoadPositionFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        searcher.Reset();
    }
    else if(command == "position"){
        stopAndJoinIfRunning(searchThread, searcher);
        parsePosition(board, ss);
    }
    else if(command == "go"){
        stopAndJoinIfRunning(searchThread, searcher);
        int timeToSpend = parseGo(board, ss);

        searchThread = std::thread([&board, &searcher, timeToSpend](){
        Move result = searcher.startSearch(board, timeToSpend);
        std::cout << "bestmove " << moveToString(result) << std::endl;
    });
    }
    else if(command == "stop"){
        stopAndJoinIfRunning(searchThread, searcher);
    }
}