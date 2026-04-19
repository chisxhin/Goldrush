#include <ncurses.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

static const int MIN_W = 80;
static const int MIN_H = 36;

static const int TITLE_W = 80;
static const int TITLE_H = 2;
static const int BOARD_W = 80;
static const int BOARD_H = 28;
static const int INFO_W = 80;
static const int INFO_H = 3;
static const int MSG_W = 80;
static const int MSG_H = 3;

static const int TILE_COUNT = 89;

enum TileKind {
    TILE_EMPTY,
    TILE_BLACK,
    TILE_START,
    TILE_SPLIT_START,
    TILE_COLLEGE,
    TILE_CAREER,
    TILE_GRADUATION,
    TILE_MARRIAGE,
    TILE_SPLIT_FAMILY,
    TILE_FAMILY,
    TILE_CAREER_2,
    TILE_PAYDAY,
    TILE_BABY,
    TILE_RETIREMENT,
    TILE_HOUSE
};

enum MiniGameKind {
    MINIGAME_RED_BLACK,
    MINIGAME_MATH,
    MINIGAME_ODD_EVEN
};

struct Tile {
    int id;
    int y;
    int x;
    std::string label;
    TileKind kind;
    int next;
    int altNext;
    int value;
};

struct Player {
    std::string name;
    char token;
    int tile;
    int cash;
    std::string job;
    int salary;
    bool married;
    int kids;
    bool hasHouse;
    int houseValue;
    bool retired;
    int startChoice;
    int familyChoice;
};

static void applyWindowBg(WINDOW* w, bool hasColor) {
    if (w && hasColor) {
        wbkgd(w, COLOR_PAIR(5));
    }
}

static bool ensureMinSize(bool hasColor) {
    int h, w;
    timeout(200);
    while (true) {
        getmaxyx(stdscr, h, w);
        if (h >= MIN_H && w >= MIN_W) {
            timeout(-1);
            return true;
        }

        if (hasColor) {
            bkgd(COLOR_PAIR(5));
        }
        clear();
        const char* line1 = "Terminal too small - please resize";
        const char* line2 = "Minimum size: 80x36";
        const char* line3 = "Press Q to quit";
        int x1 = (w - static_cast<int>(std::strlen(line1))) / 2;
        int x2 = (w - static_cast<int>(std::strlen(line2))) / 2;
        int x3 = (w - static_cast<int>(std::strlen(line3))) / 2;
        int y = h / 2;
        if (x1 < 0) x1 = 0;
        if (x2 < 0) x2 = 0;
        if (x3 < 0) x3 = 0;
        mvprintw(y - 1, x1, "%s", line1);
        mvprintw(y, x2, "%s", line2);
        mvprintw(y + 1, x3, "%s", line3);
        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            timeout(-1);
            return false;
        }
    }
}

static void destroyWindows(WINDOW*& titleWin, WINDOW*& boardWin, WINDOW*& infoWin, WINDOW*& msgWin) {
    if (titleWin) {
        delwin(titleWin);
        titleWin = nullptr;
    }
    if (boardWin) {
        delwin(boardWin);
        boardWin = nullptr;
    }
    if (infoWin) {
        delwin(infoWin);
        infoWin = nullptr;
    }
    if (msgWin) {
        delwin(msgWin);
        msgWin = nullptr;
    }
}

static void createWindows(int termH, int termW, WINDOW*& titleWin, WINDOW*& boardWin, WINDOW*& infoWin, WINDOW*& msgWin, bool hasColor) {
    int totalH = TITLE_H + BOARD_H + INFO_H + MSG_H;
    int startY = (termH - totalH) / 2;
    int startX = (termW - TITLE_W) / 2;
    if (startY < 0) startY = 0;
    if (startX < 0) startX = 0;

    titleWin = newwin(TITLE_H, TITLE_W, startY, startX);
    boardWin = newwin(BOARD_H, BOARD_W, startY + TITLE_H, startX);
    infoWin = newwin(INFO_H, INFO_W, startY + TITLE_H + BOARD_H, startX);
    msgWin = newwin(MSG_H, MSG_W, startY + TITLE_H + BOARD_H + INFO_H, startX);

    applyWindowBg(titleWin, hasColor);
    applyWindowBg(boardWin, hasColor);
    applyWindowBg(infoWin, hasColor);
    applyWindowBg(msgWin, hasColor);
}

