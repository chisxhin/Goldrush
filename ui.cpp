#include "ui.h"

#include <algorithm>
#include <cctype>
#include <clocale>
#include <utility>
#include <string>
#include <sstream>
#include <vector>

#include "tile_display.h"
#include "ui_helpers.h"

namespace {
const short UI_COLOR_GOLD = 24;
const short UI_COLOR_BROWN = 25;
const short UI_COLOR_TERRA = 26;
const short UI_COLOR_FOREST = 27;
const short UI_COLOR_STEEL = 28;
const short UI_COLOR_MAUVE = 29;

short earthyOr(short custom, short fallback) {
    if (can_change_color() && COLORS >= 32) {
        return custom;
    }
    return fallback;
}

std::string clipPanelText(const std::string& text, std::size_t width) {
    return clipUiText(text, width);
}

void drawPanelHeader(WINDOW* win, int y, const std::string& text) {
    wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    mvwprintw(win, y, 2, "%s", text.c_str());
    wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
}

void drawLabeledValue(WINDOW* win,
                      int y,
                      const std::string& label,
                      const std::string& value,
                      int valueColor = GOLDRUSH_BROWN_CREAM) {
    const int width = getmaxx(win);
    const int valueWidth = std::max(8, width - 15);
    wattron(win, COLOR_PAIR(GOLDRUSH_BROWN_SAND) | A_BOLD);
    mvwprintw(win, y, 2, "%-10s", label.c_str());
    wattroff(win, COLOR_PAIR(GOLDRUSH_BROWN_SAND) | A_BOLD);
    wattron(win, COLOR_PAIR(valueColor));
    mvwprintw(win, y, 13, "%-*s", valueWidth, clipPanelText(value, static_cast<std::size_t>(valueWidth)).c_str());
    wattroff(win, COLOR_PAIR(valueColor));
}

bool textContains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

std::string lowercaseCopy(const std::string& text) {
    std::string lowered = text;
    for (std::size_t i = 0; i < lowered.size(); ++i) {
        lowered[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lowered[i])));
    }
    return lowered;
}

std::string historyCategory(const std::string& eventText) {
    const std::string lowered = lowercaseCopy(eventText);
    if (textContains(lowered, "duel")) return "DUEL";
    if (textContains(lowered, "pong") ||
        textContains(lowered, "battleship") ||
        textContains(lowered, "hangman") ||
        textContains(lowered, "memory") ||
        textContains(lowered, "minesweeper") ||
        textContains(lowered, "minigame")) {
        return "MINIGAME";
    }
    if (textContains(lowered, "drew") ||
        textContains(lowered, "action card") ||
        textContains(lowered, "card") ||
        textContains(lowered, "sabotage")) {
        return "CARD";
    }
    if (textContains(lowered, "spun") ||
        textContains(lowered, "moved") ||
        textContains(lowered, "route") ||
        textContains(lowered, "turn")) {
        return "MOVE";
    }
    if (textContains(lowered, "won $") ||
        textContains(lowered, "lost $") ||
        textContains(lowered, "collected") ||
        textContains(lowered, "earned") ||
        textContains(lowered, "paid") ||
        textContains(lowered, "investment") ||
        textContains(lowered, "salary") ||
        textContains(lowered, "loan")) {
        return "MONEY";
    }
    return "INFO";
}

int previewNextTile(const Board& board, const Player& player, int tileIndex) {
    const Tile& current = board.tileAt(tileIndex);
    if (current.kind == TILE_SPLIT_START) {
        return player.startChoice == 0 ? current.next : current.altNext;
    }
    if (current.kind == TILE_SPLIT_FAMILY) {
        return player.familyChoice == 0 ? current.next : current.altNext;
    }
    if (current.kind == TILE_SPLIT_RISK) {
        return player.riskChoice == 0 ? current.next : current.altNext;
    }
    return current.next;
}

int distanceToNextTileKind(const Board& board, const Player& player, TileKind kind, int maxDistance) {
    int tileIndex = player.tile;
    for (int distance = 1; distance <= maxDistance; ++distance) {
        tileIndex = previewNextTile(board, player, tileIndex);
        if (tileIndex < 0) {
            return -1;
        }
        if (board.tileAt(tileIndex).kind == kind) {
            return distance;
        }
    }
    return -1;
}

