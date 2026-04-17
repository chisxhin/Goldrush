#include <ncurses.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

struct Player {
    std::string name;
    int node = 0;
    int cash = 10000;
    std::string job = "Unemployed";
    int salary = 0;
    bool married = false;
    int kids = 0;
    bool hasHouse = false;
    int houseValue = 0;
    bool retired = false;
    int choiceStart = -1; // 0 = College, 1 = Career
    int choiceFC = -1;    // 0 = Family, 1 = Career
    int choiceSR = -1;    // 0 = Safe, 1 = Risk
};

enum NodeId {
    START = 0,
    COLLEGE,
    CAREER,
    GRADUATION,
    WEDDING,
    BRANCH_FC,
    FAMILY_PATH,
    HOUSE,
    CAREER_PATH,
    PROMOTION,
    BRANCH_SR,
    SAFE_ROAD,
    RISK_ROAD,
    RETIREMENT,
    NODE_COUNT
};

struct Node {
    std::string name;
    int y;
    int x;
    int next;
    int altNext;
    bool isBranch;
};

static void drawHLine(WINDOW* w, int y, int x1, int x2) {
    for (int x = x1; x <= x2; ++x) mvwaddch(w, y, x, ACS_HLINE);
}

static void drawVLine(WINDOW* w, int x, int y1, int y2) {
    for (int y = y1; y <= y2; ++y) mvwaddch(w, y, x, ACS_VLINE);
}

static void waitForEnter(WINDOW* w, int y, int x, const std::string& msg) {
    mvwprintw(w, y, x, "%s", msg.c_str());
    wrefresh(w);
    flushinp();
    int ch;
    do {
        ch = wgetch(w);
    } while (ch != '\n' && ch != KEY_ENTER);
}

static int spin(WINDOW* w, bool hasColor) {
    werase(w);
    box(w, 0, 0);
    mvwprintw(w, 1, 2, "Hold SPACE to roll");
    mvwprintw(w, 2, 2, "Release to stop. Then ENTER to confirm");
    wrefresh(w);

    int ch;
    while (true) {
        ch = wgetch(w);
        if (ch == ' ') break;
    }

    nodelay(w, TRUE);
    int result = 1;
    int lastSpaceTick = 0;
    int tick = 0;

    while (true) {
        result = (result % 10) + 1;
        werase(w);
        box(w, 0, 0);
        mvwprintw(w, 1, 2, "Rolling: %d", result);
        wrefresh(w);

        napms(80);
        ch = wgetch(w);
        tick += 1;
        if (ch == ' ') {
            lastSpaceTick = tick;
        }
        if (tick - lastSpaceTick > 2) break;
    }
    nodelay(w, FALSE);

    for (int i = 0; i < 4; ++i) {
        werase(w);
        box(w, 0, 0);
        if (hasColor) wattron(w, COLOR_PAIR(6));
        mvwprintw(w, 1, 2, "Spin: %d", result);
        if (hasColor) wattroff(w, COLOR_PAIR(6));
        mvwprintw(w, 2, 2, "Press ENTER to confirm");
        wrefresh(w);
        napms(150);

        werase(w);
        box(w, 0, 0);
        mvwprintw(w, 1, 2, "Spin: %d", result);
        mvwprintw(w, 2, 2, "Press ENTER to confirm");
        wrefresh(w);
        napms(150);
    }

    int key;
    do {
        key = wgetch(w);
    } while (key != '\n' && key != KEY_ENTER);

    return result;
}

static int askChoice(WINDOW* w, const std::string& prompt, char a, char b) {
    werase(w);
    box(w, 0, 0);
    mvwprintw(w, 1, 2, "%s [%c/%c]", prompt.c_str(), a, b);
    wrefresh(w);
    int ch;
    while (true) {
        ch = wgetch(w);
        if (ch == a || ch == b) return ch;
        if (ch == a + 32 || ch == b + 32) return ch - 32;
    }
}