static void waitForEnter(WINDOW* w, int y, int x, const std::string& text) {
    mvwprintw(w, y, x, "%s", text.c_str());
    wrefresh(w);
    int ch;
    do {
        ch = wgetch(w);
    } while (ch != '\n' && ch != KEY_ENTER);
}

static int rollSpinner(WINDOW* msgWin, bool hasColor) {
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "Hold SPACE to roll. Release to stop.");
    mvwprintw(msgWin, 2, 2, "Result will blink. Press ENTER to confirm.");
    wrefresh(msgWin);

    int ch;
    do {
        ch = wgetch(msgWin);
    } while (ch != ' ');

    nodelay(msgWin, TRUE);
    auto lastSpace = std::chrono::steady_clock::now();
    int value = 1;
    while (true) {
        value = (std::rand() % 10) + 1;
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "Rolling: %d", value);
        mvwprintw(msgWin, 2, 2, "Release SPACE to stop");
        wrefresh(msgWin);
        napms(80);

        ch = wgetch(msgWin);
        if (ch == ' ') {
            lastSpace = std::chrono::steady_clock::now();
        }
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - lastSpace).count();
        if (elapsed > 240) break;
    }
    nodelay(msgWin, FALSE);

    for (int i = 0; i < 4; ++i) {
        werase(msgWin);
        box(msgWin, 0, 0);
        if (hasColor) wattron(msgWin, COLOR_PAIR(4));
        mvwprintw(msgWin, 1, 2, "Rolled: %d", value);
        if (hasColor) wattroff(msgWin, COLOR_PAIR(4));
        mvwprintw(msgWin, 2, 2, "Press ENTER to confirm");
        wrefresh(msgWin);
        napms(140);

        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "Rolled: %d", value);
        mvwprintw(msgWin, 2, 2, "Press ENTER to confirm");
        wrefresh(msgWin);
        napms(140);
    }

    waitForEnter(msgWin, 2, 26, "");
    return value;
}

static int totalWorth(const Player& p) {
    int total = p.cash + p.kids * 20000;
    if (p.hasHouse) total += p.houseValue;
    return total;
}

static int minRewardForTier(int tier) {
    if (tier == 2) return 3000;
    if (tier >= 3) return 5000;
    return 1000;
}

static int maxRewardForTier(int tier) {
    if (tier == 2) return 5000;
    if (tier >= 3) return 10000;
    return 2000;
}

static char tokenForName(const std::string& name, int index) {
    if (!name.empty()) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        return c;
    }
    return static_cast<char>('A' + index);
}

