#include "search.h"

constexpr int INF = 1000000000;

int Search(Board& board, int alpha, int beta, int depth, int ply){
    //TODO: Check draw by repetition and if this exact position has already been found in a transposition table
    // Also put possible stopsearch check here

    if(depth == 0) return Evaluate(board); //TODO: Replace with quiescence search

    CheckInfo info = getCheckInfo(board);
    int legalMoveCount = 0;
    MoveList moves = generateMoves(board);

    //TODO: Move ordering here? + Order best move from transposition table / iterative deepening

    for(int i = 0; i < moves.count; i++){
        Move move = moves.moves[i];
        Color mover = board.turn;
        
        int fromPiece = board.mailbox[move.fromSquare()];
        PieceType movedType = mailboxType(fromPiece);

        if(__builtin_popcountll(info.checkers) >= 2 && movedType != KING){
            continue;
        }

        board.makeMove(move);
        
        if(movedType == KING || (info.pinned & (1ULL << move.fromSquare())) || move.flags() == FLAG_EN_PASSANT || info.inCheck) {
            if(isInCheck(board, mover)) {
                board.unmakeMove(move); 
                continue;
            }
        }

        legalMoveCount++;

        int eval = -Search(board, -beta, -alpha, depth - 1, ply + 1);
        board.unmakeMove(move);

        // Put possible stopsearch check here

        if(eval >= beta){
            return beta;
        }

        if(eval > alpha){
            alpha = eval;
        }
    }

    if(legalMoveCount == 0){
        if(info.inCheck) {
            return -(INF - ply - 1);
        }
        return 0;
    }

    return alpha;
}