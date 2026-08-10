#include "search.h"

constexpr int NO_STATIC_EVAL = 32000;

constexpr int lmpThreshold[2][9] = {
    { 0, 2, 5, 10, 17, 26, 37, 50, 65 },  // not improving
    { 0, 5, 11, 21, 35, 53, 75, 101, 131 } // improving
};

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

std::string FormatScore(int score) {
    if (isMateScore(score)) {
        int pliesToMate = MATE_VALUE - abs(score);
        int movesToMate = (pliesToMate + 1) / 2;
        if (score < 0) movesToMate = -movesToMate;
        return "mate " + std::to_string(movesToMate);
    }
    return "cp " + std::to_string(score);
}

void Searcher::InitLMRTable(){ // Should change the values of a and b at some point with SPSA or texel tuning
    for(int d = 1; d < MAX_PLY; d++){
        for(int m = 1; m < 256; m++){
            lmrTable[d][m] = static_cast<int>(0.5 + log(d) * log(m) / 2.5);
        }
    }
}

void Searcher::printInfoString(int depth, int score, uint64_t time_ms){
    uint64_t nps = (time_ms > 0) ? (nodeCount * 1000 / time_ms) : 0;

    std::string scoreString = FormatScore(score);

    std::cout << "info depth " << depth << " score " << scoreString << " nodes " << nodeCount << " nps " << nps << " time " << time_ms << " pv ";
    for (int i = 0; i < pvLength[0]; i++) {
        std::cout << moveToString(pvTable[0][i]) << " ";
    }

    std::cout << std::endl;
}

void PrintBookInfoLine(const Board& board, Move bookMove){
    int eval = Evaluate(board);
    std::cout << "info depth 1 score " << FormatScore(eval)
               << " nodes 0 time 0 pv " << moveToString(bookMove) << std::endl;
}

std::string GetBookLookupKey(Board& board){
    std::string fen = board.BoardToFEN();

    if (isEnPassantCapturable(board)) {
        return fen;
    }

    std::istringstream iss(fen);
    std::string pieces, side, castling, ep, halfmove, fullmove;
    iss >> pieces >> side >> castling >> ep >> halfmove >> fullmove;

    return pieces + " " + side + " " + castling + " - " + halfmove + " " + fullmove;
}

