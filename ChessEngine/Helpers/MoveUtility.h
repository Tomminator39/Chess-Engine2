#pragma once
#include<string>
#include<cmath>

constexpr int INF = 32001;
constexpr int MATE_VALUE = 32000;
constexpr int MAX_PLY = 128;

inline bool isMateScore(int score) {
    return abs(score) > MATE_VALUE - MAX_PLY;
}