void drawLegendEntry(WINDOW* win,
                     int y,
                     int x,
                     const Tile& tile,
                     const std::string& shortName) {
    const std::string symbol = "[" + getTileBoardSymbol(tile) + "]";
    wattron(win, COLOR_PAIR(getTileColorPair(tile)) | A_BOLD);
    mvwprintw(win, y, x, "%s", symbol.c_str());
    wattroff(win, COLOR_PAIR(getTileColorPair(tile)) | A_BOLD);
    mvwprintw(win, y, x + 5, "%s", shortName.c_str());
}
}  // namespace

void initGameColors() {
    start_color();
#ifdef NCURSES_VERSION
    use_default_colors();
#endif

    if (can_change_color() && COLORS >= 32) {
        init_color(UI_COLOR_GOLD, 1000, 843, 0);
        init_color(UI_COLOR_BROWN, 361, 251, 200);
        init_color(UI_COLOR_TERRA, 886, 447, 357);
        init_color(UI_COLOR_FOREST, 133, 545, 133);
        init_color(UI_COLOR_STEEL, 447, 549, 788);
        init_color(UI_COLOR_MAUVE, 694, 509, 882);
    }

    const short gold = earthyOr(UI_COLOR_GOLD, COLOR_YELLOW);
    const short brown = earthyOr(UI_COLOR_BROWN, COLOR_WHITE);
    const short terra = earthyOr(UI_COLOR_TERRA, COLOR_RED);
    const short forest = earthyOr(UI_COLOR_FOREST, COLOR_GREEN);
    const short steel = earthyOr(UI_COLOR_STEEL, COLOR_CYAN);
    const short mauve = earthyOr(UI_COLOR_MAUVE, COLOR_MAGENTA);

    init_pair(GOLDRUSH_GOLD_BLACK, gold, COLOR_BLACK);
    init_pair(GOLDRUSH_BROWN_SAND, brown, COLOR_BLACK);
    init_pair(GOLDRUSH_GOLD_FOREST, gold, COLOR_BLACK);
    init_pair(GOLDRUSH_BLACK_GOLD, gold, COLOR_BLACK);
    init_pair(GOLDRUSH_BLACK_TERRA, terra, COLOR_BLACK);
    init_pair(GOLDRUSH_BROWN_CREAM, brown, COLOR_BLACK);
    init_pair(GOLDRUSH_CHARCOAL_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(GOLDRUSH_GOLD_SAND, gold, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_ONE, gold, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_TWO, forest, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_THREE, terra, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_FOUR, COLOR_CYAN, COLOR_BLACK);
    init_pair(GOLDRUSH_BLACK_FOREST, forest, COLOR_BLACK);
    init_pair(GOLDRUSH_BLACK_CREAM, brown, COLOR_BLACK);
    init_pair(GOLDRUSH_GOLD_TERRA, terra, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_BASIC, COLOR_WHITE, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_PAYDAY, forest, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_ACTION, COLOR_CYAN, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_MINIGAME, mauve, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_RISK, terra, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_CAREER, steel, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_HOME, brown, COLOR_BLACK);
    init_pair(GOLDRUSH_TILE_ROUTE, gold, COLOR_BLACK);
}

void draw_menu_border(bool is_active, int x, int y, int width, int height) {
    if (!is_active) return;
    attron(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x + 1, ACS_HLINE, width - 2);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    for (int row = 1; row < height - 1; ++row) {
        mvaddch(y + row, x, ACS_VLINE);
        mvaddch(y + row, x + width - 1, ACS_VLINE);
    }
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
}

bool isSpecialMinimapTile(const Tile& tile) {
    switch (tile.kind) {
        case TILE_BLACK:
        case TILE_SPLIT_START:
        case TILE_SPLIT_FAMILY:
        case TILE_SPLIT_RISK:
        case TILE_PAYDAY:
        case TILE_MARRIAGE:
        case TILE_HOUSE:
        case TILE_BABY:
        case TILE_GRADUATION:
        case TILE_NIGHT_SCHOOL:
        case TILE_SPIN_AGAIN:
        case TILE_CAREER_2:
            return true;
        default:
            return false;
    }
}

void drawMinimapDot(WINDOW* panelWin, int y, int x, const char* glyph, int colorPair) {
    wattron(panelWin, COLOR_PAIR(colorPair) | A_BOLD);
    mvwprintw(panelWin, y, x, "%s", glyph);
    wattroff(panelWin, COLOR_PAIR(colorPair) | A_BOLD);
}

std::vector<std::pair<int, int> > minimapConnections(const Board& board) {
    std::vector<std::pair<int, int> > connections;
    for (int i = 0; i < TILE_COUNT; ++i) {
        const Tile& tile = board.tileAt(i);
        if (tile.next >= 0) {
            connections.push_back(std::make_pair(i, tile.next));
        }
        if (tile.altNext >= 0) {
            connections.push_back(std::make_pair(i, tile.altNext));
        }
    }
    return connections;
}

void drawMiniLine(WINDOW* panelWin, int y1, int x1, int y2, int x2) {
    wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND) | A_DIM);
    int x = x1;
    int y = y1;
    while (x != x2) {
        x += (x2 > x) ? 1 : -1;
        mvwaddch(panelWin, y, x, '.');
    }
    while (y != y2) {
        y += (y2 > y) ? 1 : -1;
        mvwaddch(panelWin, y, x, '.');
    }
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND) | A_DIM);
}

