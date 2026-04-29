#pragma once

#include <string>

//Input: none 
//Output: holds three fields describing the minigame outcome
//Purpose: encapsulates the result of a Battleship minigame run
//Relation: returned by playBattleshipMinigame func to report how the player performed
struct BattleshipMinigameResult {
    int shipsDestroyed;
    bool clearedWave;
    bool abandoned;
};

//Input: playerName (string), hasColor (bool)
//Output: BattleshipMinigameResult struct containing shipsDestroyed, clearedWave, abandoned
//Purpose: runs the Battleship minigame loop, processes gameplay logic, and returns a summary of the player’s performance
//Relation: produces and returns a BattleshipMinigameResult, which ties directly to the struct above; 
//          internally interacts with drawing functions, input handling, and game state updates defined in the implementation file
BattleshipMinigameResult playBattleshipMinigame(const std::string& playerName, bool hasColor);
