#pragma once
#include <vector>
#include <string>
#include <ncurses.h>
#include "Board.hpp"
#include "Player.hpp"
#include "Deck.hpp"

class Game {
    Board board;
    std::vector<Player> players;
    int currentPlayer = 0;

    Deck chanceCards;
    Deck careerCards;
    Deck houseCards;

    WINDOW* boardWin = nullptr;
    WINDOW* msgWin = nullptr;
    WINDOW* ctrlWin = nullptr;
    WINDOW* pWins[4] = {nullptr, nullptr, nullptr, nullptr};

    bool quitRequested = false;

public:
    Game();
    ~Game();

    void showTitleScreen();
    void setupPlayers();
    void run();

    // Manual actions from input shortcuts
    void drawCareerCard();
    void drawHouseCard();
    void buyInsurance();

private:
    void initWindows();
    void destroyWindows();

    void render();
    void renderPlayers();
    void renderControls(const std::string& line1, const std::string& line2);

    void processTurn();
    int spin();
    void movePlayer(Player& p, int spaces);

    void handleSpace(Player& p);
    void handleCareer(Player& p);
    void handleHouse(Player& p);
    void handleChance(Player& p);
    void handleMarriage(Player& p);
    void handleBaby(Player& p);
    void handleInsurance(Player& p);
    void handleStock(Player& p);
    void handleJobLoss(Player& p);

    bool checkGameOver();
    void showWinner();

    void showMessage(const std::string& text, int colorPair = 5);
    bool confirmYesNo(const std::string& question);

    void ensureCash(Player& p, int cost);
};
