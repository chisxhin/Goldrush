#pragma once

#include <string>

//Input: none 
//Output: holds the outcome of a Minesweeper minigame run
//Purpose: encapsulates the player’s performance and game state
//Relation: returned by playMinesweeperMinigame() to report results
struct MinesweeperResult {
    int safeTilesRevealed;
    bool hitBomb;
    bool timedOut;
    bool abandoned;
};

//Input: playerName (string), hasColor (bool)
//Output: MinesweeperResult struct (safeTilesRevealed, hitBomb, timedOut, abandoned)
//Purpose: runs the full Minesweeper minigame loop, shows tutorial, initializes grid, handles input (movement, reveal, quit), updates game state (safe tiles, bombs, timer), renders UI (grid, feedback, timer, status)
//Relation: main entry point for the minigame, integrates tutorial display, grid creation, reveal logic, feedback banners, and result reporting (helper functions)
MinesweeperResult playMinesweeperMinigame(const std::string& playerName, bool hasColor);
