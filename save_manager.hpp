#pragma once

#include <string>

class Game;

class SaveManager {
public:
    static const int SAVE_VERSION = 1;

    bool saveGame(const Game& game, const std::string& filename, std::string& error) const;
    bool loadGame(Game& game, const std::string& filename, std::string& error) const;
    bool saveExists(const std::string& filename) const;
};
