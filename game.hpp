#pragma once

#include <ncurses.h>
#include <string>
#include <vector>

#include "bank.hpp"
#include "board.hpp"
#include "cards.hpp"
#include "history.hpp"
#include "player.hpp"
#include "rules.hpp"

class Game {
public:
    Game();
    ~Game();

    bool run();

private:
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
    DeckManager decks;
    Bank bank;
    ActionHistory history;
    WINDOW* titleWin;
    WINDOW* boardWin;
    WINDOW* infoWin;
    WINDOW* msgWin;
    bool hasColor;
    int retiredCount;

    bool ensureMinSize() const;
    void createWindows();
    void destroyWindows();
    void waitForEnter(WINDOW* w, int y, int x, const std::string& text) const;
    void applyWindowBg(WINDOW* w) const;
    void addHistory(const std::string& entry);
    void drawSetupTitle() const;
    void flashSpinResult(const std::string& title, int value) const;

    bool showStartScreen();
    bool configureCustomRules();
    void showTutorial();
    void showControlsPopup() const;
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
    int playActionCard(const Tile& tile, Player& player);
    void applyTileEffect(int playerIndex, const Tile& tile);
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