static void initTiles(std::vector<Tile>& tiles) {
    tiles.resize(TILE_COUNT);
    for (int i = 0; i < TILE_COUNT; ++i) {
        tiles[i].id = i;
        tiles[i].y = 1;
        tiles[i].x = 2;
        tiles[i].label = "  ";
        tiles[i].kind = TILE_EMPTY;
        tiles[i].next = (i < TILE_COUNT - 1) ? i + 1 : -1;
        tiles[i].altNext = -1;
        tiles[i].value = 0;
    }

    for (int i = 0; i <= 11; ++i) {
        tiles[i].y = 1;
        tiles[i].x = 16 + (i * 4);
    }

    tiles[12].y = 4;
    tiles[12].x = 38;

    for (int i = 13; i <= 24; ++i) {
        tiles[i].y = 7;
        tiles[i].x = 2 + ((i - 13) * 4);
    }

    for (int i = 25; i <= 37; ++i) {
        tiles[i].y = 10;
        tiles[i].x = 24 + ((i - 25) * 4);
    }

    for (int i = 38; i <= 48; ++i) {
        tiles[i].y = 13;
        tiles[i].x = 18 + ((i - 38) * 4);
    }

    for (int i = 49; i <= 58; ++i) {
        tiles[i].y = 16;
        tiles[i].x = 20 + ((i - 49) * 4);
    }

    for (int i = 59; i <= 72; ++i) {
        tiles[i].y = 19;
        tiles[i].x = 2 + ((i - 59) * 4);
    }

    for (int i = 73; i <= 86; ++i) {
        tiles[i].y = 22;
        tiles[i].x = 22 + ((i - 73) * 4);
    }

    tiles[87].y = 25;
    tiles[87].x = 36;
    tiles[88].y = 25;
    tiles[88].x = 40;

    for (int i = 0; i <= 9; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 1;
    }
    tiles[10].label = "ST";
    tiles[10].kind = TILE_START;
    tiles[11].label = "BK";
    tiles[11].kind = TILE_BLACK;
    tiles[11].value = 1;
    tiles[12].label = "SP";
    tiles[12].kind = TILE_SPLIT_START;
    tiles[12].next = 13;
    tiles[12].altNext = 25;

    tiles[13].label = "COL";
    tiles[13].kind = TILE_COLLEGE;
    for (int i = 14; i <= 24; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = (i >= 20) ? 2 : 1;
    }
    tiles[24].next = 38;

    tiles[25].label = "CAR";
    tiles[25].kind = TILE_CAREER;
    for (int i = 26; i <= 37; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = (i >= 33) ? 2 : 1;
    }
    tiles[37].next = 38;

    tiles[38].label = "GRD";
    tiles[38].kind = TILE_GRADUATION;
    for (int i = 39; i <= 48; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[41].label = "PAY";
    tiles[41].kind = TILE_PAYDAY;
    tiles[41].value = 4000;
    tiles[44].label = "WED";
    tiles[44].kind = TILE_MARRIAGE;
    tiles[47].label = "PAY";
    tiles[47].kind = TILE_PAYDAY;
    tiles[47].value = 5000;

    for (int i = 49; i <= 57; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[58].label = "SP";
    tiles[58].kind = TILE_SPLIT_FAMILY;
    tiles[49].next = 50;
    tiles[58].next = 59;
    tiles[58].altNext = 73;

    for (int i = 59; i <= 72; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[60].label = "3B";
    tiles[60].kind = TILE_BABY;
    tiles[60].value = 3;
    tiles[64].label = "2B";
    tiles[64].kind = TILE_BABY;
    tiles[64].value = 2;
    tiles[68].label = "HSE";
    tiles[68].kind = TILE_HOUSE;
    tiles[68].value = 100000;
    tiles[72].label = "1B";
    tiles[72].kind = TILE_BABY;
    tiles[72].value = 1;
    tiles[72].next = 87;

    for (int i = 73; i <= 86; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[75].label = "PAY";
    tiles[75].kind = TILE_PAYDAY;
    tiles[75].value = 5000;
    tiles[79].label = "PRM";
    tiles[79].kind = TILE_CAREER_2;
    tiles[79].value = 3000;
    tiles[83].label = "PAY";
    tiles[83].kind = TILE_PAYDAY;
    tiles[83].value = 7000;
    tiles[86].label = "BK";
    tiles[86].value = 3;
    tiles[86].next = 87;

    tiles[87].label = "RT";
    tiles[87].kind = TILE_RETIREMENT;
    tiles[88].label = "RT";
    tiles[88].kind = TILE_RETIREMENT;
    tiles[87].next = 88;
    tiles[88].next = -1;
}

static int colorForTile(const Tile& tile) {
    if (tile.kind == TILE_BLACK) return 6;
    if (tile.kind == TILE_START) return 3;
    if (tile.kind == TILE_SPLIT_START || tile.kind == TILE_SPLIT_FAMILY) return 4;
    if (tile.kind == TILE_COLLEGE) return 2;
    if (tile.kind == TILE_CAREER || tile.kind == TILE_CAREER_2) return 1;
    if (tile.kind == TILE_PAYDAY) return 7;
    if (tile.kind == TILE_BABY) return 3;
    if (tile.kind == TILE_HOUSE) return 2;
    if (tile.kind == TILE_GRADUATION) return 7;
    if (tile.kind == TILE_MARRIAGE) return 6;
    if (tile.kind == TILE_FAMILY) return 7;
    if (tile.kind == TILE_RETIREMENT) return 3;
    return 5;
}

static void drawTreeGuides(WINDOW* boardWin) {
    mvwvline(boardWin, 2, 39, ACS_VLINE, 2);
    mvwaddch(boardWin, 4, 39, ACS_TTEE);

    mvwhline(boardWin, 6, 24, ACS_HLINE, 16);
    mvwvline(boardWin, 5, 24, ACS_VLINE, 2);
    mvwvline(boardWin, 5, 56, ACS_VLINE, 5);
    mvwaddch(boardWin, 6, 24, ACS_LTEE);
    mvwaddch(boardWin, 6, 56, ACS_RTEE);

    mvwvline(boardWin, 11, 39, ACS_VLINE, 2);
    mvwvline(boardWin, 14, 39, ACS_VLINE, 2);
    mvwaddch(boardWin, 13, 39, ACS_VLINE);

    mvwhline(boardWin, 18, 26, ACS_HLINE, 14);
    mvwvline(boardWin, 17, 26, ACS_VLINE, 2);
    mvwvline(boardWin, 17, 54, ACS_VLINE, 5);
    mvwaddch(boardWin, 18, 26, ACS_LTEE);
    mvwaddch(boardWin, 18, 54, ACS_RTEE);

    mvwvline(boardWin, 23, 39, ACS_VLINE, 2);
}

static int showBranchPopup(WINDOW* parent,
                           bool hasColor,
                           const std::string& title,
                           const std::vector<std::string>& lines,
                           char a,
                           char b) {
    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(10, 44, (h - 10) / 2, (w - 44) / 2);
    applyWindowBg(popup, hasColor);
    werase(popup);
    box(popup, 0, 0);
    if (hasColor) wattron(popup, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(popup, 1, 2, "%s", title.c_str());
    if (hasColor) wattroff(popup, COLOR_PAIR(4) | A_BOLD);
    for (size_t i = 0; i < lines.size(); ++i) {
        mvwprintw(popup, 3 + static_cast<int>(i), 2, "%s", lines[i].c_str());
    }
    mvwprintw(popup, 8, 2, "[%c] or [%c]", a, b);
    wrefresh(popup);

    int ch;
    while (true) {
        ch = wgetch(popup);
        if (ch == a || ch == a + 32) {
            delwin(popup);
            touchwin(parent);
            wrefresh(parent);
            return a;
        }
        if (ch == b || ch == b + 32) {
            delwin(popup);
            touchwin(parent);
            wrefresh(parent);
            return b;
        }
    }
}

static void renderGame(WINDOW* titleWin,
                       WINDOW* boardWin,
                       WINDOW* infoWin,
                       WINDOW* msgWin,
                       const std::vector<Tile>& tiles,
                       const std::vector<Player>& players,
                       int currentPlayer,
                       bool hasColor,
                       const std::string& msg) {
    werase(titleWin);
    if (hasColor) wattron(titleWin, COLOR_PAIR(5));
    mvwprintw(titleWin, 0, 18, "T H E   G A M E   O F   L I F E");
    mvwprintw(titleWin, 1, 2, "Early Life -> Split -> College/Career -> Marriage -> Split -> Family/Career -> Retirement");
    if (hasColor) wattroff(titleWin, COLOR_PAIR(5));
    wrefresh(titleWin);

    werase(boardWin);
    box(boardWin, 0, 0);
    drawTreeGuides(boardWin);
    for (int i = 0; i < TILE_COUNT; ++i) {
        const Tile& tile = tiles[i];
        int color = colorForTile(tile);
        mvwaddch(boardWin, tile.y, tile.x, '[');
        if (hasColor) wattron(boardWin, COLOR_PAIR(color));
        mvwprintw(boardWin, tile.y, tile.x + 1, "  ");
        if (hasColor) wattroff(boardWin, COLOR_PAIR(color));
        mvwaddch(boardWin, tile.y, tile.x + 3, ']');
        mvwprintw(boardWin, tile.y + 1, tile.x + 1, "%s", tile.label.c_str());
    }

    for (int i = 0; i < TILE_COUNT; ++i) {
        int slot = 0;
        for (size_t p = 0; p < players.size(); ++p) {
            if (players[p].tile != i) continue;
            if (slot >= 2) break;
            if (hasColor) wattron(boardWin, COLOR_PAIR(1 + static_cast<int>(p % 4)) | A_BOLD);
            mvwaddch(boardWin, tiles[i].y, tiles[i].x + 1 + slot, players[p].token);
            if (hasColor) wattroff(boardWin, COLOR_PAIR(1 + static_cast<int>(p % 4)) | A_BOLD);
            slot++;
        }
    }
    wrefresh(boardWin);

    const Player& p = players[currentPlayer];
    werase(infoWin);
    box(infoWin, 0, 0);
    if (hasColor) wattron(infoWin, COLOR_PAIR(1 + (currentPlayer % 4)) | A_BOLD);
    mvwprintw(infoWin, 1, 2, "Player: %s [%c]  Tile: %d  Cash: $%d  Job: %s",
              p.name.c_str(), p.token, p.tile, p.cash, p.job.c_str());
    if (hasColor) wattroff(infoWin, COLOR_PAIR(1 + (currentPlayer % 4)) | A_BOLD);
    mvwprintw(infoWin, 2, 2, "Salary: $%d  Married: %s  Kids: %d  House: %s  [ENTER] Roll  [Q] Quit",
              p.salary, p.married ? "Yes" : "No", p.kids, p.hasHouse ? "Yes" : "No");
    wrefresh(infoWin);

    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s", msg.c_str());
    wrefresh(msgWin);
}

static int playActionCard(WINDOW* parent, const Tile& tile, Player& p, bool hasColor) {
    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(9, 34, (h - 9) / 2, (w - 34) / 2);
    applyWindowBg(popup, hasColor);

    int minAmount = minRewardForTier(tile.value);
    int maxAmount = maxRewardForTier(tile.value);
    int amount = minAmount + (std::rand() % (maxAmount - minAmount + 1));
    MiniGameKind game = static_cast<MiniGameKind>(std::rand() % 3);
    int ch;

    if (game == MINIGAME_RED_BLACK) {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "BLACK TILE ACTION");
        mvwprintw(popup, 2, 2, "Mini Game: Red / Black");
        mvwprintw(popup, 4, 2, "Pick [R]ed or [B]lack");
        mvwprintw(popup, 6, 2, "Win or lose: $%d", amount);
        wrefresh(popup);

        char guess = 'R';
        while (true) {
            ch = wgetch(popup);
            if (ch == 'r' || ch == 'R') { guess = 'R'; break; }
            if (ch == 'b' || ch == 'B') { guess = 'B'; break; }
        }

        char result = (std::rand() % 2 == 0) ? 'R' : 'B';
        bool win = (guess == result);
        if (win) p.cash += amount;
        else p.cash -= amount;

        werase(popup);
        box(popup, 0, 0);
        if (hasColor) wattron(popup, COLOR_PAIR(win ? 3 : 6));
        mvwprintw(popup, 1, 2, "The wheel landed on %s!", result == 'R' ? "RED" : "BLACK");
        mvwprintw(popup, 3, 2, "You %s $%d", win ? "WIN" : "LOSE", amount);
        if (hasColor) wattroff(popup, COLOR_PAIR(win ? 3 : 6));
        mvwprintw(popup, 5, 2, "New cash: $%d", p.cash);
        mvwprintw(popup, 6, 2, "Press ENTER");
        wrefresh(popup);
    } else if (game == MINIGAME_MATH) {
        int a = 2 + (std::rand() % 9);
        int b = 1 + (std::rand() % 9);
        bool add = (std::rand() % 2 == 0);
        int answer = add ? (a + b) : (a - b);
        if (!add && a < b) {
            int t = a;
            a = b;
            b = t;
            answer = a - b;
        }

        echo();
        curs_set(1);
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "BLACK TILE ACTION");
        mvwprintw(popup, 2, 2, "Mini Game: Quick Math");
        mvwprintw(popup, 4, 2, "Solve: %d %c %d = ", a, add ? '+' : '-', b);
        mvwprintw(popup, 6, 2, "Win or lose: $%d", amount);
        wrefresh(popup);

        char buf[16] = {0};
        wgetnstr(popup, buf, 15);
        noecho();
        curs_set(0);
        int guess = std::atoi(buf);
        bool win = (guess == answer);
        if (win) p.cash += amount;
        else p.cash -= amount;

        werase(popup);
        box(popup, 0, 0);
        if (hasColor) wattron(popup, COLOR_PAIR(win ? 3 : 6));
        mvwprintw(popup, 1, 2, "Correct answer: %d", answer);
        mvwprintw(popup, 3, 2, "You %s $%d", win ? "WIN" : "LOSE", amount);
        if (hasColor) wattroff(popup, COLOR_PAIR(win ? 3 : 6));
        mvwprintw(popup, 5, 2, "New cash: $%d", p.cash);
        mvwprintw(popup, 6, 2, "Press ENTER");
        wrefresh(popup);
    } else {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "BLACK TILE ACTION");
        mvwprintw(popup, 2, 2, "Mini Game: Odd / Even");
        mvwprintw(popup, 4, 2, "Pick [O]dd or [E]ven");
        mvwprintw(popup, 6, 2, "Win or lose: $%d", amount);
        wrefresh(popup);

        char guess = 'O';
        while (true) {
            ch = wgetch(popup);
            if (ch == 'o' || ch == 'O') { guess = 'O'; break; }
            if (ch == 'e' || ch == 'E') { guess = 'E'; break; }
        }

        int spin = 1 + (std::rand() % 10);
        bool even = (spin % 2 == 0);
        bool win = (guess == 'E' && even) || (guess == 'O' && !even);
        if (win) p.cash += amount;
        else p.cash -= amount;

        werase(popup);
        box(popup, 0, 0);
        if (hasColor) wattron(popup, COLOR_PAIR(win ? 3 : 6));
        mvwprintw(popup, 1, 2, "Spin: %d", spin);
        mvwprintw(popup, 2, 2, "%s number!", even ? "Even" : "Odd");
        mvwprintw(popup, 3, 2, "You %s $%d", win ? "WIN" : "LOSE", amount);
        if (hasColor) wattroff(popup, COLOR_PAIR(win ? 3 : 6));
        mvwprintw(popup, 5, 2, "New cash: $%d", p.cash);
        mvwprintw(popup, 6, 2, "Press ENTER");
        wrefresh(popup);
    }

    do {
        ch = wgetch(popup);
    } while (ch != '\n' && ch != KEY_ENTER);

    delwin(popup);
    touchwin(parent);
    wrefresh(parent);
    return amount;
}

static void applyTileEffect(Player& p, const Tile& tile, WINDOW* msgWin, bool hasColor) {
    std::string line = "Nothing happens here.";

    switch (tile.kind) {
        case TILE_START:
            line = "START: The journey begins.";
            break;
        case TILE_BLACK:
            playActionCard(msgWin, tile, p, hasColor);
            return;
        case TILE_COLLEGE:
            p.cash += 10000;
            line = "COLLEGE: -$10K tuition, +$20K loan.";
            break;
        case TILE_CAREER:
            p.job = "Clerk";
            p.salary = 3000;
            line = "CAREER: You became a Clerk ($3000).";
            break;
        case TILE_GRADUATION:
            if (p.startChoice == 0) {
                p.job = "Doctor";
                p.salary = 8000;
                line = "GRADUATION: Doctor path unlocked. Salary is now $8000.";
            } else {
                p.job = "Manager";
                p.salary = 5000;
                line = "GRADUATION: Career path pays off. Salary is now $5000.";
            }
            break;
        case TILE_MARRIAGE:
            p.cash -= 5000;
            p.married = true;
            line = "MARRIAGE: -$5000 and married.";
            break;
        case TILE_FAMILY:
            p.kids += 1;
            p.cash -= 2000;
            line = "FAMILY PATH: +1 kid and -$2000.";
            break;
        case TILE_CAREER_2:
            p.salary += tile.value;
            line = "CAREER PATH: promotion! Salary increased.";
            break;
        case TILE_PAYDAY:
            p.cash += tile.value;
            line = "PAYDAY: cash increased.";
            break;
        case TILE_BABY:
            p.kids += tile.value;
            p.cash -= tile.value * 1000;
            line = "FAMILY ROAD: babies added to the family.";
            break;
        case TILE_HOUSE:
            p.cash -= 50000;
            p.hasHouse = true;
            p.houseValue = tile.value;
            line = "HOUSE: bought a house on the family road.";
            break;
        case TILE_RETIREMENT:
            if (tile.id == 88) {
                p.retired = true;
                line = "RETIREMENT: You finished the game.";
            } else {
                line = "Retirement stretch.";
            }
            break;
        case TILE_SPLIT_START:
        case TILE_SPLIT_FAMILY:
        case TILE_EMPTY:
        default:
            line = "Keep moving.";
            break;
    }

    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s", line.c_str());
    mvwprintw(msgWin, 2, 2, "Press ENTER to continue");
    wrefresh(msgWin);
    int ch;
    do {
        ch = wgetch(msgWin);
    } while (ch != '\n' && ch != KEY_ENTER);
}

static int chooseNextTile(Player& p, const Tile& tile, WINDOW* msgWin) {
    if (tile.kind == TILE_SPLIT_START && p.startChoice == -1) {
        int c = showBranchPopup(
            msgWin,
            has_colors(),
            "College or Career?",
            std::vector<std::string>{
                "A: College",
                "   Bigger debt now, stronger graduation payoff.",
                "B: Career",
                "   Start earning sooner, steadier path."
            },
            'A',
            'B');
        p.startChoice = (c == 'A') ? 0 : 1;
    }
    if (tile.kind == TILE_SPLIT_FAMILY && tile.id == 58 && p.familyChoice == -1) {
        int c = showBranchPopup(
            msgWin,
            has_colors(),
            "Family or Career?",
            std::vector<std::string>{
                "A: Family",
                "   More babies, house chances, more chaos.",
                "B: Career",
                "   More payday tiles, promotions, black cards."
            },
            'A',
            'B');
        p.familyChoice = (c == 'A') ? 0 : 1;
    }

    if (tile.kind == TILE_SPLIT_START) {
        return (p.startChoice == 0) ? tile.next : tile.altNext;
    }
    if (tile.kind == TILE_SPLIT_FAMILY && tile.id == 58) {
        return (p.familyChoice == 0) ? tile.next : tile.altNext;
    }
    return tile.next;
}

static void animateMove(WINDOW* titleWin,
                        WINDOW* boardWin,
                        WINDOW* infoWin,
                        WINDOW* msgWin,
                        const std::vector<Tile>& tiles,
                        std::vector<Player>& players,
                        int currentPlayer,
                        int steps,
                        bool hasColor) {
    Player& p = players[currentPlayer];
    for (int step = 0; step < steps; ++step) {
        const Tile& current = tiles[p.tile];
        int nextTile = chooseNextTile(p, current, msgWin);
        if (nextTile < 0) break;
        p.tile = nextTile;
        renderGame(titleWin, boardWin, infoWin, msgWin, tiles, players, currentPlayer, hasColor,
                   p.name + " moved to tile " + std::to_string(p.tile));
        napms(170);
    }
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
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLACK);
        init_pair(6, COLOR_RED, COLOR_BLACK);
        init_pair(7, COLOR_BLUE, COLOR_BLACK);
        bkgd(COLOR_PAIR(5));
    }

    if (!ensureMinSize(hasColor)) {
        endwin();
        return 0;
    }

    WINDOW* titleWin = nullptr;
    WINDOW* boardWin = nullptr;
    WINDOW* infoWin = nullptr;
    WINDOW* msgWin = nullptr;

    int termH, termW;
    getmaxyx(stdscr, termH, termW);
    createWindows(termH, termW, titleWin, boardWin, infoWin, msgWin, hasColor);

    std::vector<Tile> tiles;
    initTiles(tiles);

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
        char nameBuf[32] = {0};
        wgetnstr(msgWin, nameBuf, 31);
        Player p;
        p.name = nameBuf;
        p.token = tokenForName(p.name, i);
        p.tile = 0;
        p.cash = 10000;
        p.job = "Unemployed";
        p.salary = 0;
        p.married = false;
        p.kids = 0;
        p.hasHouse = false;
        p.houseValue = 0;
        p.retired = false;
        p.startChoice = -1;
        p.familyChoice = -1;
        players.push_back(p);
    }
    noecho();
    curs_set(0);

    int currentPlayer = 0;
    bool allRetired = false;
    while (!allRetired) {
        if (!ensureMinSize(hasColor)) {
            destroyWindows(titleWin, boardWin, infoWin, msgWin);
            endwin();
            return 0;
        }
        getmaxyx(stdscr, termH, termW);
        destroyWindows(titleWin, boardWin, infoWin, msgWin);
        createWindows(termH, termW, titleWin, boardWin, infoWin, msgWin, hasColor);

        if (players[currentPlayer].retired) {
            currentPlayer = (currentPlayer + 1) % numPlayers;
            allRetired = true;
            for (int i = 0; i < numPlayers; ++i) {
                if (!players[i].retired) allRetired = false;
            }
            continue;
        }

        renderGame(titleWin, boardWin, infoWin, msgWin, tiles, players, currentPlayer, hasColor,
                   players[currentPlayer].name + "'s turn");

