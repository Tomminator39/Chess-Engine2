#pragma once
#include "../Helpers/MoveUtility.h"
#include "../Evaluation/evaluation.h"
#include "../BoardRepresentation/board.h"
#include "../BoardRepresentation/movegenerator.h"
#include "openingbook.h"
#include "transpositiontable.h"
#include "moveordering.h"
#include<bit>
#include<chrono>
#include<utility>
#include<atomic>
#include<iostream>
#include<string>

std::string moveToString(const Move& move);

class Searcher {
    public:
        Searcher(int ttMegabytes, const std::string& bookFilePath) : transpositionTable(ttMegabytes), openingBook(loadFileToString(bookFilePath)) {InitLMRTable();}

        Move startSearch(Board& board, int timeLimitMS);

        void Stop() { stopSearch = true; }
        void Reset(){
            bestMove.data = 0; 
            nodeCount = 0; 
            bestEval = 0; 
            transpositionTable.Reset();
            moveOrderer.Reset();
        }

    private:
        OpeningBook openingBook;

        TranspositionTable transpositionTable;

        MoveOrderer moveOrderer;

        Move bestMove{};
        int bestEval = 0;
        Move bestMoveThisIteration{};
        int bestEvalThisIteration = 0;

        std::atomic<bool> stopSearch = false;
        long long nodeCount = 0;
        std::chrono::steady_clock::time_point deadline;

        Move pvTable[MAX_PLY][MAX_PLY];
        int pvLength[MAX_PLY];
        
        int staticEvalHistory[MAX_PLY];
        int lmrTable[MAX_PLY][256]; // Precomputed reduction rate for lmr

        void InitLMRTable();

        int Quiescence(Board& board, int alpha, int beta, int ply, Move previousMove);
        int Search(Board& board, int alpha, int beta, int depth, int ply, Move previousMove, bool previousWasNullMove = false);

        void printInfoString(int depth, int score, uint64_t time_ms);
};