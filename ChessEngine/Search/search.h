#pragma once
#include "../Evaluation/evaluation.h"
#include "../BoardRepresentation/board.h"
#include "../BoardRepresentation/movegenerator.h"
#include<bit>
#include<chrono>
#include<utility>
#include<atomic>

constexpr int INF = 32001;
constexpr int MATE_VALUE = 32000;
constexpr int MAX_PLY = 128;

enum class TTFlag : uint8_t { EXACT, LOWER_BOUND, UPPER_BOUND };

inline bool isMateScore(int score) {
    return abs(score) > MATE_VALUE - MAX_PLY;
}

struct TranspositionEntry{
    uint64_t hash; //TODO: Size this down to either 16 or 32 bits, and then also do the cache line bucket stuff
    Move bestMove;
    int16_t score;
    uint16_t age;
    uint8_t depth;
    TTFlag flag;
};

struct ProbeResult {
    bool found;       // was there a usable matching key entry
    bool cutoff;       // caller can return immediately
    int score;         // meaningful only if cutoff is true 
    Move bestMove;      // meaningful if found is true, even without cutoff for move ordering
};

class TranspositionTable{
    public:
        std::vector<TranspositionEntry> entries;

        size_t calculateTableSize(size_t megabytes);

        uint16_t currentAge = 0;

        void Store(uint64_t key, int score, int depth, Move bestMove, TTFlag flag, int ply);
        ProbeResult Probe(uint64_t key, int depth, int alpha, int beta, int ply);

        TranspositionTable(int mb){
            entries.resize(calculateTableSize(mb));
        };
};

class Searcher {
    public:
        Searcher(int ttMegabytes) : transpositionTable(ttMegabytes){}

        Move startSearch(Board& board, int timeLimitMS);

        void Stop() { stopSearch = true; }
        void Reset(){ bestMove.data = 0; nodeCount = 0; bestEval = 0; std::fill(transpositionTable.entries.begin(), transpositionTable.entries.end(), TranspositionEntry{});; transpositionTable.currentAge = 0;}

    private:
        TranspositionTable transpositionTable; 

        Move bestMove{};
        int bestEval = 0;
        Move bestMoveThisIteration{};
        int bestEvalThisIteration = 0;

        std::atomic<bool> stopSearch = false;
        long long nodeCount = 0;
        std::chrono::steady_clock::time_point deadline;

        int Quiescence(Board& board, int alpha, int beta, int ply);
        int Search(Board& board, int alpha, int beta, int depth, int ply);
};