static void applyEffect(Player& p, int node, WINDOW* w) {
    werase(w);
    box(w, 0, 0);

    switch (node) {
        case START:
            mvwprintw(w, 1, 2, "START: Begin your journey.");
            break;
        case COLLEGE:
            p.cash -= 10000;
            p.cash += 20000;
            mvwprintw(w, 1, 2, "COLLEGE: -$10K +$20K loan.");
            break;
        case CAREER:
            p.job = "Clerk";
            p.salary = 3000;
            mvwprintw(w, 1, 2, "CAREER: Clerk ($3000 salary).");
            break;
        case GRADUATION:
            if (p.job == "Clerk") {
                p.job = "Manager";
                p.salary = 5000;
                mvwprintw(w, 1, 2, "GRADUATION: Manager ($5000).");
            } else {
                p.job = "Doctor";
                p.salary = 8000;
                mvwprintw(w, 1, 2, "GRADUATION: Doctor ($8000).");
            }
            break;
        case WEDDING:
            p.cash -= 5000;
            p.married = true;
            mvwprintw(w, 1, 2, "WEDDING: -$5000. Married = Yes.");
            break;
        case FAMILY_PATH:
            p.kids += 1;
            p.cash -= 2000;
            mvwprintw(w, 1, 2, "FAMILY PATH: +1 kid, -$2000.");
            break;
        case CAREER_PATH:
            p.salary += 5000;
            mvwprintw(w, 1, 2, "CAREER PATH: +$5000 salary.");
            break;
        case HOUSE:
            p.cash -= 50000;
            p.hasHouse = true;
            p.houseValue = 100000;
            mvwprintw(w, 1, 2, "HOUSE: -$50K, house value $100K.");
            break;
        case PROMOTION:
            p.salary += 5000;
            mvwprintw(w, 1, 2, "PROMOTION: +$5000 salary.");
            break;
        case SAFE_ROAD:
            p.cash += 3000;
            mvwprintw(w, 1, 2, "SAFE ROAD: +$3000.");
            break;
        case RISK_ROAD: {
            bool win = (std::rand() % 2) == 0;
            if (win) {
                p.cash += 15000;
                mvwprintw(w, 1, 2, "RISK ROAD: +$15000!");
            } else {
                p.cash -= 10000;
                mvwprintw(w, 1, 2, "RISK ROAD: -$10000.");
            }
            break;
        }
        case RETIREMENT:
            p.retired = true;
            mvwprintw(w, 1, 2, "RETIREMENT: You finished.");
            break;
        default:
            mvwprintw(w, 1, 2, "Nothing happens.");
            break;
    }

    wrefresh(w);
    waitForEnter(w, 2, 2, "Press ENTER to continue");
}

