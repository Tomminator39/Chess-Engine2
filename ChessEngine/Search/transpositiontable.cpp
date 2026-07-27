#include"transpositiontable.h"

void TranspositionTable::Reset(){
    std::fill(entries.begin(), entries.end(), TranspositionEntry{});; 
    currentAge = 0;
}

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