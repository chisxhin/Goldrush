#include "Game.hpp"
#include <chrono>
#include <thread>
#include <sstream>
#include <cstdlib>
#include <ctime>

Game::Game() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    board.loadBoard();
    chanceCards.loadFromFile("chanceCards.txt", CardType::CHANCE);
    careerCards.loadFromFile("careers.txt", CardType::CAREER);
    houseCards.loadFromFile("houses.txt", CardType::HOUSE);

    chanceCards.shuffle();
    careerCards.shuffle();
    houseCards.shuffle();
}

Game::~Game() {
    destroyWindows();
}

void Game::showTitleScreen() {
    clear();
    mvprintw(3, 10, "THE GAME OF LIFE (Terminal Edition)");
    mvprintw(5, 10, "Press any key to start...");
    refresh();
    getch();
}

void Game::setupPlayers() {
    echo();
    curs_set(1);
    clear();
    mvprintw(3, 5, "Enter number of players (2-4): ");
    refresh();

    int n = 0;
    while (n < 2 || n > 4) {
        char buf[4] = {0};
        getnstr(buf, 3);
        n = atoi(buf);
        if (n < 2 || n > 4) {
            mvprintw(5, 5, "Invalid. Enter 2-4: ");
            refresh();
        }
    }

    players.clear();
    for (int i = 0; i < n; ++i) {
        char name[32] = {0};
        mvprintw(7 + i, 5, "Player %d name: ", i + 1);
        refresh();
        getnstr(name, 31);
        Player p;
        p.name = name;
        p.cash = 10000;
        p.position = 0;
        p.colorPair = (i == 0 ? 1 : i == 1 ? 2 : i == 2 ? 6 : 7);
        players.push_back(p);
    }

    noecho();
    curs_set(0);
    clear();
    refresh();

    // Initialize windows before using msgWin in confirmYesNo/showMessage.
    initWindows();

    for (auto& p : players) {
        bool college = confirmYesNo(p.name + ": Go to college? (Y/N)");
        if (college) {
            p.hasCollege = true;
            p.takeLoan(20000);
            handleCareer(p);
        } else {
            p.hasCollege = false;
            handleCareer(p);
        }
    }
}

void Game::run() {
    initWindows();
    render();

    while (!quitRequested) {
        processTurn();
        if (checkGameOver()) break;
        currentPlayer = (currentPlayer + 1) % static_cast<int>(players.size());
    }

    showWinner();
}

void Game::initWindows() {
    if (boardWin || msgWin || ctrlWin) return;
    int h, w;
    getmaxyx(stdscr, h, w);

    if (h < 45 || w < 100) {
        clear();
        mvprintw(2, 2, "Terminal too small. Minimum 100x45.");
        mvprintw(4, 2, "Resize and restart.");
        refresh();
        getch();
        return;
    }

    boardWin = newwin(20, 78, 1, 2);
    msgWin = newwin(4, 78, 22, 2);
    ctrlWin = newwin(3, 78, 27, 2);

    pWins[0] = newwin(7, 38, 31, 2);
    pWins[1] = newwin(7, 38, 31, 42);
    pWins[2] = newwin(7, 38, 38, 2);
    pWins[3] = newwin(7, 38, 38, 42);
}

void Game::destroyWindows() {
    if (boardWin) delwin(boardWin);
    if (msgWin) delwin(msgWin);
    if (ctrlWin) delwin(ctrlWin);
    for (auto& w : pWins) {
        if (w) delwin(w);
    }
}

void Game::render() {
    if (!boardWin) return;
    board.display(boardWin, players);
    renderPlayers();
    renderControls("[SPACE] Spin | [C] Career | [H] House | [I] Insurance", "[Q] Quit");
    wrefresh(boardWin);
    wrefresh(msgWin);
    wrefresh(ctrlWin);
}

void Game::renderPlayers() {
    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        WINDOW* win = pWins[i];
        if (!win) continue;
        werase(win);
        box(win, 0, 0);
        auto& p = players[i];

        if (has_colors()) wattron(win, COLOR_PAIR(p.colorPair));
        mvwprintw(win, 1, 2, "Player %d", i + 1);
        if (has_colors()) wattroff(win, COLOR_PAIR(p.colorPair));

        mvwprintw(win, 2, 2, "Name: %s", p.name.c_str());
        mvwprintw(win, 3, 2, "Cash: $%d  Loans: $%d", p.cash, p.loanAmount);
        mvwprintw(win, 4, 2, "Career: %s ($%d)", p.career.c_str(), p.salary);
        mvwprintw(win, 5, 2, "Married: %s  Kids: %d",
                  p.isMarried ? "Yes" : "No", p.children);
        mvwprintw(win, 6, 2, "House: %s  Insurance: %s",
                  p.hasHouse ? "Yes" : "No",
                  p.hasInsurance ? "Yes" : "No");

        wrefresh(win);
    }
}

