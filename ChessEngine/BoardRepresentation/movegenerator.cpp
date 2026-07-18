#include "movegenerator.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstdlib>

int directionCompass[] = {8, -8, -1, 1, 7, -7, 9, -9};
static int squaresToEdge[64][8];

static uint64_t knightAttacks[64];
static uint64_t kingAttacks[64];
static uint64_t pawnAttacks[2][64]; // [color][square]

static PieceType slidingPieces[] = {ROOK, BISHOP, QUEEN};
static PieceType promotionPieces[] = {KNIGHT, BISHOP, ROOK, QUEEN};
static int knightMoves[] = {15, 17, -15, -17, 6, 10, -6, -10};

void PrecomputeMoveData(){
    for(int file = 0; file < 8; file++){
        for(int rank = 0; rank < 8; rank++){
            int numNorth = 7 - rank;
            int numSouth = rank;
            int numWest = file;
            int numEast = 7 - file;

            int squareIndex = rank * 8 + file;

            squaresToEdge[squareIndex][0] = numNorth;
            squaresToEdge[squareIndex][1] = numSouth;
            squaresToEdge[squareIndex][2] = numWest;
            squaresToEdge[squareIndex][3] = numEast;
            squaresToEdge[squareIndex][4] = std::min(numNorth, numWest);
            squaresToEdge[squareIndex][5] = std::min(numSouth, numEast);
            squaresToEdge[squareIndex][6] = std::min(numNorth, numEast);
            squaresToEdge[squareIndex][7] = std::min(numSouth, numWest);
        }
    }

    static const int knightOffsets[8][2] = {
        {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}
    };
    static const int kingOffsets[8][2] = {
        {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
    };

    for(int square = 0; square < 64; square++){
        int file = square % 8;
        int rank = square / 8;

        knightAttacks[square] = 0ULL;
        for(auto& off : knightOffsets){
            int f = file + off[0];
            int r = rank + off[1];
            if(f >= 0 && f < 8 && r >= 0 && r < 8)
                knightAttacks[square] |= (1ULL << (r * 8 + f));
        }

        kingAttacks[square] = 0ULL;
        for(auto& off : kingOffsets){
            int f = file + off[0];
            int r = rank + off[1];
            if(f >= 0 && f < 8 && r >= 0 && r < 8)
                kingAttacks[square] |= (1ULL << (r * 8 + f));
        }

        pawnAttacks[WHITE][square] = 0ULL;
        pawnAttacks[BLACK][square] = 0ULL;
        if(file - 1 >= 0 && rank + 1 < 8) pawnAttacks[WHITE][square] |= (1ULL << (square + 7));
        if(file + 1 < 8  && rank + 1 < 8) pawnAttacks[WHITE][square] |= (1ULL << (square + 9));
        if(file - 1 >= 0 && rank - 1 >= 0) pawnAttacks[BLACK][square] |= (1ULL << (square - 9));
        if(file + 1 < 8  && rank - 1 >= 0) pawnAttacks[BLACK][square] |= (1ULL << (square - 7));
    }
}

void generateSlidingMoves(const Board& board, MoveList& moveList, int startSquare, PieceType type){
    int startDirIndex = (type == BISHOP) ? 4 : 0;
    int endDirIndex = (type == ROOK) ? 4 : 8;

    for(int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++){
        int maxSteps = squaresToEdge[startSquare][directionIndex];
        for(int n = 0; n < maxSteps; n++){
            int targetSquare = startSquare + directionCompass[directionIndex] * (n + 1);
            int8_t pieceOnTargetSquare = board.mailbox[targetSquare];

            if(pieceOnTargetSquare == -1){
                Move move = {startSquare, targetSquare, FLAG_QUIET};
                moveList.moves[moveList.count++] = move;
                continue; // keep sliding 
            }

            if(mailboxColor(pieceOnTargetSquare) == board.turn) break; // blocked by friendly

            Move move = {startSquare, targetSquare, FLAG_CAPTURE};
            moveList.moves[moveList.count++] = move;
            break;
        }
    }
}

void generatePawnMoves(const Board& board, MoveList& moveList, int startSquare){
    int direction = (board.turn == WHITE) ? 8 : -8;
    int captureSquares[] = {startSquare + direction + 1, startSquare + direction - 1};
    int startRank = (board.turn == WHITE) ? 1 : 6;
    int promotionRank = (board.turn == WHITE) ? 7 : 0;

    Move move;
    move.setFromSquare(startSquare);

    for(int square : captureSquares){
        if(abs(startSquare % 8 - square % 8) != 1) continue; // prevent edge wrapping
        if(square < 0 || square >= 64) continue;
        
        int possiblePiece = board.mailbox[square];
        int possiblePieceColor = mailboxColor(possiblePiece);
        int possiblePieceType = mailboxType(possiblePiece);
        if(possiblePiece != -1 && possiblePieceColor != board.turn){
            move.setTargetSquare(square);

            if (move.targetSquare() / 8 == promotionRank) {
                bool isCap = (board.occupancy[!board.turn] & (1ULL << move.targetSquare())) != 0;
                uint8_t baseFlag = isCap ? FLAG_PROM_CAPT_KNIGHT : FLAG_PROMOTION_KNIGHT;

                // Grab a pointer to where the next 4 moves will go
                Move* dst = &moveList.moves[moveList.count];

                // Compute the moves directly into their final memory slots
                move.setFlags(baseFlag);     dst[0] = move; // Knight
                move.setFlags(baseFlag + 1); dst[1] = move; // Bishop
                move.setFlags(baseFlag + 2); dst[2] = move; // Rook
                move.setFlags(baseFlag + 3); dst[3] = move; // Queen

                // Increment the count all at once at the very end
                moveList.count += 4;
            }
            else{
                move.setFlags(FLAG_CAPTURE);
                moveList.moves[moveList.count++] = move;
            }
        }

        if(board.enPassantSquare == square && square >= 0){
            move.setTargetSquare(square);
            move.setFlags(FLAG_EN_PASSANT);
            moveList.moves[moveList.count++] = move;
        }
    }

    if(board.mailbox[startSquare + direction] == -1){
        int targetSquare = startSquare + direction;

        if (promotionRank == targetSquare / 8) {
            move.setTargetSquare(targetSquare);

            // 2. Grab a pointer to the destination in the move list
            Move* dst = &moveList.moves[moveList.count];

            // 3. Directly write the 4 promotion moves using sequential flags
            move.setFlags(FLAG_PROMOTION_KNIGHT); dst[0] = move;
            move.setFlags(FLAG_PROMOTION_BISHOP); dst[1] = move;
            move.setFlags(FLAG_PROMOTION_ROOK);   dst[2] = move;
            move.setFlags(FLAG_PROMOTION_QUEEN);  dst[3] = move;

            // 4. Update the count once
            moveList.count += 4;
        } else {
            // Standard quiet single-push pawn move
            move.setTargetSquare(targetSquare);
            move.setFlags(FLAG_QUIET);
            moveList.moves[moveList.count++] = move;
        }
    

        if(board.mailbox[startSquare + direction*2] == -1 && startRank == startSquare / 8){
            move.setTargetSquare(startSquare + direction*2);
            move.setFlags(FLAG_QUIET);
            moveList.moves[moveList.count++] = move;
        }
    }
}

void generateKnightMoves(const Board& board, MoveList& moveList, int startSquare){
    uint64_t attacks = knightAttacks[startSquare] & ~board.occupancy[board.turn];
    while(attacks){
        int targetSquare = __builtin_ctzll(attacks);
        attacks &= attacks - 1;

        uint8_t flag = (board.occupancy[!board.turn] & (1ULL << targetSquare)) ? FLAG_CAPTURE : FLAG_QUIET;
        moveList.moves[moveList.count++] = Move(startSquare, targetSquare, flag);
    }
}

void generateKingMoves(const Board& board, MoveList& moveList, int startSquare){
    uint64_t attacks = kingAttacks[startSquare] & ~board.occupancy[board.turn];
    while(attacks){
        int targetSquare = __builtin_ctzll(attacks);
        attacks &= attacks - 1;

        uint8_t flag = (board.occupancy[!board.turn] & (1ULL << targetSquare)) ? FLAG_CAPTURE : FLAG_QUIET;
        moveList.moves[moveList.count++] = Move(startSquare, targetSquare, flag);
    }
}

bool isSquareAttacked(const Board& board, int testedSquare, Color attackedSide){
    // Sliding Pieces (bishop/rook/queen):
    for(int directionIndex = 0; directionIndex < 8; directionIndex ++){
        for(int n = 0; n < squaresToEdge[testedSquare][directionIndex]; n++){
            int targetSquare = testedSquare + directionCompass[directionIndex] * (n + 1);
            int pieceOnTargetSquare = board.mailbox[targetSquare];

            if(pieceOnTargetSquare == -1) continue;

            int pieceOnTargetSquareType = mailboxType(pieceOnTargetSquare);
            Color pieceColor = mailboxColor(pieceOnTargetSquare);

            if(pieceColor == attackedSide) break;

            if(directionIndex >= 4){
                if(pieceOnTargetSquareType == BISHOP || pieceOnTargetSquareType == QUEEN) return true;
            } else {
                if(pieceOnTargetSquareType == ROOK || pieceOnTargetSquareType == QUEEN) return true;
            }
            break; // any piece, friendly or not, blocks the ray past this point
        }
    }

    // Knight
    if(knightAttacks[testedSquare] & board.pieceBB[!attackedSide][KNIGHT]) return true;

    // King
    if(kingAttacks[testedSquare] & board.pieceBB[!attackedSide][KING]) return true;

    // Pawns
    if(pawnAttacks[attackedSide][testedSquare] & board.pieceBB[!attackedSide][PAWN]) return true;

    return false;
}

void generateCastlingMoves(const Board& board, MoveList& moveList){
    auto addCastle = [&](int from, int to, uint8_t right, 
                     std::initializer_list<int> emptySquares,
                     std::initializer_list<int> safeSquares){
    if(!(board.castlingRights & right)) return;
    for(int sq : emptySquares)
        if(board.mailbox[sq] != -1) return;
    for(int sq : safeSquares)
        if(isSquareAttacked(board, sq, board.turn)) return;
    Move move = {from, to, FLAG_CASTLING};
    moveList.moves[moveList.count++] = move;
};

if(board.turn == WHITE){
    addCastle(E1, G1, WHITE_KINGSIDE,  {F1, G1},    {E1, F1, G1});
    addCastle(E1, C1, WHITE_QUEENSIDE, {B1, C1, D1}, {E1, D1, C1});
} else {
    addCastle(E8, G8, BLACK_KINGSIDE,  {F8, G8},    {E8, F8, G8});
    addCastle(E8, C8, BLACK_QUEENSIDE, {B8, C8, D8}, {E8, D8, C8});
}
}

bool isInCheck(const Board& board, Color side){
    int kingSquare = __builtin_ctzll(board.pieceBB[side][KING]);
    return isSquareAttacked(board, kingSquare, side);
}

CheckInfo getCheckInfo(Board& board){
    CheckInfo info;
    Color attackedSide = board.turn;
    int kingSquare = __builtin_ctzll(board.pieceBB[attackedSide][KING]);

    for(int directionIndex = 0; directionIndex < 8; directionIndex ++){
        int friendlySquare = -1;

        for(int n = 0; n < squaresToEdge[kingSquare][directionIndex]; n++){
            int targetSquare = kingSquare + directionCompass[directionIndex] * (n + 1);
            int pieceOnTargetSquare = board.mailbox[targetSquare];

            if(pieceOnTargetSquare == -1) continue;

            int pieceOnTargetSquareType = mailboxType(pieceOnTargetSquare);
            Color pieceColor = mailboxColor(pieceOnTargetSquare);

            bool isMatchingAttacker = false;
            if(directionIndex >= 4){
                if(pieceOnTargetSquareType == BISHOP || pieceOnTargetSquareType == QUEEN) {
                    isMatchingAttacker = true;
                }
            } else {
                if(pieceOnTargetSquareType == ROOK || pieceOnTargetSquareType == QUEEN) {
                    isMatchingAttacker = true;
                }
            }

            if(friendlySquare == -1) {
                if(pieceColor != attackedSide) {
                    if(isMatchingAttacker) {
                        info.checkers |= (1ULL << targetSquare);
                    }
                    break;
                } else {
                    friendlySquare = targetSquare;
                }
            } 
            else {
                if(pieceColor != attackedSide) {
                    if(isMatchingAttacker) {
                        info.pinned |= (1ULL << friendlySquare);
                    }
                    break;
                } else {
                    break;
                }
            }
        }
    }

    if(knightAttacks[kingSquare] & board.pieceBB[!attackedSide][KNIGHT]) {
        info.checkers |= (knightAttacks[kingSquare] & board.pieceBB[!attackedSide][KNIGHT]);
    }

    if(kingAttacks[kingSquare] & board.pieceBB[!attackedSide][KING]) {
        info.checkers |= (kingAttacks[kingSquare] & board.pieceBB[!attackedSide][KING]);
    }

    if(pawnAttacks[attackedSide][kingSquare] & board.pieceBB[!attackedSide][PAWN]) {
        info.checkers |= (pawnAttacks[attackedSide][kingSquare] & board.pieceBB[!attackedSide][PAWN]);
    }

    info.inCheck = (info.checkers != 0);

    return info;
}

MoveList generateMoves(Board& board){
    MoveList pseudoLegal;

    for(PieceType pieceType : slidingPieces){ // for each sliding piece (rook/bishop/queen)
        uint64_t bb = board.pieceBB[board.turn][pieceType];
        while(bb){
            int square = __builtin_ctzll(bb);
            generateSlidingMoves(board, pseudoLegal, square, pieceType);
            bb &= bb - 1;
        }
    }

    uint64_t pawns = board.pieceBB[board.turn][PAWN];
    while(pawns){
        int square = __builtin_ctzll(pawns);
        generatePawnMoves(board, pseudoLegal, square);
        pawns &= pawns - 1;
    }

    uint64_t knights = board.pieceBB[board.turn][KNIGHT];
    while(knights){
        int square = __builtin_ctzll(knights);
        generateKnightMoves(board, pseudoLegal, square);
        knights &= knights - 1;
    }

    uint64_t kings = board.pieceBB[board.turn][KING];
    if(kings){
        int square = __builtin_ctzll(kings);
        generateKingMoves(board, pseudoLegal, square);
    }

    generateCastlingMoves(board, pseudoLegal);

    return pseudoLegal;
}

long long Perft(int depth, Board& board){
    if(depth == 0){
        return 1ULL;
    }

    MoveList moves = generateMoves(board);
    long long nodes = 0;

    for(int i = 0; i < moves.count; i++){
        Color mover = board.turn;

        board.makeMove(moves.moves[i]);
        if(!isInCheck(board, mover)){
            nodes += Perft(depth - 1, board);
        }
        board.unmakeMove(moves.moves[i]);
    }
    
    return nodes;
}

void RunPerftTest(Board& board, int depth){
    std::cout << "\nDepth | Nodes Checked | Total Time Taken\n";
    std::cout << "-----------------------------------------\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int d = 1; d <= depth; d++) {
        long long nodes = Perft(d, board);

        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = currentTime - startTime;

        // Print the stats for this depth
        std::cout << "  " << d << "   | "
                  << nodes << " | " 
                  << elapsed.count() << " ms\n";
    }
}