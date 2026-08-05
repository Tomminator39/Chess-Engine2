#include "moveordering.h"

int selectBestMoveIndex(int* scores, int i, int count){
    int bestIndex = i;
    for(int j = i; j < count; j++){
        if(scores[j] > scores[bestIndex]){
            bestIndex = j;
        }
    }
    return bestIndex;
}

// Maps promotion flag bits (00=knight, 01=bishop, 10=rook, 11=queen)
// to a PieceType. Works for both FLAG_PROMOTION_* and FLAG_PROM_CAPT_*
// since both encode the promo piece in the low 2 bits.
constexpr PieceType promotionPieceType(Move move) {
    constexpr PieceType table[4] = { KNIGHT, BISHOP, ROOK, QUEEN };
    return table[move.flags() & 0x3];
}

bool MoveOrderer::IsKiller(Move move, int ply){
    return move.data == killerMoves[ply][0].data || move.data == killerMoves[ply][1].data;
}

void MoveOrderer::RecordAttempt(Color color, Move move){
    historyTotal[color][move.fromSquare()][move.targetSquare()]++;
}

void MoveOrderer::RecordCounter(Move previousMove, Move newCounterMove, Color color){
    if (previousMove.data == 0 || newCounterMove.data == 0) return;  // no real previous move (root), nothing to key off
    counterMove[color][previousMove.fromSquare()][previousMove.targetSquare()] = newCounterMove;
}

void MoveOrderer::DecayHistory(){
    for(int c = 0; c < 2; c++)
        for(int f = 0; f < 64; f++)
            for(int t = 0; t < 64; t++){
                historySuccess[c][f][t] /= 2;
                historyTotal[c][f][t] /= 2;
            }
}

int MoveOrderer::GetMVVLVA(const Board& board, Move move) {
    PieceType victimPiece = (move.flags() == FLAG_EN_PASSANT) ? PAWN : mailboxType(board.mailbox[move.targetSquare()]);
    PieceType attackerPiece = mailboxType(board.mailbox[move.fromSquare()]);

    return pieceValues[victimPiece] - pieceValues[attackerPiece];
}

void MoveOrderer::RecordCutoff(Move move, int ply, int depth, Color color){
    if(!move.isCapture() && move.data != killerMoves[ply][0].data){
        killerMoves[ply][1] = killerMoves[ply][0];
        killerMoves[ply][0] = move;
    }
    if(!move.isCapture()){
        historySuccess[color][move.fromSquare()][move.targetSquare()] += depth * depth;
    }
}

int MoveOrderer::SEE(const Board& board, Move move) {
    int targetSquare = move.targetSquare();
    int fromSquare = move.fromSquare();

    // Handle en passant target square
    int victimSquare = (move.flags() == FLAG_EN_PASSANT) ? ((board.turn == WHITE) ? targetSquare - 8 : targetSquare + 8) : targetSquare;

    int gains[32];
    int depth = 0;

    uint64_t occupied = board.occupancy[2];
    Color sideToMove = board.turn;

    bool isPromotion = move.isPromotion();
    PieceType promoPiece = isPromotion ? promotionPieceType(move) : PAWN;

    PieceType currentAttackerType = isPromotion ? promoPiece : mailboxType(board.mailbox[fromSquare]);

    gains[0] = pieceValues[mailboxType(board.mailbox[victimSquare])];
    if (isPromotion) {
        gains[0] += pieceValues[promoPiece] - pieceValues[PAWN];
    }

    // Remove first attacker
    occupied &= ~(1ULL << fromSquare);
    occupied &= ~(1ULL << victimSquare);

    while (true) {
        depth++;
        sideToMove = (sideToMove == WHITE) ? BLACK : WHITE;

        // Gain if current attacker is captured
        gains[depth] = pieceValues[currentAttackerType] - gains[depth - 1];

        if (std::max(-gains[depth - 1], gains[depth]) < 0) break; // Early exit if the best this side can do is losing

        uint64_t attackers = attackersTo(board, targetSquare, occupied);

        // Find lowest-value attacker belonging to sideToMove
        int nextAttackerSquare = -1;
        for (PieceType p = PAWN; p <= KING; p = static_cast<PieceType>(p + 1)) {
            uint64_t myAttackers = attackers & board.pieceBB[sideToMove][p];
            if (myAttackers != 0) {
                nextAttackerSquare = __builtin_ctzll(myAttackers);
                currentAttackerType = p;
                break;
            }
        }

        if (nextAttackerSquare == -1) break;

        if (currentAttackerType == KING) {
            uint64_t opponentAttackers = attackers & ~board.occupancy[sideToMove];
            if (opponentAttackers != 0) {
                break;
            }
        }
        occupied &= ~(1ULL << nextAttackerSquare);
    }

    while (depth > 1) {
        depth--;
        gains[depth - 1] = -std::max(-gains[depth - 1], gains[depth]);
    }
    return gains[0];
}

void MoveOrderer::Reset(){
    std::fill(&killerMoves[0][0], &killerMoves[0][0] + MAX_PLY * 2, Move());
    std::fill(&historySuccess[0][0][0], &historySuccess[0][0][0] + 2*64*64, 0);
    std::fill(&historyTotal[0][0][0], &historyTotal[0][0][0] + 2*64*64, 0);
    std::fill(&counterMove[0][0][0], &counterMove[0][0][0] + 2*64*64, Move());
}

void MoveOrderer::ScoreMoves(const Board& board, MoveList& moves, Move priorityMove, int* scores, int ply, Move previousMove){
    for(int i = 0; i < moves.count; i++){
        Move move = moves.moves[i];

        if(move.data == priorityMove.data && priorityMove.data != 0){ // Hash move
            scores[i] = SCORE_PRIORITY;
            continue;
        }

        if(move.isCapture()){ // Captures (includes capture-promotions)
            int mvvlvaScore = GetMVVLVA(board, move);
            if(SEE(board, move) >= 0){
                scores[i] = SCORE_WINNING_CAPT + mvvlvaScore;
            }
            else{
                scores[i] = SCORE_LOSING_CAPT + mvvlvaScore;
            }
        }
        // Quiet promotions
        else if(move.isPromotion()){
            if(promotionPieceType(move) == QUEEN){
                scores[i] = SCORE_PROMOTION;
            }
            else if(move.data == killerMoves[ply][0].data || move.data == killerMoves[ply][1].data){
                scores[i] = SCORE_KILLER;
            }
            else{
                scores[i] = SCORE_QUIET;
            }
        }
        // Killer Moves
        else if(move.data == killerMoves[ply][0].data || move.data == killerMoves[ply][1].data){
            scores[i] = SCORE_KILLER;
        }
        else if(previousMove.data != 0 && move.data == counterMove[!board.turn][previousMove.fromSquare()][previousMove.targetSquare()].data){
            scores[i] = SCORE_COUNTER;
        }
        // Quiet Moves
        else{
            int total = historyTotal[board.turn][move.fromSquare()][move.targetSquare()];
            int success = historySuccess[board.turn][move.fromSquare()][move.targetSquare()];
            scores[i] = (total > 0) ? static_cast<int>((static_cast<int64_t>(success) * SCALE_FACTOR) / total) : 0;
        }
    }
}