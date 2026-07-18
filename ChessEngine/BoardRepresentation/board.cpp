#include "board.h"
#include <sstream>
#include <iostream>

static constexpr uint8_t castlingMask[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11
};

PieceType Board::fenCharToPiece(char fenChar){
    switch(fenChar){
        case 'p': return PAWN;
        case 'n': return KNIGHT;
        case 'b': return BISHOP;
        case 'r': return ROOK;
        case 'q': return QUEEN;
        case 'k': return KING;
        default: return PAWN;
    }
}

void Board::LoadPositionFromFen(const std::string& fen){
    std::istringstream ss(fen);
    std::string pieceField, sideField, castlingField, enPassantField;

    ss >> pieceField >> sideField >> castlingField >> enPassantField >> halfMoveCounter >> fullMoveCounter;
    
    int file = 0;
    int rank = 7;

    std::fill(mailbox, mailbox + 64, -1);
    
    for (char c : pieceField){
        if (c == '/'){
            rank--;
            file = 0;
        } else if (isdigit(c)){
            file += (c - '0');
        } else {
            Color piece_color = (isupper(c)) ? WHITE : BLACK;
            PieceType piece_type = fenCharToPiece(tolower(c));
            int piece_square = rank * 8 + file;
            
            pieceBB[piece_color][piece_type] |= (1ULL << piece_square);

            mailbox[piece_square] = piece_color * 6 + piece_type;

            file ++;
        }
    }

    for (int piece = 0; piece < 6; piece++){
        occupancy[WHITE] |= pieceBB[WHITE][piece];
        occupancy[BLACK] |= pieceBB[BLACK][piece];
    }

    occupancy[2] = occupancy[WHITE] | occupancy[BLACK];

    turn = (sideField == "w") ? WHITE : BLACK;

    for (char c : castlingField){
        switch(c){
            case 'K': castlingRights |= WHITE_KINGSIDE; break;
            case 'Q': castlingRights |= WHITE_QUEENSIDE; break;
            case 'k': castlingRights |= BLACK_KINGSIDE; break;
            case 'q': castlingRights |= BLACK_QUEENSIDE; break;
        }
    }
    if (enPassantField == "-"){
        enPassantSquare = -1;
        } else {
        int ep_file = enPassantField[0] - 'a';
        int ep_rank = enPassantField[1] - '1';
        enPassantSquare = ep_rank * 8 + ep_file;
    }
}