void Game::renderControls(const std::string& line1, const std::string& line2) {
    werase(ctrlWin);
    box(ctrlWin, 0, 0);
    mvwprintw(ctrlWin, 1, 2, "%s  |  %s", line1.c_str(), line2.c_str());
    wrefresh(ctrlWin);
}

void Game::processTurn() {
    Player& p = players[currentPlayer];

    std::ostringstream os;
    os << p.name << "'s turn. Press SPACE to spin.";
    showMessage(os.str());

    int ch;
    while (true) {
        ch = getch();
        if (ch == ' ') break;
        if (ch == 'c' || ch == 'C') {
            drawCareerCard();
            render();
            showMessage(os.str());
        }
        if (ch == 'h' || ch == 'H') {
            drawHouseCard();
            render();
            showMessage(os.str());
        }
        if (ch == 'i' || ch == 'I') {
            buyInsurance();
            render();
            showMessage(os.str());
        }
        if (ch == 'q' || ch == 'Q') {
            if (confirmYesNo("Quit game? (Y/N)")) {
                quitRequested = true;
                return;
            } else {
                showMessage(os.str());
            }
        }
    }

    int result = spin();
    movePlayer(p, result);
    handleSpace(p);

    render();
}

int Game::spin() {
    int final = 1;
    for (int i = 0; i < 20; ++i) {
        final = (rand() % 10) + 1;
        std::ostringstream os;
        char fx = "&#@$%*"[rand() % 6];
        os << "Spinning: [" << final << "] " << fx << " " << fx;
        showMessage(os.str());
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }
    return final;
}

void Game::movePlayer(Player& p, int spaces) {
    for (int i = 0; i < spaces; ++i) {
        int nextPos = (p.position + 1) % board.size();
        p.position = nextPos;

        Space s = board.getSpace(p.position);
        if (s.type == SpaceType::PAYDAY) {
            p.payday();
            showMessage(p.name + " passed PAYDAY. +$" + std::to_string(p.salary),
                        3);
        }

        board.display(boardWin, players);
        renderPlayers();
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }
}

void Game::handleSpace(Player& p) {
    Space s = board.getSpace(p.position);

    switch (s.type) {
        case SpaceType::START:
            showMessage("Back to START.");
            break;
        case SpaceType::COLLEGE:
            if (!p.hasCollege) {
                p.hasCollege = true;
                p.takeLoan(20000);
                showMessage("College! Loan $20000.", 3);
                handleCareer(p);
            } else {
                showMessage("Already graduated.");
            }
            break;
        case SpaceType::CAREER:
            handleCareer(p);
            break;
        case SpaceType::MARRIAGE:
            handleMarriage(p);
            break;
        case SpaceType::HOUSE:
            handleHouse(p);
            break;
        case SpaceType::BABY:
            handleBaby(p);
            break;
        case SpaceType::STOCK:
            handleStock(p);
            break;
        case SpaceType::INSURANCE:
            handleInsurance(p);
            break;
        case SpaceType::PAYDAY:
            p.payday();
            showMessage("PAYDAY! +$" + std::to_string(p.salary), 3);
            break;
        case SpaceType::TAX_RETURN:
            p.cash += 5000;
            showMessage("Tax return +$5000", 3);
            break;
        case SpaceType::VACATION:
            ensureCash(p, 3000);
            p.cash -= 3000;
            showMessage("Vacation -$3000", 4);
            break;
        case SpaceType::LAWSUIT:
            ensureCash(p, 5000);
            p.cash -= 5000;
            showMessage("Lawsuit -$5000", 4);
            break;
        case SpaceType::JOB_LOSS:
            handleJobLoss(p);
            break;
        case SpaceType::RETIREMENT:
            p.retired = true;
            showMessage("Retired! Wait for others.", 3);
            break;
        case SpaceType::CHANCE:
            handleChance(p);
            break;
        default:
            break;
    }
}

void Game::handleCareer(Player& p) {
    Card card = careerCards.drawCard();
    if (card.requiresDegree && !p.hasCollege) {
        for (int i = 0; i < 5; ++i) {
            Card c2 = careerCards.drawCard();
            if (!c2.requiresDegree) { card = c2; break; }
        }
    }
    p.career = card.title;
    p.salary = card.salary;
    showMessage("Career: " + p.career + " ($" + std::to_string(p.salary) + ")", 3);
}

void Game::handleHouse(Player& p) {
    Card house = houseCards.drawCard();
    std::ostringstream os;
    os << "House: " << house.title << " Price $" << house.price << " Value $" << house.value;
    showMessage(os.str(), 3);
    if (!p.hasHouse && confirmYesNo("Buy this house? (Y/N)")) {
        ensureCash(p, house.price);
        p.cash -= house.price;
        p.hasHouse = true;
        p.houseValue = house.value;
        showMessage("House purchased.");
    } else {
        showMessage("Skipped house.");
    }
}