void initialize_game_ui() {
    std::setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) {
        initGameColors();
        bkgd(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    }
    clear();
    refresh();
}

void destroy_game_ui() {
    endwin();
}

void apply_ui_background(WINDOW* window) {
    if (!window) return;
    wbkgd(window, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
}

int ui_player_color_pair(int playerIndex) {
    switch (playerIndex % 4) {
        case 0: return GOLDRUSH_PLAYER_ONE;
        case 1: return GOLDRUSH_PLAYER_TWO;
        case 2: return GOLDRUSH_PLAYER_THREE;
        default: return GOLDRUSH_PLAYER_FOUR;
    }
}

int getTileColorPair(const Tile& tile) {
    switch (tile.kind) {
        case TILE_PAYDAY:
        case TILE_SAFE:
            return GOLDRUSH_TILE_PAYDAY;
        case TILE_BLACK:
            return tile.value >= 3 ? GOLDRUSH_TILE_MINIGAME : GOLDRUSH_TILE_ACTION;
        case TILE_RISKY:
        case TILE_MARRIAGE:
            return GOLDRUSH_TILE_RISK;
        case TILE_CAREER:
        case TILE_CAREER_2:
        case TILE_COLLEGE:
        case TILE_GRADUATION:
        case TILE_NIGHT_SCHOOL:
            return GOLDRUSH_TILE_CAREER;
        case TILE_HOUSE:
        case TILE_FAMILY:
        case TILE_BABY:
        case TILE_SPLIT_FAMILY:
            return GOLDRUSH_TILE_HOME;
        case TILE_START:
        case TILE_SPLIT_START:
        case TILE_SPLIT_RISK:
        case TILE_SPIN_AGAIN:
        case TILE_RETIREMENT:
            return GOLDRUSH_TILE_ROUTE;
        case TILE_EMPTY:
        default:
            return GOLDRUSH_TILE_BASIC;
    }
}

void draw_title_banner_ui(WINDOW* titleWin) {
    int height = 0;
    int width = 0;
    getmaxyx(titleWin, height, width);
    werase(titleWin);
    box(titleWin, 0, 0);
    wattron(titleWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    mvwprintw(titleWin, 0, 2, " GOLDRUSH ");
    if (height < 8 || width < 70) {
        const std::string title = "GOLDRUSH";
        const int titleX = std::max(2, (width - static_cast<int>(title.size())) / 2);
        mvwprintw(titleWin, std::min(1, height - 2), titleX, "%s", title.c_str());
        if (height >= 4) {
            const std::string subtitle = "Retire with the highest net worth";
            mvwprintw(titleWin,
                      2,
                      std::max(2, (width - static_cast<int>(subtitle.size())) / 2),
                      "%s",
                      clipPanelText(subtitle, static_cast<std::size_t>(std::max(8, width - 4))).c_str());
        }
    } else {
        mvwprintw(titleWin, 1, 2, "  ________       .__       .___                   .__     ");
        mvwprintw(titleWin, 2, 2, " /  _____/  ____ |  |    __| _/______ __ __  _____|  |__  ");
        mvwprintw(titleWin, 3, 2, "/   \\  ___ /  _ \\|  |   / __ |\\_  __ \\  |  \\/  ___/  |  \\ ");
        mvwprintw(titleWin, 4, 2, "\\    \\_\\  (  <_> )  |__/ /_/ | |  | \\/  |  /\\___ \\|   Y  \\");
        mvwprintw(titleWin, 5, 2, " \\______  /\\____/|____/\\____ | |__|  |____//____  >___|  /");
        mvwprintw(titleWin, 6, 2, "        \\/                  \\/                  \\/     \\/ ");
    }
    wattroff(titleWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    wrefresh(titleWin);
}

void draw_board_ui(WINDOW* boardWin,
                   const Board& board,
                   const std::vector<Player>& players,
                   int currentPlayer,
                   int highlightedTile) {
    board.render(boardWin, players, currentPlayer, highlightedTile, has_colors());
}

void drawBoardLegend(WINDOW* win) {
    if (!win) {
        return;
    }

    Board legendBoard;
    drawLegendEntry(win, 0, 0, legendBoard.tileAt(20), "Payday");
    drawLegendEntry(win, 0, 18, legendBoard.tileAt(39), "Action");
    drawLegendEntry(win, 1, 0, legendBoard.tileAt(84), "Minigame");
    drawLegendEntry(win, 1, 18, legendBoard.tileAt(83), "Risk");
}

void drawCurrentHintBox(WINDOW* win, const Board& board, const Player& player, const RuleSet& rules) {
    if (!win) {
        return;
    }

    int height = 0;
    int width = 0;
    getmaxyx(win, height, width);
    werase(win);
    if (height <= 3) {
        std::string secondLine;
        if (player.loans >= 4) {
            secondLine = "Debt warning: " + std::to_string(player.loans) + " loans.";
        } else if (player.cash < 30000) {
            secondLine = "Low cash: avoid a big penalty.";
        } else if (!player.actionCards.empty()) {
            secondLine = "Hint: " + std::to_string(player.actionCards.size()) + " action card(s) ready.";
        } else {
            secondLine = "Region: " + board.regionNameForTile(player.tile) + ".";
        }

        mvwprintw(win, 0, 0, "%s", clipPanelText("GOAL: Retire with the highest net worth.", static_cast<std::size_t>(std::max(8, width - 1))).c_str());
        mvwprintw(win, 1, 0, "%s", clipPanelText(secondLine, static_cast<std::size_t>(std::max(8, width - 1))).c_str());
        const int paydayDistance = distanceToNextTileKind(board, player, TILE_PAYDAY, 8);
        const std::string thirdLine = paydayDistance > 0
            ? "Next payday in " + std::to_string(paydayDistance) + " space" + (paydayDistance == 1 ? "." : "s.")
            : "No payday nearby.";
        mvwprintw(win, 2, 0, "%s", clipPanelText(thirdLine, static_cast<std::size_t>(std::max(8, width - 1))).c_str());
        wrefresh(win);
        return;
    }
    const bool framed = height >= 6;
    const int textX = framed ? 2 : 0;
    const int headerY = framed ? 1 : 0;
    const int line1Y = framed ? 2 : 1;
    const int line2Y = framed ? 3 : 2;
    const int line3Y = framed ? 4 : 3;

    if (framed) {
        box(win, 0, 0);
    }
    drawPanelHeader(win, headerY, "CURRENT GOAL");
    mvwprintw(win, line1Y, textX, "%s", clipPanelText("Reach retirement with the highest net worth.", static_cast<std::size_t>(std::max(8, width - textX - 1))).c_str());

    std::string secondLine;
    if (player.loans >= 4) {
        secondLine = "Warning: " + std::to_string(player.loans) + " loans make penalties sting.";
    } else if (player.cash < 30000) {
        secondLine = "Low cash: avoid big losses or more loans may trigger.";
    } else if (!player.actionCards.empty()) {
        secondLine = "Hint: you have " + std::to_string(player.actionCards.size()) + " action card(s) ready.";
    } else if (rules.toggles.investmentEnabled && player.investedNumber > 0) {
        secondLine = "Investment: spin " + std::to_string(player.investedNumber) + " pays $" +
                     std::to_string(player.investPayout) + ".";
    } else {
        secondLine = "Region: " + board.regionNameForTile(player.tile) + ".";
    }
    mvwprintw(win, line2Y, textX, "%s", clipPanelText(secondLine, static_cast<std::size_t>(std::max(8, width - textX - 1))).c_str());

    const int paydayDistance = distanceToNextTileKind(board, player, TILE_PAYDAY, 8);
    if (paydayDistance > 0) {
        mvwprintw(win, line3Y, textX, "Next payday in %d space%s.", paydayDistance, paydayDistance == 1 ? "" : "s");
    } else {
        mvwprintw(win, line3Y, textX, "%s", clipPanelText("No payday is nearby on the visible route.", static_cast<std::size_t>(std::max(8, width - textX - 1))).c_str());
    }
    wrefresh(win);
}

void drawPlayerPanel(WINDOW* sideWin,
                     const Board& board,
                     const std::vector<Player>& players,
                     int currentPlayerIndex) {
    if (!sideWin) {
        return;
    }
    if (players.empty() || currentPlayerIndex < 0 || currentPlayerIndex >= static_cast<int>(players.size())) {
        return;
    }

    const Player& player = players[static_cast<std::size_t>(currentPlayerIndex)];
    int panelHeight = 0;
    int panelWidth = 0;
    getmaxyx(sideWin, panelHeight, panelWidth);
    const bool compact = panelHeight < 31 || panelWidth < 42;
    const int rosterWidth = std::max(10, panelWidth - 5);
    const int regionWidth = std::max(10, panelWidth - 18);
    const int valueWidth = std::max(10, panelWidth - 15);
    drawPanelHeader(sideWin, 1, "PLAYERS");
    for (std::size_t i = 0; i < players.size() && i < 4; ++i) {
        const int row = 2 + static_cast<int>(i);
        const int colorPair = ui_player_color_pair(static_cast<int>(i));
        const std::string typeText =
            players[i].type == PlayerType::CPU
                ? "CPU " + cpuDifficultyLabel(players[i].cpuDifficulty)
                : "HUMAN";
        std::ostringstream roster;
        roster << "P" << (i + 1) << " [" << players[i].token << "] " << typeText;
        if (players[i].retired) {
            roster << " RET";
        }

        if (static_cast<int>(i) == currentPlayerIndex) {
            wattron(sideWin, COLOR_PAIR(colorPair) | A_BOLD | A_REVERSE);
            mvwprintw(sideWin, row, 2, "%-*s", rosterWidth, clipPanelText(roster.str(), static_cast<std::size_t>(rosterWidth)).c_str());
            wattroff(sideWin, COLOR_PAIR(colorPair) | A_BOLD | A_REVERSE);
        } else {
            wattron(sideWin, COLOR_PAIR(colorPair) | A_BOLD);
            mvwprintw(sideWin, row, 2, "%-*s", rosterWidth, clipPanelText(roster.str(), static_cast<std::size_t>(rosterWidth)).c_str());
            wattroff(sideWin, COLOR_PAIR(colorPair) | A_BOLD);
        }
    }

    if (compact) {
        drawPanelHeader(sideWin, 6, "STATUS");
        drawLabeledValue(sideWin, 7, "Cash:", "$" + std::to_string(player.cash), GOLDRUSH_TILE_PAYDAY);
        drawLabeledValue(sideWin, 8, "Loans:", std::to_string(player.loans) + " | Inv " +
            (player.investedNumber > 0 ? std::to_string(player.investedNumber) : "None"),
            player.loans > 0 ? GOLDRUSH_TILE_RISK : GOLDRUSH_BROWN_CREAM);
        drawLabeledValue(sideWin, 9, "Job:", clipPanelText(player.job, static_cast<std::size_t>(valueWidth)));
        drawLabeledValue(sideWin, 10, "Space:", std::to_string(player.tile) + " / " +
            clipPanelText(board.regionNameForTile(player.tile), static_cast<std::size_t>(regionWidth)));

        drawPanelHeader(sideWin, 12, "LIFE");
        drawLabeledValue(sideWin, 13, "Family:", std::string(player.married ? "Married" : "Single") +
            " | Kids " + std::to_string(player.kids));
        drawLabeledValue(sideWin, 14, "Home:", clipPanelText(player.retirementHome.empty()
            ? (player.houseName.empty() ? "None" : player.houseName)
            : player.retirementHome, static_cast<std::size_t>(valueWidth)));
        drawLabeledValue(sideWin, 15, "Pets:", std::to_string(static_cast<int>(player.petCards.size())));

        drawPanelHeader(sideWin, 17, "DEFENSE");
        if (player.type == PlayerType::CPU) {
            drawLabeledValue(sideWin, 18, "CPU:", cpuDifficultyLabel(player.cpuDifficulty) +
                " | Sab " + std::to_string(player.sabotageCooldown),
                ui_player_color_pair(currentPlayerIndex));
        } else {
            drawLabeledValue(sideWin, 18, "Defense:", "Sh " + std::to_string(player.shieldCards) +
                " | In " + std::to_string(player.insuranceUses) +
                " | Sab " + std::to_string(player.sabotageCooldown));
        }
    } else {
        drawPanelHeader(sideWin, 6, "STATUS");
        drawLabeledValue(sideWin, 7, "Cash:", "$" + std::to_string(player.cash), GOLDRUSH_TILE_PAYDAY);
        drawLabeledValue(sideWin, 8, "Loans:", std::to_string(player.loans), player.loans > 0 ? GOLDRUSH_TILE_RISK : GOLDRUSH_BROWN_CREAM);
        drawLabeledValue(sideWin, 9, "Job:", clipPanelText(player.job, static_cast<std::size_t>(valueWidth)));
        drawLabeledValue(sideWin, 10, "Invest:", player.investedNumber > 0
            ? (std::to_string(player.investedNumber) + " -> $" + std::to_string(player.investPayout))
            : "None");
        drawLabeledValue(sideWin, 11, "Space:", std::to_string(player.tile) + " / " + clipPanelText(board.regionNameForTile(player.tile), static_cast<std::size_t>(regionWidth)));

        drawPanelHeader(sideWin, 13, "LIFE");
        drawLabeledValue(sideWin, 14, "Married:", std::string(player.married ? "Yes" : "No") + "  Kids: " + std::to_string(player.kids));
        drawLabeledValue(sideWin, 15, "Pets:", std::to_string(static_cast<int>(player.petCards.size())));
        drawLabeledValue(sideWin, 16, "Home:", clipPanelText(player.retirementHome.empty()
            ? (player.houseName.empty() ? "None" : player.houseName)
            : player.retirementHome, static_cast<std::size_t>(valueWidth)));

        drawPanelHeader(sideWin, 18, "DEFENSE");
        if (player.type == PlayerType::CPU) {
            drawLabeledValue(sideWin, 19, "CPU:", cpuDifficultyLabel(player.cpuDifficulty), ui_player_color_pair(currentPlayerIndex));
        } else {
            drawLabeledValue(sideWin, 19, "Shields:", std::to_string(player.shieldCards));
            drawLabeledValue(sideWin, 20, "Insurance:", std::to_string(player.insuranceUses));
        }
        drawLabeledValue(sideWin, 21, "Sab CD:", std::to_string(player.sabotageCooldown));
    }
}

std::string formatHistoryEvent(const std::string& eventText) {
    return "[" + historyCategory(eventText) + "] " + eventText;
}

int getHistoryEventColor(const std::string& eventText) {
    const std::string category = historyCategory(eventText);
    if (category == "MONEY") return GOLDRUSH_TILE_PAYDAY;
    if (category == "CARD") return GOLDRUSH_TILE_ACTION;
    if (category == "MINIGAME") return GOLDRUSH_TILE_MINIGAME;
    if (category == "DUEL") return GOLDRUSH_TILE_ROUTE;
    if (category == "MOVE") return GOLDRUSH_TILE_CAREER;
    return GOLDRUSH_BROWN_CREAM;
}

void drawEventMessage(WINDOW* messageWin, const std::string& title, const std::string& message) {
    if (!messageWin) {
        return;
    }

    int height = 0;
    int width = 0;
    getmaxyx(messageWin, height, width);
    werase(messageWin);
    box(messageWin, 0, 0);

    wattron(messageWin, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    mvwprintw(messageWin, 1, 2, "EVENT: %s", clipPanelText(title, static_cast<std::size_t>(std::max(0, width - 10))).c_str());
    wattroff(messageWin, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);

    const std::vector<std::string> lines = wrapUiText(message, static_cast<std::size_t>(std::max(10, width - 4)));
    const int maxLines = std::min(std::max(0, height - 4), static_cast<int>(lines.size()));
    wattron(messageWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    for (int i = 0; i < maxLines; ++i) {
        mvwprintw(messageWin, 2 + i, 2, "%s", lines[static_cast<std::size_t>(i)].c_str());
    }
    wattroff(messageWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    wrefresh(messageWin);
}

void draw_sidebar_ui(WINDOW* panelWin,
                     const Board& board,
                     const std::vector<Player>& players,
                     int currentPlayer,
                     const std::vector<std::string>& historyLines,
                     const RuleSet& rules) {
    (void)historyLines;
    werase(panelWin);
    box(panelWin, 0, 0);

    wattron(panelWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    mvwprintw(panelWin, 1, 2, "MINI MAP");
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);

    const int mapTop = 3;
    const int mapLeft = 3;
    const int mapWidth = 32;
    const int mapHeight = 11;
    int minTileX = board.tileAt(0).x;
    int maxTileX = board.tileAt(0).x;
    int minTileY = board.tileAt(0).y;
    int maxTileY = board.tileAt(0).y;
    for (int i = 1; i < TILE_COUNT; ++i) {
        const Tile& tile = board.tileAt(i);
        minTileX = std::min(minTileX, tile.x);
        maxTileX = std::max(maxTileX, tile.x);
        minTileY = std::min(minTileY, tile.y);
        maxTileY = std::max(maxTileY, tile.y);
    }

    wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
    mvwhline(panelWin, 2, 1, ACS_HLINE, 38);
    mvwhline(panelWin, mapTop + mapHeight + 1, 1, ACS_HLINE, 38);
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));

    const std::vector<std::pair<int, int> > connections = minimapConnections(board);
    for (std::size_t i = 0; i < connections.size(); ++i) {
        const Tile& from = board.tileAt(connections[i].first);
        const Tile& to = board.tileAt(connections[i].second);
        const int fromX = mapLeft + ((from.x - minTileX) * (mapWidth - 1)) / std::max(1, maxTileX - minTileX);
        const int fromY = mapTop + ((from.y - minTileY) * (mapHeight - 1)) / std::max(1, maxTileY - minTileY);
        const int toX = mapLeft + ((to.x - minTileX) * (mapWidth - 1)) / std::max(1, maxTileX - minTileX);
        const int toY = mapTop + ((to.y - minTileY) * (mapHeight - 1)) / std::max(1, maxTileY - minTileY);
        drawMiniLine(panelWin, fromY, fromX, toY, toX);
    }

    for (int i = 0; i < TILE_COUNT; ++i) {
        const Tile& tile = board.tileAt(i);
        const int drawX = mapLeft + ((tile.x - minTileX) * (mapWidth - 1)) / std::max(1, maxTileX - minTileX);
        const int drawY = mapTop + ((tile.y - minTileY) * (mapHeight - 1)) / std::max(1, maxTileY - minTileY);

        const bool playerOneHere = players.size() > 0 && players[0].tile == i;
        const bool playerTwoHere = players.size() > 1 && players[1].tile == i;

        if (playerOneHere && playerTwoHere) {
            drawMinimapDot(panelWin, drawY, drawX, "◎", GOLDRUSH_GOLD_TERRA);
        } else if (playerOneHere) {
            drawMinimapDot(panelWin, drawY, drawX, "●", GOLDRUSH_PLAYER_ONE);
        } else if (playerTwoHere) {
            drawMinimapDot(panelWin, drawY, drawX, "●", GOLDRUSH_BLACK_FOREST);
        } else if (tile.kind == TILE_RETIREMENT) {
            drawMinimapDot(panelWin, drawY, drawX, "★", GOLDRUSH_GOLD_TERRA);
        } else if (isSpecialMinimapTile(tile)) {
            drawMinimapDot(panelWin, drawY, drawX, "◆", GOLDRUSH_BLACK_TERRA);
        } else {
            drawMinimapDot(panelWin, drawY, drawX, "·", GOLDRUSH_BROWN_CREAM);
        }
    }

    mvwprintw(panelWin, mapTop + mapHeight + 2, 2, "P1");
    drawMinimapDot(panelWin, mapTop + mapHeight + 2, 6, "●", GOLDRUSH_PLAYER_ONE);
    mvwprintw(panelWin, mapTop + mapHeight + 2, 9, "Gold");
    mvwprintw(panelWin, mapTop + mapHeight + 3, 2, "P2");
    drawMinimapDot(panelWin, mapTop + mapHeight + 3, 6, "●", GOLDRUSH_BLACK_FOREST);
    mvwprintw(panelWin, mapTop + mapHeight + 3, 9, "Green");

    if (!players.empty() && currentPlayer >= 0 && currentPlayer < static_cast<int>(players.size())) {
        const Player& player = players[currentPlayer];
        const std::string home = player.retirementHome.empty() ? "--" : player.retirementHome;
        const std::string invest = player.investedNumber > 0 ? std::to_string(player.investedNumber) : "-";
        const int statsY = mapTop + mapHeight + 5;

        wattron(panelWin, COLOR_PAIR(ui_player_color_pair(currentPlayer)) | A_BOLD);
        mvwprintw(panelWin, statsY, 2, "%-.24s's trail", player.name.c_str());
        wattroff(panelWin, COLOR_PAIR(ui_player_color_pair(currentPlayer)) | A_BOLD);

        wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        mvwprintw(panelWin, statsY + 1, 2, "Cash:%d  Loans:%d", player.cash, player.loans);
        mvwprintw(panelWin, statsY + 2, 2, "Job:%-.15s  Inv:%s", player.job.c_str(), invest.c_str());
        mvwprintw(panelWin, statsY + 3, 2, "Married:%s Kids:%d Pets:%d",
                  player.married ? "Y" : "N",
                  player.kids,
                  static_cast<int>(player.petCards.size()));
        mvwprintw(panelWin, statsY + 4, 2, "Home:%-.24s", home.c_str());
        mvwprintw(panelWin, statsY + 5, 2, "%-.34s", rules.editionName.c_str());
        wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    }
    wrefresh(panelWin);
}

void draw_message_ui(WINDOW* msgWin, const std::string& line1, const std::string& line2) {
    drawEventMessage(msgWin, line1, line2);
}

int selector_component(char* prompt_text,
                       char** option_texts,
                       int* option_values,
                       int number_of_options,
                       int default_value,
                       int y_offset,
                       int width,
                       int height) {
    const UILayout layout = calculateUILayout();
    int termHeight = 0;
    int termWidth = 0;
    getmaxyx(stdscr, termHeight, termWidth);
    const int preferredX = layout.originX + layout.boardWidth + 2;
    const int preferredY = layout.originY + layout.headerHeight + 1 + y_offset;
    const int x = std::max(0, std::min(preferredX, termWidth - width - 3));
    const int y = std::max(1, std::min(preferredY, termHeight - height - 2));

    int chosen_option_value = default_value;
    int highlighted_option = 0;

    for (int i = 0; i < number_of_options; ++i) {
        if (option_values[i] == default_value) {
            highlighted_option = i;
            break;
        }
    }

    while (true) {
        attron(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        mvaddch(y - 1, x - 2, ACS_ULCORNER);
        mvhline(y - 1, x - 1, ACS_HLINE, width + 2);
        mvaddch(y - 1, x + width + 1, ACS_URCORNER);
        for (int row = 0; row < height; ++row) {
            mvaddch(y + row, x - 2, ACS_VLINE);
            mvaddch(y + row, x + width + 1, ACS_VLINE);
        }
        mvaddch(y + height, x - 2, ACS_LLCORNER);
        mvhline(y + height, x - 1, ACS_HLINE, width + 2);
        mvaddch(y + height, x + width + 1, ACS_LRCORNER);
        mvprintw(y, x, "%s", prompt_text);
        attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

        for (int i = 0; i < number_of_options; ++i) {
            const int optionY = y + 1 + i;
            if (i == highlighted_option) {
                attron(A_REVERSE | A_BOLD);
            }
            mvprintw(optionY, x, "%-*s", width, option_texts[i]);
            if (i == highlighted_option) {
                attroff(A_REVERSE | A_BOLD);
            }
        }

        refresh();
        const int input_character = getch();
        if (input_character == KEY_UP) {
            highlighted_option = highlighted_option == 0 ? number_of_options - 1 : highlighted_option - 1;
        } else if (input_character == KEY_DOWN) {
            highlighted_option = highlighted_option == number_of_options - 1 ? 0 : highlighted_option + 1;
        } else if (input_character == '\n' || input_character == '\r' || input_character == KEY_ENTER) {
            chosen_option_value = option_values[highlighted_option];
            break;
        }
    }

    return chosen_option_value;
}

int choose_branch_with_selector(const std::string& prompt_title,
                                const std::vector<std::string>& option_lines,
                                const std::vector<int>& option_values,
                                int default_value) {
    std::vector<char*> mutable_options;
    mutable_options.reserve(option_lines.size());
    for (size_t i = 0; i < option_lines.size(); ++i) {
        mutable_options.push_back(const_cast<char*>(option_lines[i].c_str()));
    }
    std::vector<int> mutable_values = option_values;
    char* prompt = const_cast<char*>(prompt_title.c_str());
    int width = static_cast<int>(prompt_title.size());
    for (size_t i = 0; i < option_lines.size(); ++i) {
        width = std::max(width, static_cast<int>(option_lines[i].size()));
    }
    return selector_component(prompt,
                              mutable_options.data(),
                              mutable_values.data(),
                              static_cast<int>(option_lines.size()),
                              default_value,
                              0,
                              width,
                              static_cast<int>(option_lines.size()) + 1);
}

void update_position_highlights(WINDOW* boardWin,
                                const Board& board,
                                const std::vector<Player>& players,
                                int current_position,
                                int previous_position,
                                int playerIndex) {
    (void)previous_position;
    board.render(boardWin, players, playerIndex, current_position, has_colors());
}
