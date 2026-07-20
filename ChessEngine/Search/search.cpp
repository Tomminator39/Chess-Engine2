#include "search.h"

size_t TranspositionTable::calculateTableSize(size_t megabytes) {
    size_t bytes = megabytes * 1024 * 1024;
    size_t entrySize = sizeof(TranspositionEntry);
    
    size_t maxEntries = bytes / entrySize;

    return std::bit_floor(maxEntries);
}

void TranspositionTable::Store(uint64_t key, int score, int depth, Move bestMove, TTFlag flag, int ply){
    size_t index = key & (entries.size() - 1);

    TranspositionEntry& existing = entries[index];
    if (currentAge == existing.age && (currentAge != existing.age || depth < existing.depth)) return;

    int newScore = score;
    if (isMateScore(newScore)) {
        if (newScore > 0) newScore += ply;
        else newScore -= ply;
    }

    entries[index] = { key, bestMove, static_cast<int16_t>(newScore), currentAge, static_cast<uint8_t>(depth), flag};
}

ProbeResult TranspositionTable::Probe(uint64_t key, int depth, int alpha, int beta, int ply){
    ProbeResult result;
    result.found = false;
    result.cutoff = false;

    size_t index = key & (entries.size() - 1);
    TranspositionEntry& existing = entries[index];

    if(existing.hash != key) return result;
    result.found = true;
    result.bestMove = existing.bestMove; // For move ordering

    int score = existing.score;
    if (isMateScore(score)) {
        if (score > 0) score -= ply;
        else score += ply;
    }

    if (existing.depth >= depth) {
        
        if (existing.flag == TTFlag::EXACT) {
            result.score = score;
            result.cutoff = true;
            return result;
        }
        
        if (existing.flag == TTFlag::UPPER_BOUND && score <= alpha) {
            result.score = alpha; // or score
            result.cutoff = true;
            return result;
        }
        
        if (existing.flag == TTFlag::LOWER_BOUND && score >= beta) {
            result.score = beta; // or score
            result.cutoff = true;
            return result;
        }
    }

    return result;
}

void scoreMoves(Board& board, MoveList& moves, Move priorityMove, int* scores){ // Instead of one order moves function, this function scores each move and then SelectBestMove handles selection sort.
    for(int i = 0; i < moves.count; i++){
        Move move = moves.moves[i];

        if(move.data == priorityMove.data && priorityMove.data != 0){
            scores[i] = INF;
            continue;
        }

        if(move.isCapture()){
            int victimSquare = (move.flags() == FLAG_EN_PASSANT) ? ((board.turn == WHITE) ? move.targetSquare() - 8 : move.targetSquare() + 8) : move.targetSquare();
            int8_t victimPiece = board.mailbox[victimSquare];
            int8_t attackerPiece = board.mailbox[move.fromSquare()];
            int victimValue = pieceValues[mailboxType(victimPiece)];
            int attackerValue = pieceValues[mailboxType(attackerPiece)];

            scores[i] = victimValue * 10 - attackerValue;
        }
        else{
            scores[i] = 0;
        }
    }
}

int selectBestMoveIndex(int* scores, int i, int count){
    int bestIndex = i;
    for(int j = i; j < count; j++){
        if(scores[j] > scores[bestIndex]){
            bestIndex = j;
        }
    }
    return bestIndex;
}