void Game::handleChance(Player& p) {
    Card c = chanceCards.drawCard();
    if (c.amount >= 0) {
        p.cash += c.amount;
        showMessage(c.description + " +$" + std::to_string(c.amount), 3);
    } else {
        ensureCash(p, -c.amount);
        p.cash += c.amount;
        showMessage(c.description + " $" + std::to_string(c.amount), 4);
    }
}

void Game::handleMarriage(Player& p) {
    if (!p.isMarried) {
        ensureCash(p, 10000);
        p.cash -= 10000;
        p.isMarried = true;
        showMessage("Married! Wedding -$10000", 3);
    } else {
        showMessage("Already married.");
    }
}

void Game::handleBaby(Player& p) {
    p.children += 1;
    p.cash += 5000;
    showMessage("Baby! +$5000", 3);
}

void Game::handleInsurance(Player& p) {
    if (!p.hasInsurance) {
        if (confirmYesNo("Buy insurance for $5000? (Y/N)")) {
            ensureCash(p, 5000);
            p.cash -= 5000;
            p.hasInsurance = true;
            showMessage("Insurance purchased.");
        } else {
            showMessage("Skipped insurance.");
        }
    } else {
        showMessage("Already insured.");
    }
}

void Game::handleStock(Player& p) {
    if (confirmYesNo("Buy stock for $10000? (Y/N)")) {
        ensureCash(p, 10000);
        p.cash -= 10000;
        p.stocks.push_back("BlueChip");
        showMessage("Bought stock.");
    } else {
        showMessage("Skipped stock.");
    }
}

void Game::handleJobLoss(Player& p) {
    p.career = "Unemployed";
    p.salary = 0;
    showMessage("Job loss! Draw new career.");
    handleCareer(p);
}

bool Game::checkGameOver() {
    for (const auto& p : players) {
        if (!p.retired) return false;
    }
    return true;
}

void Game::showWinner() {
    int bestIdx = 0;
    int bestWorth = players[0].calculateNetWorth();
    for (int i = 1; i < static_cast<int>(players.size()); ++i) {
        int worth = players[i].calculateNetWorth();
        if (worth > bestWorth) {
            bestWorth = worth;
            bestIdx = i;
        }
    }
    showMessage("Winner: " + players[bestIdx].name + " ($" + std::to_string(bestWorth) + ")");
    getch();
}

void Game::showMessage(const std::string& text, int colorPair) {
    if (!msgWin) {
        clear();
        if (has_colors()) attron(COLOR_PAIR(colorPair));
        mvprintw(2, 2, "%s", text.c_str());
        if (has_colors()) attroff(COLOR_PAIR(colorPair));
        mvprintw(4, 2, "Press any key to continue...");
        refresh();
        getch();
        return;
    }
    werase(msgWin);
    box(msgWin, 0, 0);
    if (has_colors()) wattron(msgWin, COLOR_PAIR(colorPair));
    mvwprintw(msgWin, 1, 2, "%s", text.c_str());
    if (has_colors()) wattroff(msgWin, COLOR_PAIR(colorPair));
    mvwprintw(msgWin, 2, 2, "Press any key to continue...");
    wrefresh(msgWin);
    getch();
}

bool Game::confirmYesNo(const std::string& question) {
    if (!msgWin) {
        clear();
        mvprintw(2, 2, "%s", question.c_str());
        mvprintw(4, 2, "Y/N");
        refresh();
        while (true) {
            int ch = getch();
            if (ch == 'y' || ch == 'Y') return true;
            if (ch == 'n' || ch == 'N') return false;
        }
    }
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s", question.c_str());
    mvwprintw(msgWin, 2, 2, "Y/N");
    wrefresh(msgWin);
    while (true) {
        int ch = getch();
        if (ch == 'y' || ch == 'Y') return true;
        if (ch == 'n' || ch == 'N') return false;
    }
}

void Game::ensureCash(Player& p, int cost) {
    if (p.cash >= cost) return;
    int needed = cost - p.cash;
    int loan = ((needed + 999) / 1000) * 1000;
    p.takeLoan(loan);
    showMessage("Loan taken: $" + std::to_string(loan), 4);
}

void Game::drawCareerCard() {
    if (players.empty()) return;
    handleCareer(players[currentPlayer]);
}

void Game::drawHouseCard() {
    if (players.empty()) return;
    handleHouse(players[currentPlayer]);
}

void Game::buyInsurance() {
    if (players.empty()) return;
    handleInsurance(players[currentPlayer]);
}
