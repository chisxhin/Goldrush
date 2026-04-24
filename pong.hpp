#pragma once

#include <string>

struct PongMinigameResult {
    int playerScore;
    int cpuScore;
    bool abandoned;
};

PongMinigameResult playPongMinigame(const std::string& playerName, bool hasColor);
