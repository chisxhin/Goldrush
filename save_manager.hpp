#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

class Game;

struct SaveFileInfo {
    int saveVersion;
    std::string filename;
    std::string path;
    std::string gameId;
    std::string assignedFilename;
    std::string createdText;
    std::string modifiedText;
    std::string lastSavedText;
    std::uintmax_t sizeBytes;
    std::time_t createdTime;
    std::time_t modifiedTime;
    std::time_t lastSavedTime;
    bool metadataValid;
    bool duplicateGameId;
    bool assignedTarget;
};

class SaveManager {
public:
    static const int SAVE_VERSION = 7;

    bool saveGame(const Game& game, const std::string& filename, std::string& error) const;
    bool loadGame(Game& game, const std::string& filename, std::string& error) const;
    bool saveExists(const std::string& filename) const;
    bool ensureSaveDirectory(std::string& error) const;
    std::vector<SaveFileInfo> listSaveFiles(std::string& error) const;
    bool archiveDuplicateSaves(const std::string& gameId,
                               const std::string& keepFilename,
                               int& archivedCount,
                               std::string& error) const;
    std::string defaultSaveFilename() const;
    std::string generateGameId() const;
    std::string normalizeFilename(const std::string& filename) const;
    std::string resolvePath(const std::string& filename) const;
};
