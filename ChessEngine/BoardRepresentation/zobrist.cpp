#include "zobrist.h"

uint64_t allZobristKeys[781];
uint64_t seed = 30092007ULL;

void InitZobristKeys(){
    std::mt19937_64 generator(seed);

    for(int i = 0; i < 781; i++){
        allZobristKeys[i] = generator();
    }
}