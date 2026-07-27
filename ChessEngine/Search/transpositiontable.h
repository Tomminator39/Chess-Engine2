#pragma once
#include <cstdint>
#include <vector>
#include "../BoardRepresentation/board.h"
#include "../Helpers/MoveUtility.h"

enum class TTFlag : uint8_t { EXACT, LOWER_BOUND, UPPER_BOUND };

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
        void Reset();

        TranspositionTable(int mb){
            entries.resize(calculateTableSize(mb));
        };
};