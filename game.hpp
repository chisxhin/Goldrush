#pragma once

#include <ncurses.h>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#include "bank.hpp"
#include "board.hpp"
#include "cards.hpp"
#include "history.hpp"
#include "player.hpp"
#include "random_service.hpp"
#include "rules.hpp"

class SaveManager;
struct SaveFileInfo;

class Game {
public:
    Game();
    explicit Game(std::uint32_t seed);
    ~Game();

    bool run();

private:
    friend class SaveManager;

    enum StartChoice {
        START_NEW_GAME,
        START_LOAD_GAME,
        START_QUIT_GAME
    };

    static const int MIN_W = 122;
    static const int MIN_H = 40;
    static const int TITLE_W = 122;
    static const int TITLE_H = 8;
    static const int BOARD_W = 82;
    static const int BOARD_H = 28;
    static const int INFO_W = 40;
    static const int INFO_H = 28;
    static const int MSG_W = 122;
    static const int MSG_H = 4;

    Board board;
    std::vector<Player> players;
    RuleSet rules;
    RandomService rng;
    DeckManager decks;
    Bank bank;
    ActionHistory history;
    WINDOW* titleWin;
    WINDOW* boardWin;
    WINDOW* infoWin;
    WINDOW* msgWin;
    bool hasColor;
    int retiredCount;
    int currentPlayerIndex;
    int turnCounter;
    std::string gameId;
    std::string assignedSaveFilename;
    std::time_t createdTime;
    std::time_t lastSavedTime;

    bool ensureMinSize() const;
    void createWindows();
    void destroyWindows();
    void waitForEnter(WINDOW* w, int y, int x, const std::string& text) const;
    void applyWindowBg(WINDOW* w) const;
    void addHistory(const std::string& entry);
    void drawSetupTitle() const;
    void flashSpinResult(const std::string& title, int value) const;
    bool promptForFilename(const std::string& action,
                           const std::string& defaultName,
                           std::string& filename);
    bool chooseSaveFileToLoad(SaveFileInfo& selected);
    bool saveCurrentGame();
    bool loadSavedGame();

    StartChoice showStartScreen();
    bool configureCustomRules();
    void showTutorial();
    void showControlsPopup() const;
    void showScoreboardPopup() const;
    void showTileGuidePopup() const;
    void setupRules();
    void setupPlayers();
    void setupInvestments();
    int waitForTurnCommand(int currentPlayer);
    void renderGame(int currentPlayer, const std::string& msg, const std::string& detail) const;
    void renderHeader() const;
    int rollSpinner(const std::string& title, const std::string& detail);
    void showInfoPopup(const std::string& line1, const std::string& line2) const;
    int showBranchPopup(const std::string& title,
                        const std::vector<std::string>& lines,
                        char a,
                        char b);
    void playBlackTileMinigame(int playerIndex);
    int playActionCard(int playerIndex, const Tile& tile);
    void applyTileEffect(int playerIndex, const Tile& tile);
    int findPreviousTile(const Player& player, int tileId) const;
    std::string movePlayerByAction(int playerIndex, int steps);
    std::string applyActionEffect(int playerIndex,
                                  const Tile& tile,
                                  const ActionEffect& effect,
                                  int& amountDelta);
    void chooseCareer(Player& player, bool requiresDegree);
    void resolveFamilyStop(Player& player);
    void resolveNightSchool(Player& player);
    void resolveMarriageStop(Player& player);
    void resolveBabyStop(Player& player, const Tile& tile);
    void resolveSafeRoute(Player& player);
    void resolveRiskyRoute(Player& player);
    void resolveRetirement(int playerIndex);
    void buyHouse(Player& player);
    void assignInvestment(Player& player);
    void resolveInvestmentPayouts(int spinnerValue);
    void maybeAwardSpinToWin(Player& player, int spinnerValue);
    void maybeAwardPetCard(Player& player, const std::string& reason);
    int chooseNextTile(Player& player, const Tile& tile);
    bool animateMove(int currentPlayer, int steps);
    void takeMovementSpin(int currentPlayer, const std::string& reason);
    bool allPlayersRetired() const;
    void finalizeScoring();
    int calculateFinalWorth(const Player& player) const;

    int minRewardForTier(int tier) const;
    int maxRewardForTier(int tier) const;
};