int Searcher::Quiescence(Board& board, int alpha, int beta, int ply){
    nodeCount++;
    if (nodeCount % 2048 == 0){
        if(std::chrono::steady_clock::now() >= deadline){
            stopSearch = true;
        }
    }
    
    if(stopSearch) return 0;

    // If in check generateMoves (later change to specific check-evasion generator) else only generate captures
    ProbeResult evalCheck = transpositionTable.Probe(board.zobristKey, 0, alpha, beta, ply);
    if(ply > 0 && evalCheck.cutoff){
        return evalCheck.score;
    }

    CheckInfo info = getCheckInfo(board);
    int legalMoveCount = 0;
    MoveList moves;
    if(info.inCheck){
        moves = generateMoves(board);
    }
    else{
        int evaluation = Evaluate(board);
        if(evaluation >= beta){
            return beta;
        }
        if(evaluation > alpha) alpha = evaluation;
        moves = generateMoves(board, true);
    }

    // Move ordering
    Move priorityMove = ply == 0 ? bestMove : (evalCheck.found ? evalCheck.bestMove : Move());
    int scores[256]; 
    scoreMoves(board, moves, priorityMove, scores);

    TTFlag evalType = TTFlag::UPPER_BOUND;
    Move bestMoveInCurrentSearch;

    for(int i = 0; i < moves.count; i++){
        // Selection sort for move ordering
        int bestIndex = selectBestMoveIndex(scores, i, moves.count);
        std::swap(moves.moves[i], moves.moves[bestIndex]);
        std::swap(scores[i], scores[bestIndex]);

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

        int eval = -Quiescence(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move);

        if(stopSearch) return 0;

        if(eval >= beta){
            transpositionTable.Store(board.zobristKey, beta, 0, move, TTFlag::LOWER_BOUND, ply);
            return beta;
        }

        if(eval > alpha){
            alpha = eval;
            evalType = TTFlag::EXACT;
            bestMoveInCurrentSearch = move;
            if(ply == 0){
                bestMoveThisIteration = move;
                bestEvalThisIteration = eval;
            }
        }
    }

    if(legalMoveCount == 0 && info.inCheck){
        Move nullMove;
        transpositionTable.Store(board.zobristKey, -(MATE_VALUE - ply), MAX_PLY, nullMove, TTFlag::EXACT, ply);
        return -(MATE_VALUE - ply);
    }

    transpositionTable.Store(board.zobristKey, alpha, 0, bestMoveInCurrentSearch, evalType, ply);
    return alpha;
}

int Searcher::Search(Board& board, int alpha, int beta, int depth, int ply){
    nodeCount++;
    if (nodeCount % 2048 == 0){
        if(std::chrono::steady_clock::now() >= deadline){
            stopSearch = true;
        }
    }
    
    if(stopSearch) return 0;

    if(depth == 0) return Quiescence(board, alpha, beta, ply);

    //TODO: Detect draw by repetition (dont forget to add this to quiescence?)

    ProbeResult evalCheck = transpositionTable.Probe(board.zobristKey, depth, alpha, beta, ply);
    if(ply > 0 && evalCheck.cutoff){
        return evalCheck.score;
    }

    CheckInfo info = getCheckInfo(board);
    int legalMoveCount = 0;
    MoveList moves = generateMoves(board);

    // Move ordering
    Move priorityMove = ply == 0 ? bestMove : (evalCheck.found ? evalCheck.bestMove : Move());
    int scores[256]; 
    scoreMoves(board, moves, priorityMove, scores);

    TTFlag evalType = TTFlag::UPPER_BOUND;
    Move bestMoveInCurrentSearch;

    for(int i = 0; i < moves.count; i++){
        // Selection sort for move ordering
        int bestIndex = selectBestMoveIndex(scores, i, moves.count);
        std::swap(moves.moves[i], moves.moves[bestIndex]);
        std::swap(scores[i], scores[bestIndex]);

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

        if(stopSearch) return 0;

        if(eval >= beta){
            transpositionTable.Store(board.zobristKey, beta, depth, move, TTFlag::LOWER_BOUND, ply);
            return beta;
        }

        if(eval > alpha){
            alpha = eval;
            evalType = TTFlag::EXACT;
            bestMoveInCurrentSearch = move;
            if(ply == 0){
                bestMoveThisIteration = move;
                bestEvalThisIteration = eval;
            }
        }
    }

    if(legalMoveCount == 0){
        Move nullMove;
        if(info.inCheck) {
            transpositionTable.Store(board.zobristKey, -(MATE_VALUE - ply), MAX_PLY, nullMove, TTFlag::EXACT, ply);
            return -(MATE_VALUE - ply);
        }
        transpositionTable.Store(board.zobristKey, 0, depth, nullMove, TTFlag::EXACT, ply);
        return 0;
    }

    transpositionTable.Store(board.zobristKey, alpha, depth, bestMoveInCurrentSearch, evalType, ply);
    return alpha;
}

Move Searcher::startSearch(Board& board, int timeLimitMS){
    stopSearch = false; // Stop a true from a previous search from cancelling a new search
    deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeLimitMS);

    // Iterative deepening loop
    for(int searchDepth = 1; searchDepth < 64; searchDepth++){ // 64 is essentially infinite, but time control should stop a search
        bestMoveThisIteration.data = 0;
        bestEvalThisIteration = -INF;
        Search(board, -INF, INF, searchDepth, 0);

        if(bestMoveThisIteration.data != 0){
            bestMove = bestMoveThisIteration;
            bestEval = bestEvalThisIteration;
        }

        if(stopSearch){
            break;
        }
    }

    return bestMove;
}