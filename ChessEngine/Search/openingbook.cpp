#include "openingbook.h"
#include <sstream>
#include <cmath>
#include <algorithm>

OpeningBook::OpeningBook(const std::string& fileContent) {
    rng.seed(std::random_device{}()); // Initialize random seed

    std::stringstream ss(fileContent);
    std::string line;
    std::string currentFen = "";
    std::vector<BookMove> currentMoves;

    while (std::getline(ss, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        //line starting with "pos"
        if (line.rfind("pos", 0) == 0) {
            if (!currentFen.empty()) {
                movesByPosition[currentFen] = currentMoves;
                currentMoves.clear();
            }
            currentFen = line.substr(3); // Extract FEN
            currentFen.erase(0, currentFen.find_first_not_of(" \t"));
        } else {
            std::stringstream lineStream(line);
            std::string moveStr;
            int count;
            if (lineStream >> moveStr >> count) {
                currentMoves.push_back({moveStr, count});
            }
        }
    }

    if (!currentFen.empty()) {
        movesByPosition[currentFen] = currentMoves;
    }
}

std::string OpeningBook::RemoveMoveCountersFromFEN(const std::string& fen) const {
    size_t pos1 = fen.find_last_of(' ');
    if (pos1 == std::string::npos) return fen;
    size_t pos2 = fen.find_last_of(' ', pos1 - 1);
    if (pos2 == std::string::npos) return fen;
    return fen.substr(0, pos2);
}

bool OpeningBook::TryGetBookMove(const std::string& currentFen, std::string& outMoveString, double weightPow) const {
    weightPow = std::clamp(weightPow, 0.0, 1.0);
    std::string keyFen = RemoveMoveCountersFromFEN(currentFen);

    auto it = movesByPosition.find(keyFen);
    if (it == movesByPosition.end() || it->second.empty()) {
        outMoveString = "Null";
        return false;
    }

    const auto& moves = it->second;

    auto weightedPlayCount = [weightPow](int playCount) {
        return static_cast<int>(std::ceil(std::pow(playCount, weightPow)));
    };

    int totalPlayCount = 0;
    for (const auto& move : moves) {
        totalPlayCount += weightedPlayCount(move.numTimesPlayed);
    }

    std::vector<double> probCumul(moves.size(), 0.0);
    double cumulativeSum = 0.0;

    for (size_t i = 0; i < moves.size(); ++i) {
        double weight = static_cast<double>(weightedPlayCount(moves[i].numTimesPlayed)) / totalPlayCount;
        cumulativeSum += weight;
        probCumul[i] = cumulativeSum;
    }

    std::uniform_real_distribution<double> dist(0.0, cumulativeSum);
    double randomVal = dist(rng);

    for (size_t i = 0; i < moves.size(); ++i) {
        if (randomVal <= probCumul[i]) {
            outMoveString = moves[i].moveString;
            return true;
        }
    }

    outMoveString = moves.back().moveString;
    return true;
}

std::string loadFileToString(const std::string& filePath){
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file at " << filePath << std::endl;
        return "";
    }

    std::cout << "Successfully opened file: " << filePath << std::endl;
    
    std::stringstream buffer;
    buffer << file.rdbuf(); // Read whole file buffer into stream
    return buffer.str();    // Convert stream to string
}