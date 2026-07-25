#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

struct BookMove {
    std::string moveString;
    int numTimesPlayed;
};

class OpeningBook {
private:
    std::unordered_map<std::string, std::vector<BookMove>> movesByPosition;
    mutable std::mt19937 rng;

    // Helper function to strip halfmove clock and fullmove number from FEN
    std::string RemoveMoveCountersFromFEN(const std::string& fen) const;

public:
    OpeningBook(const std::string& fileContent);

    bool TryGetBookMove(const std::string& currentFen, std::string& outMoveString, double weightPow = 0.5) const;
};

std::string loadFileToString(const std::string& filePath);