void Board::makeMove(Move move){
    int8_t captured;

    if(move.flags() == FLAG_EN_PASSANT){
        uint8_t captured_pawn_square = (turn == WHITE) ? move.targetSquare() - 8 : move.targetSquare() + 8;
        captured = mailbox[captured_pawn_square];
    }
    else{
        captured = mailbox[move.targetSquare()];
    }

    history[ply++] = {castlingRights, enPassantSquare, halfMoveCounter, captured};

    uint8_t movedEntry = mailbox[move.fromSquare()];
    Color movedColor = mailboxColor(movedEntry);
    PieceType movedType = mailboxType(movedEntry);
    pieceBB[movedColor][movedType] ^= (1ULL << move.fromSquare()) | (1ULL << move.targetSquare());
    mailbox[move.targetSquare()] = movedEntry;
    mailbox[move.fromSquare()] = -1;

    uint8_t capturedColor = (captured != -1) ? mailboxColor(captured) : 0;
    uint8_t capturedType = (captured != -1) ? mailboxType(captured) : 0;

    uint8_t flags = move.flags();
    switch(flags){
        case FLAG_QUIET: break;
        case FLAG_CAPTURE:
            pieceBB[capturedColor][capturedType] ^= (1ULL << move.targetSquare());
            occupancy[2] &= ~(1ULL << move.targetSquare());
            occupancy[capturedColor] ^= (1ULL << move.targetSquare()); break;
        case FLAG_EN_PASSANT: { int capturedPawnSquare = (turn == WHITE) ? move.targetSquare() - 8 : move.targetSquare() + 8;
            pieceBB[capturedColor][capturedType] ^= (1ULL << capturedPawnSquare);
            occupancy[2] &= ~(1ULL << capturedPawnSquare);
            occupancy[capturedColor] ^= (1ULL << capturedPawnSquare);
            mailbox[capturedPawnSquare] = -1;
            enPassantSquare = -1; break; }
        case FLAG_CASTLING: 
            if(movedColor == WHITE){
                if(move.targetSquare() == G1){
                    pieceBB[WHITE][ROOK] ^= (1ULL << H1) | (1ULL << F1);
                    occupancy[2] ^= (1ULL << H1) | (1ULL << F1);
                    occupancy[WHITE] ^= (1ULL << H1) | (1ULL << F1);
                    mailbox[H1] = -1;
                    mailbox[F1] = WHITE * 6 + ROOK;
                }
                else{
                    pieceBB[WHITE][ROOK] ^= (1ULL << A1) | (1ULL << D1);
                    occupancy[2] ^= (1ULL << A1) | (1ULL << D1);
                    occupancy[WHITE] ^= (1ULL << A1) | (1ULL << D1);
                    mailbox[A1] = -1;
                    mailbox[D1] = WHITE * 6 + ROOK;
                }
            }
            else{
                if(move.targetSquare() == G8){
                    pieceBB[BLACK][ROOK] ^= (1ULL << H8) | (1ULL << F8);
                    occupancy[2] ^= (1ULL << H8) | (1ULL << F8);
                    occupancy[BLACK] ^= (1ULL << H8) | (1ULL << F8);
                    mailbox[H8] = -1;
                    mailbox[F8] = BLACK * 6 + ROOK;
                }
                else{
                    pieceBB[BLACK][ROOK] ^= (1ULL << A8) | (1ULL << D8);
                    occupancy[2] ^= (1ULL << A8) | (1ULL << D8);
                    occupancy[BLACK] ^= (1ULL << A8) | (1ULL << D8);
                    mailbox[A8] = -1;
                    mailbox[D8] = BLACK * 6 + ROOK;
                }
            } break;
        case FLAG_PROMOTION_KNIGHT:
        case FLAG_PROMOTION_BISHOP:
        case FLAG_PROMOTION_ROOK:
        case FLAG_PROMOTION_QUEEN: {
            // Map the flag back to PieceType enum (Knight=0, Bishop=1, Rook=2, Queen=3)
            // Since FLAG_PROMOTION_KNIGHT is 8, subtracting 8 gives 0, 9 gives 1, etc.
            PieceType promPiece = static_cast<PieceType>(flags - FLAG_PROMOTION_KNIGHT + 1);

            pieceBB[movedColor][PAWN] ^= (1ULL << move.targetSquare());
            pieceBB[movedColor][promPiece] ^= (1ULL << move.targetSquare());
            mailbox[move.targetSquare()] = movedColor * 6 + promPiece; 
            break;
        }
        case FLAG_PROM_CAPT_KNIGHT:
        case FLAG_PROM_CAPT_BISHOP:
        case FLAG_PROM_CAPT_ROOK:
        case FLAG_PROM_CAPT_QUEEN: {    
            // Since FLAG_PROM_CAPT_KNIGHT is 12, subtracting 12 gives you the piece type
            PieceType promPiece = static_cast<PieceType>(flags - FLAG_PROM_CAPT_KNIGHT + 1);

            pieceBB[movedColor][PAWN] ^= (1ULL << move.targetSquare());
            pieceBB[movedColor][promPiece] ^= (1ULL << move.targetSquare());
            pieceBB[capturedColor][capturedType] ^= (1ULL << move.targetSquare());
            occupancy[2] &= ~(1ULL << move.targetSquare());
            occupancy[capturedColor] ^= (1ULL << move.targetSquare());
            mailbox[move.targetSquare()] = movedColor * 6 + promPiece; 
            break;
        }
    }

    occupancy[2] ^= (1ULL << move.fromSquare()) | (1ULL << move.targetSquare());
    occupancy[movedColor] ^= (1ULL << move.fromSquare()) | (1ULL << move.targetSquare());

    if(movedType == PAWN || move.flags() == FLAG_CAPTURE || (move.isCapture() && move.isPromotion())) halfMoveCounter = 0;
    else halfMoveCounter++;

    if(movedType == PAWN && move.targetSquare() - move.fromSquare() == 16){
        enPassantSquare = move.targetSquare() - 8;
        } else if(movedType == PAWN && move.fromSquare() - move.targetSquare() == 16){
        enPassantSquare = move.targetSquare() + 8;
    } else {
        enPassantSquare = -1;
    }

    castlingRights &= castlingMask[move.fromSquare()];
    castlingRights &= castlingMask[move.targetSquare()];

    turn = (turn == WHITE) ? BLACK : WHITE;
}