static int totalWorth(const Player& p) {
    int total = p.cash + (p.kids * 20000);
    if (p.hasHouse) total += p.houseValue;
    return total;
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    bool hasColor = has_colors();
    if (hasColor) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_GREEN, -1);   // Player 1
        init_pair(2, COLOR_CYAN, -1);    // Player 2
        init_pair(3, COLOR_MAGENTA, -1); // Player 3
        init_pair(4, COLOR_YELLOW, -1);  // Player 4
        init_pair(5, COLOR_WHITE, -1);   // Default text
        init_pair(6, COLOR_RED, -1);     // Risk
        init_pair(7, COLOR_BLUE, -1);    // Safe
    }

    int termH, termW;
    getmaxyx(stdscr, termH, termW);
    if (termH < 34 || termW < 80) {
        clear();
        mvprintw(2, 2, "Terminal too small. Minimum 80x34.");
        refresh();
        getch();
        endwin();
        return 1;
    }

    WINDOW* titleWin = newwin(3, 76, 1, 1);
    WINDOW* boardWin = newwin(20, 76, 4, 1);
    WINDOW* infoWin = newwin(6, 76, 25, 1);
    WINDOW* msgWin = newwin(4, 76, 31, 1);

    std::vector<Node> nodes(NODE_COUNT);
    nodes[START] = {"START", 2, 8, COLLEGE, CAREER, true};
    nodes[COLLEGE] = {"COLLEGE", 4, 8, GRADUATION, -1, false};
    nodes[CAREER] = {"CAREER", 4, 28, GRADUATION, -1, false};
    nodes[GRADUATION] = {"GRAD", 6, 18, WEDDING, -1, false};
    nodes[WEDDING] = {"WEDDING", 8, 18, BRANCH_FC, -1, false};
    nodes[BRANCH_FC] = {"BRANCH", 10, 18, FAMILY_PATH, CAREER_PATH, true};
    nodes[FAMILY_PATH] = {"FAMILY", 10, 10, HOUSE, -1, false};
    nodes[HOUSE] = {"HOUSE", 12, 10, BRANCH_SR, -1, false};
    nodes[CAREER_PATH] = {"CAREER", 10, 26, PROMOTION, -1, false};
    nodes[PROMOTION] = {"PROMO", 12, 26, BRANCH_SR, -1, false};
    nodes[BRANCH_SR] = {"BRANCH", 14, 18, SAFE_ROAD, RISK_ROAD, true};
    nodes[SAFE_ROAD] = {"SAFE", 14, 10, RETIREMENT, -1, false};
    nodes[RISK_ROAD] = {"RISK", 14, 26, RETIREMENT, -1, false};
    nodes[RETIREMENT] = {"RETIRE", 16, 18, -1, -1, false};

    // Title
    werase(titleWin);
    box(titleWin, 0, 0);
    if (hasColor) wattron(titleWin, COLOR_PAIR(5));
    mvwprintw(titleWin, 1, 22, "T H E   G A M E   O F   L I F E");
    if (hasColor) wattroff(titleWin, COLOR_PAIR(5));
    wrefresh(titleWin);

    // Start screen: ask player count and names
    echo();
    curs_set(1);
    int numPlayers = 0;
    while (numPlayers < 2 || numPlayers > 4) {
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "How many players? (2-4): ");
        wrefresh(msgWin);
        char buf[8] = {0};
        wgetnstr(msgWin, buf, 7);
        numPlayers = std::atoi(buf);
    }

    std::vector<Player> players;
    players.reserve(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "Player %d name: ", i + 1);
        wrefresh(msgWin);
        char namebuf[32] = {0};
        wgetnstr(msgWin, namebuf, 31);
        Player p;
        p.name = namebuf;
        players.push_back(p);
    }
    noecho();
    curs_set(0);

    bool allRetired = false;
    int current = 0;

    while (!allRetired) {
        Player& p = players[current];
        if (p.retired) {
            current = (current + 1) % numPlayers;
            continue;
        }

    // Draw board
        werase(boardWin);
        box(boardWin, 0, 0);

        // Draw path lines
        drawVLine(boardWin, 6, 2, 6);
        drawVLine(boardWin, 32, 2, 6);
        drawHLine(boardWin, 4, 6, 32);
        drawVLine(boardWin, 19, 6, 10);
        drawVLine(boardWin, 9, 10, 13);
        drawVLine(boardWin, 29, 10, 13);
        drawHLine(boardWin, 10, 9, 29);
        drawVLine(boardWin, 19, 12, 16);
        drawHLine(boardWin, 14, 9, 29);
        drawVLine(boardWin, 19, 15, 16);

        mvwaddch(boardWin, 4, 6, ACS_LLCORNER);
        mvwaddch(boardWin, 4, 32, ACS_LRCORNER);
        mvwaddch(boardWin, 10, 19, ACS_PLUS);
        mvwaddch(boardWin, 14, 19, ACS_PLUS);

        // Labels
        for (int i = 0; i < NODE_COUNT; ++i) {
            int y = nodes[i].y;
            int x = nodes[i].x;
            // Draw tile box (3x9)
            mvwaddch(boardWin, y - 1, x - 2, ACS_ULCORNER);
            drawHLine(boardWin, y - 1, x - 1, x + 6);
            mvwaddch(boardWin, y - 1, x + 7, ACS_URCORNER);
            drawVLine(boardWin, x - 2, y, y + 1);
            drawVLine(boardWin, x + 7, y, y + 1);
            mvwaddch(boardWin, y + 2, x - 2, ACS_LLCORNER);
            drawHLine(boardWin, y + 2, x - 1, x + 6);
            mvwaddch(boardWin, y + 2, x + 7, ACS_LRCORNER);

            if (hasColor) {
                int color = 5;
                if (i == SAFE_ROAD) color = 7;
                if (i == RISK_ROAD) color = 6;
                wattron(boardWin, COLOR_PAIR(color));
                mvwprintw(boardWin, y, x, "%s", nodes[i].name.c_str());
                wattroff(boardWin, COLOR_PAIR(color));
            } else {
                mvwprintw(boardWin, y, x, "%s", nodes[i].name.c_str());
            }
        }

        // Tokens
        for (int i = 0; i < numPlayers; ++i) {
            int ny = nodes[players[i].node].y;
            int nx = nodes[players[i].node].x + 1;
            if (hasColor) wattron(boardWin, COLOR_PAIR(1 + (i % 4)));
            mvwaddch(boardWin, ny + 1, nx + (i % 4), '1' + i);
            if (hasColor) wattroff(boardWin, COLOR_PAIR(1 + (i % 4)));
        }

        wrefresh(boardWin);

        // Info panel (current player)
        werase(infoWin);
        box(infoWin, 0, 0);
        mvwprintw(infoWin, 1, 2, "PLAYER: %s", p.name.c_str());
        mvwprintw(infoWin, 2, 2, "Cash: $%d  Job: %s  Salary: $%d", p.cash, p.job.c_str(), p.salary);
        mvwprintw(infoWin, 3, 2, "Married: %s  Kids: %d", p.married ? "Yes" : "No", p.kids);
        mvwprintw(infoWin, 4, 2, "House: %s", p.hasHouse ? "Yes" : "No");
        mvwprintw(infoWin, 5, 2, "[ENTER] Spin  |  [Q] Quit");
        wrefresh(infoWin);

        // Wait for ENTER or Q
        int ch;
        do {
            ch = wgetch(infoWin);
            if (ch == 'q' || ch == 'Q') {
                endwin();
                return 0;
            }
        } while (ch != '\n' && ch != KEY_ENTER);

        int roll = spin(msgWin, hasColor);

        // Move steps without forcing branch choice mid-move.
        for (int step = 0; step < roll; ++step) {
            int n = p.node;
            if (n == START && p.choiceStart != -1) {
                p.node = (p.choiceStart == 0) ? nodes[n].next : nodes[n].altNext;
            } else if (n == BRANCH_FC && p.choiceFC != -1) {
                p.node = (p.choiceFC == 0) ? nodes[n].next : nodes[n].altNext;
            } else if (n == BRANCH_SR && p.choiceSR != -1) {
                p.node = (p.choiceSR == 0) ? nodes[n].next : nodes[n].altNext;
            } else {
                p.node = nodes[n].next;
            }
            if (p.node < 0) break;
        }

        // If landed on a branch node, ask for the next move.
        if (p.node == START) {
            int c = askChoice(msgWin, "Choose path: [A] College / [B] Career", 'A', 'B');
            p.choiceStart = (c == 'A') ? 0 : 1;
        } else if (p.node == BRANCH_FC) {
            int c = askChoice(msgWin, "Choose path: [A] Family / [B] Career", 'A', 'B');
            p.choiceFC = (c == 'A') ? 0 : 1;
        } else if (p.node == BRANCH_SR) {
            int c = askChoice(msgWin, "Choose road: [A] Safe / [B] Risk", 'A', 'B');
            p.choiceSR = (c == 'A') ? 0 : 1;
        }

        applyEffect(p, p.node, msgWin);

        // Check retirement
        allRetired = true;
        for (const auto& pl : players) {
            if (!pl.retired) {
                allRetired = false;
                break;
            }
        }

        current = (current + 1) % numPlayers;
    }

    // Game over
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "GAME OVER!");

    int best = 0;
    int bestScore = totalWorth(players[0]);
    for (int i = 1; i < numPlayers; ++i) {
        int score = totalWorth(players[i]);
        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    mvwprintw(msgWin, 2, 2, "%s wins! Total: $%d", players[best].name.c_str(), bestScore);
    wrefresh(msgWin);
    waitForEnter(msgWin, 2, 40, "Press ENTER to exit");

    endwin();
    return 0;
}
