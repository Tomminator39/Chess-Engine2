#pragma once
#include "../Helpers/MoveUtility.h"
#include "../BoardRepresentation/board.h"
#include "../BoardRepresentation/movegenerator.h"
#include "../Evaluation/evaluation.h"

constexpr int SCORE_PRIORITY      = 1000000; // TT/PV move
constexpr int SCORE_WINNING_CAPT  =   200000; // SEE >= 0, offset by SEE
constexpr int SCORE_PROMOTION     =   175000; // Quiet queen promotion
constexpr int SCORE_KILLER        =   150000;
constexpr int SCORE_COUNTER       =   100000;
constexpr int SCORE_QUIET         =        0; // + history
constexpr int SCORE_LOSING_CAPT   =  -100000; // SEE < 0, offset by SEE

constexpr int SCALE_FACTOR = 12000;

class MoveOrderer {
    private:
        Move killerMoves[MAX_PLY][2];
        int historySuccess[2][64][64]; // History heuristic
        int historyTotal[2][64][64]; // Butterfly heuristic
        Move counterMove[2][64][64];

    public:
        bool IsKiller(Move move, int ply);
        void RecordAttempt(Color color, Move move);
        void RecordCounter(Move previousMove, Move counterMove, Color color);
        void DecayHistory();
        int GetMVVLVA(const Board& board, Move move);
        void RecordCutoff(Move move, int ply, int depth, Color color);
        int SEE(const Board& board, Move move);
        void ScoreMoves(const Board& board, MoveList& moves, Move priorityMove, int* scores, int ply, Move previousMove);
        void Reset();
};

int selectBestMoveIndex(int* scores, int i, int count);