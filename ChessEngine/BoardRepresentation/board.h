#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>

enum Color{WHITE, BLACK};

enum PieceType{PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

const char pieceChars[2][6] = {
    {'P', 'N', 'B', 'R', 'Q', 'K'},  // white
    {'p', 'n', 'b', 'r', 'q', 'k'}   // black
};

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

struct Move {
    uint16_t data;

    constexpr Move() : data(0) {}

    constexpr Move(int from, int to, uint8_t flags) 
        : data((from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0x0F) << 12)) {}

    constexpr int fromSquare() const   { return data & 0x3F; }
    constexpr int targetSquare() const { return (data >> 6) & 0x3F; }
    constexpr uint8_t flags() const    { return (data >> 12) & 0x0F; }

    constexpr void setFromSquare(int from) {
        data = (data & ~0x3F) | (from & 0x3F);
    }

    constexpr void setTargetSquare(int to) {
        data = (data & ~(0x3F << 6)) | ((to & 0x3F) << 6);
    }

    constexpr void setFlags(uint8_t flags) {
        data = (data & 0x0FFF) | ((flags & 0x0F) << 12);
    }

    constexpr bool isCapture() const   { return (flags() & 4) != 0; }
    constexpr bool isPromotion() const { return (flags() & 8) != 0; }
};

constexpr uint8_t WHITE_KINGSIDE  = 1;
constexpr uint8_t WHITE_QUEENSIDE = 2;
constexpr uint8_t BLACK_KINGSIDE  = 4;
constexpr uint8_t BLACK_QUEENSIDE = 8;

constexpr uint8_t FLAG_QUIET             = 0;
constexpr uint8_t FLAG_CASTLING          = 1;
constexpr uint8_t FLAG_CAPTURE           = 4;
constexpr uint8_t FLAG_EN_PASSANT        = 5;

constexpr uint8_t FLAG_PROMOTION_KNIGHT  = 8;
constexpr uint8_t FLAG_PROMOTION_BISHOP  = 9;
constexpr uint8_t FLAG_PROMOTION_ROOK    = 10;
constexpr uint8_t FLAG_PROMOTION_QUEEN   = 11;

constexpr uint8_t FLAG_PROM_CAPT_KNIGHT  = 12;
constexpr uint8_t FLAG_PROM_CAPT_BISHOP  = 13;
constexpr uint8_t FLAG_PROM_CAPT_ROOK    = 14;
constexpr uint8_t FLAG_PROM_CAPT_QUEEN   = 15;

class Board{
    public:
     struct BoardState {
            uint8_t castlingRights;
            int8_t enPassantSquare;
            int halfMoveCounter;
            int8_t capturedPiece;
        };

    private:
        PieceType fenCharToPiece(char fenChar);
        BoardState history[1024];
        int ply = 0;

    public:
        uint64_t pieceBB[2][6] = {}; // Bitboards for move gen
        uint64_t occupancy[3] = {}; // All white pieces, all black pieces, and all occupied squares.

        int8_t mailbox[64]; // Mailbox for piece lookup

        Color turn = WHITE;

        uint8_t castlingRights = 0;

        int halfMoveCounter = 0;
        int fullMoveCounter = 0;
        
        int8_t enPassantSquare = -1;

        void LoadPositionFromFen(const std::string& fen);

        void makeMove(Move move);
        void unmakeMove(Move move);
};

inline Color mailboxColor(int entry) { return static_cast<Color>(entry / 6); }
inline PieceType mailboxType(int entry) { return static_cast<PieceType>(entry % 6); }

void printBoard(const Board& board);