void Board::unmakeMove(Move move){
    ply--;
    castlingRights = history[ply].castlingRights;
    enPassantSquare = history[ply].enPassantSquare;
    halfMoveCounter = history[ply].halfMoveCounter;
    int8_t captured = history[ply].capturedPiece;

    int movedEntry = mailbox[move.targetSquare()];
    Color movedColor = mailboxColor(movedEntry);
    PieceType movedType = mailboxType(movedEntry);
    uint8_t flags = move.flags();

    // Check if the move was a promotion (flags >= 8)
    if (flags >= FLAG_PROMOTION_KNIGHT) {
        // Since we are unmaking, the piece currently on targetSquare is the promotion piece.
        // We clear the promotion piece and restore the pawn bitboard.
        pieceBB[movedColor][movedType] ^= (1ULL << move.targetSquare());
        pieceBB[movedColor][PAWN] ^= (1ULL << move.fromSquare());
    } else {
        pieceBB[movedColor][movedType] ^= (1ULL << move.fromSquare()) | (1ULL << move.targetSquare());
    }

    int capturedColor = (captured != -1) ? mailboxColor(captured) : 0;
    int capturedType = (captured != -1) ? mailboxType(captured) : 0;

    switch(flags){
        case FLAG_QUIET:
            mailbox[move.fromSquare()] = movedEntry;
            mailbox[move.targetSquare()] = -1;
            break;

        case FLAG_CAPTURE:
            pieceBB[capturedColor][capturedType] ^= (1ULL << move.targetSquare());
            occupancy[2] |= (1ULL << move.targetSquare());
            occupancy[capturedColor] ^= (1ULL << move.targetSquare());
            mailbox[move.fromSquare()] = movedEntry;
            mailbox[move.targetSquare()] = captured;
            break;

        case FLAG_EN_PASSANT: {
            int capturedPawnSquare = (movedColor == WHITE) ? move.targetSquare() - 8 : move.targetSquare() + 8;
            pieceBB[capturedColor][capturedType] ^= (1ULL << capturedPawnSquare);
            occupancy[2] |= (1ULL << capturedPawnSquare);
            occupancy[capturedColor] ^= (1ULL << capturedPawnSquare);
            mailbox[capturedPawnSquare] = captured;
            mailbox[move.fromSquare()] = movedEntry;
            mailbox[move.targetSquare()] = -1;
            break;
        }

        case FLAG_CASTLING:
            if(movedColor == WHITE){
                if(move.targetSquare() == G1){
                    pieceBB[WHITE][ROOK] ^= (1ULL << H1) | (1ULL << F1);
                    occupancy[2] ^= (1ULL << H1) | (1ULL << F1);
                    occupancy[WHITE] ^= (1ULL << H1) | (1ULL << F1);
                    mailbox[H1] = WHITE * 6 + ROOK;
                    mailbox[F1] = -1;
                }
                else{
                    pieceBB[WHITE][ROOK] ^= (1ULL << A1) | (1ULL << D1);
                    occupancy[2] ^= (1ULL << A1) | (1ULL << D1);
                    occupancy[WHITE] ^= (1ULL << A1) | (1ULL << D1);
                    mailbox[A1] = WHITE * 6 + ROOK;
                    mailbox[D1] = -1;
                }
            }
            else{
                if(move.targetSquare() == G8){
                    pieceBB[BLACK][ROOK] ^= (1ULL << H8) | (1ULL << F8);
                    occupancy[2] ^= (1ULL << H8) | (1ULL << F8);
                    occupancy[BLACK] ^= (1ULL << H8) | (1ULL << F8);
                    mailbox[H8] = BLACK * 6 + ROOK;
                    mailbox[F8] = -1;
                }
                else{
                    pieceBB[BLACK][ROOK] ^= (1ULL << A8) | (1ULL << D8);
                    occupancy[2] ^= (1ULL << A8) | (1ULL << D8);
                    occupancy[BLACK] ^= (1ULL << A8) | (1ULL << D8);
                    mailbox[A8] = BLACK * 6 + ROOK;
                    mailbox[D8] = -1;
                }
            }
            mailbox[move.fromSquare()] = movedEntry;
            mailbox[move.targetSquare()] = -1;
            break;

        case FLAG_PROMOTION_KNIGHT:
        case FLAG_PROMOTION_BISHOP:
        case FLAG_PROMOTION_ROOK:
        case FLAG_PROMOTION_QUEEN:
            mailbox[move.fromSquare()] = movedColor * 6 + PAWN;
            mailbox[move.targetSquare()] = -1;
            break;

        case FLAG_PROM_CAPT_KNIGHT:
        case FLAG_PROM_CAPT_BISHOP:
        case FLAG_PROM_CAPT_ROOK:
        case FLAG_PROM_CAPT_QUEEN:
            pieceBB[capturedColor][capturedType] ^= (1ULL << move.targetSquare());
            occupancy[2] |= (1ULL << move.targetSquare());
            occupancy[capturedColor] ^= (1ULL << move.targetSquare());
            mailbox[move.fromSquare()] = movedColor * 6 + PAWN;
            mailbox[move.targetSquare()] = captured;
            break;
    }

    occupancy[2] ^= (1ULL << move.fromSquare()) | (1ULL << move.targetSquare());
    occupancy[movedColor] ^= (1ULL << move.fromSquare()) | (1ULL << move.targetSquare());

    turn = (turn == WHITE) ? BLACK : WHITE;
}

void printBoard(const Board& board){
    for(int guiRank = 7; guiRank >= 0; guiRank--){
        std::cout << "\n";
        for(int guiFile = 0; guiFile < 8; guiFile++){
            int guiSquare = guiRank * 8 + guiFile;
            if (!(board.occupancy[2] & (1ULL << guiSquare))){
                std::cout << ".";
            }
            else{
                int printPiece = board.mailbox[guiSquare];
                std::cout << pieceChars[mailboxColor(printPiece)][mailboxType(printPiece)];
            }
        }
    }
}