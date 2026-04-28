#pragma once

#include <string>
#include <vector>

struct CompletedGameEntry {
    std::string date;
    std::string gameId;
    std::string winner;
    int winnerScore = 0;
    int winnerCash = 0;
    int rounds = 0;
    std::string players;
    std::string mode;
};

bool appendCompletedGameHistory(const CompletedGameEntry& entry, std::string& error);
std::vector<CompletedGameEntry> loadCompletedGameHistory(std::string& error);
void showCompletedGameHistoryScreen(bool hasColor);