int Searcher::Quiescence(Board& board, int alpha, int beta, int ply, Move previousMove){
    nodeCount++;
    if (nodeCount % 2048 == 0){
        if(std::chrono::steady_clock::now() >= deadline){
            stopSearch = true;
        }
    }
    
    if(stopSearch) return 0;

    pvLength[ply] = 0;

    if(ply > 0 && (board.isRepetition() || board.halfMoveCounter >= 100)) return 0; //TODO: Add contempt here

    // If in check generateMoves (later change to specific check-evasion generator) else only generate captures
    ProbeResult evalCheck = transpositionTable.Probe(board.zobristKey, 0, alpha, beta, ply);
    
    bool isPvNode = (beta - alpha) > 1;
    if(!isPvNode && ply > 0 && evalCheck.cutoff){
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
    moveOrderer.ScoreMoves(board, moves, priorityMove, scores, ply, previousMove);

    TTFlag evalType = TTFlag::UPPER_BOUND;
    Move bestMoveInCurrentSearch;
    bool firstMoveSearched = false;

    for(int i = 0; i < moves.count; i++){
        // Selection sort for move ordering
        int bestIndex = selectBestMoveIndex(scores, i, moves.count);
        std::swap(moves.moves[i], moves.moves[bestIndex]);
        std::swap(scores[i], scores[bestIndex]);

        Move move = moves.moves[i];
        Color mover = board.turn;

        if(!info.inCheck && move.data != priorityMove.data 
            && move.isCapture() && scores[i] < SCORE_QUIET){
                break; // this and all subsequent moves are worse (scores are sorted)
            }
        
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

        if(!move.isCapture()){
            moveOrderer.RecordAttempt(mover, move);
        }

        legalMoveCount++;
        int eval;

        if(!firstMoveSearched){ // PVS
            eval = -Quiescence(board, -beta, -alpha, ply + 1, move);
            firstMoveSearched = true;
        }
        else{
            eval = -Quiescence(board, -alpha - 1, -alpha, ply + 1, move);
            if (eval > alpha && eval < beta) {
                eval = -Quiescence(board, -beta, -alpha, ply + 1, move);
            }
        }
        board.unmakeMove(move);

        if(stopSearch) return 0;

        if(eval >= beta){
            moveOrderer.RecordCutoff(move, ply, 1, mover);
            moveOrderer.RecordCounter(previousMove, move, mover == WHITE ? BLACK : WHITE);
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

            pvTable[ply][0] = move;
            for(int j = 0; j < pvLength[ply + 1]; j++){
                pvTable[ply][j + 1] = pvTable[ply + 1][j];
            }
            pvLength[ply] = pvLength[ply + 1] + 1;
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

int Searcher::Search(Board& board, int alpha, int beta, int depth, int ply, Move previousMove, bool previousWasNullMove){
    nodeCount++;
    if (nodeCount % 2048 == 0){
        if(std::chrono::steady_clock::now() >= deadline){
            stopSearch = true;
        }
    }
    
    if(stopSearch) return 0;

    pvLength[ply] = 0;

    if(ply > 0 && (board.isRepetition() || board.halfMoveCounter >= 100)) return 0; //TODO: Add contempt here

    if(depth == 0) return Quiescence(board, alpha, beta, ply, previousMove);

    ProbeResult evalCheck = transpositionTable.Probe(board.zobristKey, depth, alpha, beta, ply);
    
    bool isPvNode = (beta - alpha) > 1;
    if(!isPvNode && ply > 0 && evalCheck.cutoff){
        return evalCheck.score;
    }

    CheckInfo info = getCheckInfo(board);

    int staticEval = info.inCheck ? NO_STATIC_EVAL : Evaluate(board);
    staticEvalHistory[ply] = staticEval;

    bool improving = !info.inCheck && ply >= 2 && staticEvalHistory[ply - 2] != NO_STATIC_EVAL && staticEval > staticEvalHistory[ply - 2];

    // Reverse Futility Pruning 
    if (ply > 0 && !isPvNode && !info.inCheck && depth <= 3){
        int margin = 120 * depth;
        if(!improving) margin += 60;
        if (staticEval - margin >= beta) {
            if(!isMateScore(beta)){
                return beta;
            }
        }
    }

    // Null move pruning. before generating moves to possibly save time, but we do need checkinfo
    bool hasNonPawnMaterial = (board.pieceBB[board.turn][KNIGHT] | board.pieceBB[board.turn][BISHOP] | board.pieceBB[board.turn][ROOK] | board.pieceBB[board.turn][QUEEN]) != 0; // Zugzwang cases
    if(depth >= 3 && !isPvNode && !info.inCheck && !previousWasNullMove && hasNonPawnMaterial){
        int R = 3; 
        board.makeNullMove();
        int reducedDepth = std::max(0, depth - 1 - R);
        int nullEval = -Search(board, -beta, -beta + 1, reducedDepth, ply + 1, Move(), true);
        board.unmakeNullMove();

        if(stopSearch) return 0;

        if(nullEval >= beta){
            if (nullEval >= MATE_VALUE - MAX_PLY) return beta;
            return beta;
        }
    }

    int legalMoveCount = 0;
    int quietMoveCount = 0; // Tracked for LMP  
    MoveList moves = generateMoves(board);

    // Move ordering
    Move priorityMove = ply == 0 ? bestMove : (evalCheck.found ? evalCheck.bestMove : Move());
    int scores[256]; 
    moveOrderer.ScoreMoves(board, moves, priorityMove, scores, ply, previousMove);

    TTFlag evalType = TTFlag::UPPER_BOUND;
    Move bestMoveInCurrentSearch;
    bool firstMoveSearched = false;

    for(int i = 0; i < moves.count; i++){
        // Selection sort for move ordering
        int bestIndex = selectBestMoveIndex(scores, i, moves.count);
        std::swap(moves.moves[i], moves.moves[bestIndex]);
        std::swap(scores[i], scores[bestIndex]);

        Move move = moves.moves[i];
        Color mover = board.turn;
        bool isQuiet = !move.isCapture() && !move.isPromotion();

        // LMP
        if(!isPvNode && !info.inCheck && depth <= 5 && isQuiet && quietMoveCount >= lmpThreshold[improving][depth] && !moveOrderer.IsKiller(move, ply) && !isInCheck(board, board.turn)){ //TODO: tune the depth value
            continue;
        }

        int fromPiece = board.mailbox[move.fromSquare()];
        PieceType movedType = mailboxType(fromPiece);

        if(__builtin_popcountll(info.checkers) >= 2 && movedType != KING){ // Legality checking
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

        if (isQuiet) {
            quietMoveCount++;
            moveOrderer.RecordAttempt(mover, move);
        }

        int eval;

        int reduction = 0;

        // Grab LMR reduction from table
        if(firstMoveSearched && depth >= 3 && !move.isCapture() && !move.isPromotion() && !info.inCheck && isPvNode == false && !isInCheck(board, board.turn)){
            reduction = lmrTable[depth][i];
            if(!improving) reduction += 1;  // one extra ply of reduction when not improving
            if(moveOrderer.IsKiller(move, ply)) reduction = reduction / 2;  // reduce killers less, but not totally
            reduction = std::min(reduction, depth - 1);  // never let reduced depth go below 1
        }

        if(!firstMoveSearched){ // PVS
            eval = -Search(board, -beta, -alpha, depth - 1, ply + 1, move);
            firstMoveSearched = true;
        }
        else{
            eval = -Search(board, -alpha - 1, -alpha, depth - 1 - reduction, ply + 1, move);
            if(eval > alpha && reduction > 0){
                // the reduced search beat alpha so re-search at full depth still null window to confirm
                eval = -Search(board, -alpha - 1, -alpha, depth - 1, ply + 1, move);
            }
            if(eval > alpha && eval < beta){
                eval = -Search(board, -beta, -alpha, depth - 1, ply + 1, move);
            }
        }

        board.unmakeMove(move);

        if(stopSearch) return 0;

        if(eval >= beta){
            moveOrderer.RecordCutoff(move, ply, depth, mover);
            moveOrderer.RecordCounter(previousMove, move, mover == WHITE ? BLACK : WHITE);
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

            pvTable[ply][0] = move;
            for(int j = 0; j < pvLength[ply + 1]; j++){
                pvTable[ply][j + 1] = pvTable[ply + 1][j];
            }
            pvLength[ply] = pvLength[ply + 1] + 1;
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
    std::string currentFen = GetBookLookupKey(board);
    std::string bookMoveString;

    if (openingBook.TryGetBookMove(currentFen, bookMoveString)) {
        Move bookMove = stringToMove(bookMoveString, board);
        if (bookMove.data != 0) {
            PrintBookInfoLine(board, bookMove);
            return bookMove;
        }
    }

    stopSearch = false; // Stop a true from a previous search from cancelling a new search
    nodeCount = 0;
    moveOrderer.DecayHistory();
    auto searchStartTime = std::chrono::steady_clock::now();
    deadline = searchStartTime + std::chrono::milliseconds(timeLimitMS);

    // Iterative deepening loop
    for(int searchDepth = 1; searchDepth < 64; searchDepth++){ // 64 is essentially infinite, but time control should stop a search
        bestMoveThisIteration.data = 0;
        bestEvalThisIteration = -INF;

        int alpha, beta, delta = 30;

        if(searchDepth == 1 || isMateScore(bestEval)){ // Prevent widening the window around a mate score since it arbitrarily large.
            alpha = -INF;
            beta = INF;
        } else {
            alpha = bestEval - delta;
            beta = bestEval + delta;
        }

        int score;
        while(true){ // Aspiration Window
            score = Search(board, alpha, beta, searchDepth, 0, Move());

            if(stopSearch) break;

                if(score <= alpha){
                alpha = std::max(-INF, alpha - delta);
                delta *= 2;
            } else if(score >= beta){
                beta = std::min(INF, beta + delta);
                delta *= 2;
            } else {
                break; // landed inside the window
            }

            if(isMateScore(score)){ // Same check as earlier
                alpha = -INF;
                beta = INF;
            }
        }

        if(bestMoveThisIteration.data != 0){
            bestMove = bestMoveThisIteration;
            bestEval = bestEvalThisIteration;

            auto currentTime = std::chrono::steady_clock::now();
            auto timeSpentMS = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - searchStartTime).count();
            printInfoString(searchDepth, bestEval, timeSpentMS);
        }

        if(stopSearch){
            break;
        }
    }

    return bestMove;
}