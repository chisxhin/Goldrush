#pragma once

#include <string>

struct MinesweeperResult {
    int safeTilesRevealed;
    bool hitBomb;
    bool timedOut;
    bool abandoned;
};

MinesweeperResult playMinesweeperMinigame(const std::string& playerName, bool hasColor);
