#pragma once

#include <string>

struct HangmanResult {
    bool won;           // Did the player guess the word?
    int attemptsLeft;   // How many tries remaining (0-10)
    int lettersGuessed; // How many letter slots were revealed
    bool abandoned;     // Did player quit (Q key)?
};

HangmanResult playHangmanMinigame(const std::string& playerName, bool hasColor);
