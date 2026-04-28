#include "ui.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <locale.h>
#include <set>
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
const int UI_ESC_DELAY_MS = 25;

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

std::string playerLifePhase(const Board& board, const Player& player) {
    if (player.retired || board.tileAt(player.tile).kind == TILE_RETIREMENT) {
        return "Retired";
    }
    if (player.tile == 0) {
        return "Starting Out";
    }
    if (player.tile >= 13 && player.tile <= 37) {
        return player.startChoice == 0 ? "College Years" : "Career Launch";
    }
    if (player.tile >= 1 && player.tile <= 58) {
        if (board.tileAt(player.tile).kind == TILE_MARRIAGE) {
            return "Marriage Stop";
        }
        return "Early Adult Life";
    }
    if (player.tile >= 59 && player.tile <= 68) {
        return "Family Years";
    }
    if (player.tile >= 69 && player.tile <= 78) {
        return "Work Years";
    }
    if (player.tile >= 79 && player.tile <= 86) {
        return player.riskChoice == 1 ? "Risk Route" : "Safe Route";
    }
    return board.regionNameForTile(player.tile);
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
    drawBoxAtSafe(stdscr, y, x, height, width);
    attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
}

bool isSpecialMinimapTile(const Tile& tile) {
    switch (tile.kind) {
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

std::string minimapTileLabel(const Tile& tile) {
    switch (tile.kind) {
        case TILE_COLLEGE:
            return "O";
        case TILE_CAREER:
            return "A";
        case TILE_GRADUATION:
            return "G";
        case TILE_MARRIAGE:
            return "M";
        case TILE_FAMILY:
            return "F";
        case TILE_CAREER_2:
            return "W";
        case TILE_SAFE:
            return "S";
        case TILE_RISKY:
            return "R";
        case TILE_RETIREMENT:
            return "RET";
        case TILE_START:
            return "ST";
        default:
            return "";
    }
}

std::string minimapPathGlyph(int tileId, const Tile& tile) {
    if (tileId == 0) {
        return "ST";
    }
    if (tileId >= 13 && tileId <= 24) {
        return "O";
    }
    if (tileId >= 25 && tileId <= 36) {
        return "A";
    }
    if (tileId == 37) {
        return "G";
    }
    if (tile.kind == TILE_MARRIAGE) {
        return "M";
    }
    if (tileId >= 59 && tileId <= 68) {
        return (tileId == 59) ? "F" : "F";
    }
    if (tileId >= 69 && tileId <= 78) {
        return (tileId == 69) ? "W" : "W";
    }
    if (tileId >= 80 && tileId <= 82) {
        return (tileId == 80) ? "S" : "S";
    }
    if (tileId >= 83 && tileId <= 85) {
        return (tileId == 83) ? "R" : "R";
    }
    if (tile.kind == TILE_RETIREMENT) {
        return "T";
    }
    if (tile.kind == TILE_BLACK || tile.kind == TILE_PAYDAY) {
        return "c";
    }
    return minimapTileLabel(tile);
}

const char* minimapDotGlyph(const Tile& tile) {
    if (tile.kind == TILE_RETIREMENT) {
        return "*";
    }
    if (isSpecialMinimapTile(tile)) {
        return "+";
    }
    return ".";
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

std::vector<int> minimapNeighbors(const Board& board, int tileId) {
    std::vector<int> neighbors;
    for (int i = 0; i < TILE_COUNT; ++i) {
        const Tile& tile = board.tileAt(i);
        if (tile.next == tileId || tile.altNext == tileId) {
            neighbors.push_back(i);
        }
        if (i == tileId) {
            if (tile.next >= 0) {
                neighbors.push_back(tile.next);
            }
            if (tile.altNext >= 0) {
                neighbors.push_back(tile.altNext);
            }
        }
    }
    std::sort(neighbors.begin(), neighbors.end());
    neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    return neighbors;
}

bool minimapNeedsVertex(const Board& board, int tileId) {
    const Tile& tile = board.tileAt(tileId);
    const std::vector<int> neighbors = minimapNeighbors(board, tileId);
    if (neighbors.size() != 2) {
        return true;
    }

    const Tile& a = board.tileAt(neighbors[0]);
    const Tile& b = board.tileAt(neighbors[1]);
    const bool sameColumn = (a.x == tile.x && b.x == tile.x);
    const bool sameRow = (a.y == tile.y && b.y == tile.y);
    return !(sameColumn || sameRow);
}

std::set<int> minimapReachableTiles(const Board& board) {
    std::set<int> reachable;
    std::vector<int> pending(1, 0);

    while (!pending.empty()) {
        const int current = pending.back();
        pending.pop_back();
        if (current < 0 || current >= TILE_COUNT || reachable.count(current) != 0) {
            continue;
        }

        reachable.insert(current);
        const Tile& tile = board.tileAt(current);
        if (tile.next >= 0) {
            pending.push_back(tile.next);
        }
        if (tile.altNext >= 0) {
            pending.push_back(tile.altNext);
        }
    }

    return reachable;
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
    setlocale(LC_ALL, "");
    initscr();
#ifdef NCURSES_VERSION
    set_escdelay(UI_ESC_DELAY_MS);
#endif
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

void drawBoxSafe(WINDOW* window) {
    if (!window) {
        return;
    }
    box(window, 0, 0);
}

void drawBorderLineSafe(WINDOW* window, int y, int x, int width) {
    if (!window || width <= 0) {
        return;
    }
    int maxY = 0;
    int maxX = 0;
    getmaxyx(window, maxY, maxX);
    if (y < 0 || y >= maxY || x >= maxX) {
        return;
    }
    const int startX = std::max(0, x);
    const int endX = std::min(maxX - 1, x + width - 1);
    if (endX < startX) {
        return;
    }
    mvwhline(window, y, startX, ACS_HLINE, endX - startX + 1);
}

void drawBorderColumnSafe(WINDOW* window, int y, int x, int height) {
    if (!window || height <= 0) {
        return;
    }
    int maxY = 0;
    int maxX = 0;
    getmaxyx(window, maxY, maxX);
    if (x < 0 || x >= maxX || y >= maxY) {
        return;
    }
    const int startY = std::max(0, y);
    const int endY = std::min(maxY - 1, y + height - 1);
    if (endY < startY) {
        return;
    }
    mvwvline(window, startY, x, ACS_VLINE, endY - startY + 1);
}

void drawBorderCharSafe(WINDOW* window, int y, int x, chtype ch) {
    if (!window) {
        return;
    }
    int maxY = 0;
    int maxX = 0;
    getmaxyx(window, maxY, maxX);
    if (y < 0 || y >= maxY || x < 0 || x >= maxX) {
        return;
    }
    mvwaddch(window, y, x, ch);
}

void drawBoxAtSafe(WINDOW* window, int y, int x, int height, int width) {
    if (!window || height <= 0 || width <= 0) {
        return;
    }

    const int top = y;
    const int bottom = y + height - 1;
    const int left = x;
    const int right = x + width - 1;

    if (height == 1) {
        drawBorderLineSafe(window, top, left, width);
        return;
    }
    if (width == 1) {
        drawBorderColumnSafe(window, top, left, height);
        return;
    }

    drawBorderCharSafe(window, top, left, ACS_ULCORNER);
    drawBorderLineSafe(window, top, left + 1, width - 2);
    drawBorderCharSafe(window, top, right, ACS_URCORNER);
    drawBorderColumnSafe(window, top + 1, left, height - 2);
    drawBorderColumnSafe(window, top + 1, right, height - 2);
    drawBorderCharSafe(window, bottom, left, ACS_LLCORNER);
    drawBorderLineSafe(window, bottom, left + 1, width - 2);
    drawBorderCharSafe(window, bottom, right, ACS_LRCORNER);
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
    drawBoxSafe(titleWin);
    wattron(titleWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    if (height < 8 || width < 70) {
        const std::string title = "GOLDRUSH";
        const int titleX = std::max(2, (width - static_cast<int>(title.size())) / 2);
        if (height > 2) {
            mvwprintw(titleWin, 1, titleX, "%s",
                      clipPanelText(title, static_cast<std::size_t>(std::max(1, width - 4))).c_str());
        }
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
                   int highlightedTile,
                   BoardViewMode viewMode) {
    board.render(boardWin, players, currentPlayer, highlightedTile, has_colors(), viewMode);
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
        drawBoxSafe(win);
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
    drawBoxSafe(messageWin);

    wattron(messageWin, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    const std::string clippedTitle = clipPanelText(title, static_cast<std::size_t>(std::max(0, width - 4)));
    mvwprintw(messageWin, 1, std::max(2, (width - static_cast<int>(clippedTitle.size())) / 2), "%s", clippedTitle.c_str());
    wattroff(messageWin, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);

    const std::vector<std::string> lines = wrapUiText(message, static_cast<std::size_t>(std::max(10, width - 4)));
    const int maxLines = std::min(std::max(0, height - 4), static_cast<int>(lines.size()));
    wattron(messageWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    for (int i = 0; i < maxLines; ++i) {
        const std::string& line = lines[static_cast<std::size_t>(i)];
        mvwprintw(messageWin, 2 + i, std::max(2, (width - static_cast<int>(line.size())) / 2), "%s", line.c_str());
    }
    wattroff(messageWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    wrefresh(messageWin);
}

void drawMinimapPanel(WINDOW* panelWin,
                      const Board& board,
                      const std::vector<Player>& players,
                      int currentPlayer) {
    werase(panelWin);
    drawBoxSafe(panelWin);

    wattron(panelWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    mvwprintw(panelWin, 1, 2, "MINIMAP");
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);

    const int panelHeight = getmaxy(panelWin);
    const int mapTop = 3;
    const int mapLeft = 3;
    const int mapCellWidth = 3;
    const std::set<int> reachable = minimapReachableTiles(board);
    int minTileX = board.tileAt(0).x;
    int maxTileX = board.tileAt(0).x;
    int minTileY = board.tileAt(0).y;
    int maxTileY = board.tileAt(0).y;
    for (int i = 1; i < TILE_COUNT; ++i) {
        if (reachable.count(i) == 0) {
            continue;
        }
        const Tile& tile = board.tileAt(i);
        minTileX = std::min(minTileX, tile.x);
        maxTileX = std::max(maxTileX, tile.x);
        minTileY = std::min(minTileY, tile.y);
        maxTileY = std::max(maxTileY, tile.y);
    }

    const int logicalRows = std::max(1, maxTileY - minTileY);
    const int maxMapBottom = std::max(mapTop + logicalRows, panelHeight - 11);
    const int mapCellHeight = (mapTop + (logicalRows * 2) <= maxMapBottom) ? 2 : 1;
    const int mapHeight = ((maxTileY - minTileY) * mapCellHeight) + 1;
    const int separatorWidth = std::max(4, getmaxx(panelWin) - 2);
    const int mapBottom = mapTop + mapHeight;

    wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
    drawBorderLineSafe(panelWin, 2, 1, separatorWidth);
    drawBorderLineSafe(panelWin, mapBottom + 1, 1, separatorWidth);
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));

    const std::vector<std::pair<int, int> > connections = minimapConnections(board);
    for (std::size_t i = 0; i < connections.size(); ++i) {
        if (reachable.count(connections[i].first) == 0 || reachable.count(connections[i].second) == 0) {
            continue;
        }
        const Tile& from = board.tileAt(connections[i].first);
        const Tile& to = board.tileAt(connections[i].second);
        const int fromX = mapLeft + ((from.x - minTileX) * mapCellWidth);
        const int fromY = mapTop + ((from.y - minTileY) * mapCellHeight);
        const int toX = mapLeft + ((to.x - minTileX) * mapCellWidth);
        const int toY = mapTop + ((to.y - minTileY) * mapCellHeight);
        drawMiniLine(panelWin, fromY, fromX, toY, toX);
    }

    for (int i = 0; i < TILE_COUNT; ++i) {
        if (reachable.count(i) == 0) {
            continue;
        }
        const Tile& tile = board.tileAt(i);
        const int drawX = mapLeft + ((tile.x - minTileX) * mapCellWidth);
        const int drawY = mapTop + ((tile.y - minTileY) * mapCellHeight);

        int occupants = 0;
        int firstPlayer = -1;
        for (std::size_t p = 0; p < players.size(); ++p) {
            if (players[p].tile != i) {
                continue;
            }
            if (firstPlayer < 0) {
                firstPlayer = static_cast<int>(p);
            }
            ++occupants;
        }

        if (occupants > 1) {
            drawMinimapDot(panelWin, drawY, drawX, "@", GOLDRUSH_GOLD_TERRA);
        } else if (occupants == 1) {
            drawMinimapDot(panelWin, drawY, drawX, "o", ui_player_color_pair(firstPlayer));
        } else {
            drawMinimapDot(panelWin,
                           drawY,
                           drawX,
                           minimapDotGlyph(tile),
                           tile.kind == TILE_RETIREMENT ? GOLDRUSH_GOLD_TERRA
                                                        : (isSpecialMinimapTile(tile) ? GOLDRUSH_BLACK_TERRA
                                                                                      : GOLDRUSH_BROWN_CREAM));
        }
    }

    if (mapBottom + 2 < panelHeight - 1) {
        mvwprintw(panelWin, mapBottom + 2, 2, "%-.34s", "o player  @ stacked  + stop/event");
    }
    if (mapBottom + 3 < panelHeight - 1) {
        mvwprintw(panelWin, mapBottom + 3, 2, "%-.34s", ". route  * retirement");
    }

    if (!players.empty() && currentPlayer >= 0 && currentPlayer < static_cast<int>(players.size())) {
        const Player& player = players[currentPlayer];
        const std::string home = player.retirementHome.empty() ? "--" : player.retirementHome;
        const std::string invest = player.investedNumber > 0 ? std::to_string(player.investedNumber) : "-";
        const int statsY = mapBottom + 5;

        if (statsY < panelHeight - 1) {
            wattron(panelWin, COLOR_PAIR(ui_player_color_pair(currentPlayer)) | A_BOLD);
            mvwprintw(panelWin, statsY, 2, "%-.24s's trail", player.name.c_str());
            wattroff(panelWin, COLOR_PAIR(ui_player_color_pair(currentPlayer)) | A_BOLD);
        }

        wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        if (statsY + 1 < panelHeight - 1) {
            mvwprintw(panelWin, statsY + 1, 2, "Cash:%d  Loans:%d", player.cash, player.loans);
        }
        if (statsY + 2 < panelHeight - 1) {
            mvwprintw(panelWin, statsY + 2, 2, "Job:%-.15s  Inv:%s", player.job.c_str(), invest.c_str());
        }
        if (statsY + 3 < panelHeight - 1) {
            mvwprintw(panelWin, statsY + 3, 2, "Married:%s Kids:%d Pets:%d",
                      player.married ? "Y" : "N",
                      player.kids,
                      static_cast<int>(player.petCards.size()));
        }
        if (statsY + 4 < panelHeight - 1) {
            mvwprintw(panelWin, statsY + 4, 2, "Home:%-.24s", home.c_str());
        }
        if (statsY + 5 < panelHeight - 1) {
            mvwprintw(panelWin, statsY + 5, 2, "Tab/Enter/Esc closes this popup.");
        }
        wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    }

    wrefresh(panelWin);
}

void draw_sidebar_ui(WINDOW* panelWin,
                     const Board& board,
                     const std::vector<Player>& players,
                     int currentPlayer,
                     const std::vector<std::string>& historyLines,
                     const RuleSet& rules) {
    int panelHeight = 0;
    int panelWidth = 0;
    getmaxyx(panelWin, panelHeight, panelWidth);
    const bool compact = panelHeight < 31 || panelWidth < 42;
    const int phaseHeaderY = compact ? 21 : 23;
    const int phaseStartY = phaseHeaderY + 1;
    const int footerY = panelHeight - 3;
    const int textWidth = std::max(12, panelWidth - 5);

    werase(panelWin);
    drawBoxSafe(panelWin);

    if (!players.empty() && currentPlayer >= 0 && currentPlayer < static_cast<int>(players.size())) {
        drawPlayerPanel(panelWin, board, players, currentPlayer);
    }

    drawPanelHeader(panelWin, phaseHeaderY, "LIFE PHASES");
    for (std::size_t i = 0; i < players.size() && (phaseStartY + static_cast<int>(i)) < footerY - 1; ++i) {
        const int row = phaseStartY + static_cast<int>(i);
        const std::string phase = playerLifePhase(board, players[i]);
        wattron(panelWin, COLOR_PAIR(ui_player_color_pair(static_cast<int>(i))) | A_BOLD);
        mvwprintw(panelWin, row, 2, "P%d", static_cast<int>(i + 1));
        wattroff(panelWin, COLOR_PAIR(ui_player_color_pair(static_cast<int>(i))) | A_BOLD);
        mvwprintw(panelWin,
                  row,
                  6,
                  "%-*s",
                  textWidth - 4,
                  clipPanelText(players[i].name + ": " + phase, static_cast<std::size_t>(textWidth - 4)).c_str());
    }

    if (!historyLines.empty() && footerY > phaseStartY + static_cast<int>(players.size())) {
        wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
        drawBorderLineSafe(panelWin, footerY - 1, 1, std::max(4, panelWidth - 2));
        wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
        drawPanelHeader(panelWin, footerY, "LATEST");
        const std::string latest = clipPanelText(historyLines.front(), static_cast<std::size_t>(std::max(8, panelWidth - 5)));
        mvwprintw(panelWin, footerY + 1, 2, "%s", latest.c_str());
    } else if (footerY > phaseStartY + static_cast<int>(players.size())) {
        wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
        drawBorderLineSafe(panelWin, footerY - 1, 1, std::max(4, panelWidth - 2));
        wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
        mvwprintw(panelWin, footerY, 2, "%-.34s", rules.editionName.c_str());
        mvwprintw(panelWin, footerY + 1, 2, "TAB: scores  G: guide  K: controls");
    }

    wrefresh(panelWin);
    return;
}

void draw_message_ui(WINDOW* msgWin, const std::string& line1, const std::string& line2) {
    if (!msgWin) {
        return;
    }

    int height = 0;
    int width = 0;
    getmaxyx(msgWin, height, width);
    werase(msgWin);
    drawBoxSafe(msgWin);

    wattron(msgWin, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    mvwprintw(msgWin, 1, 2, "%s",
              clipPanelText(line1, static_cast<std::size_t>(std::max(8, width - 4))).c_str());
    wattroff(msgWin, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);

    const std::vector<std::string> lines =
        wrapUiText(line2, static_cast<std::size_t>(std::max(10, width - 4)));
    const int maxLines = std::min(std::max(0, height - 4), static_cast<int>(lines.size()));
    wattron(msgWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    for (int i = 0; i < maxLines; ++i) {
        mvwprintw(msgWin, 2 + i, 2, "%s", lines[static_cast<std::size_t>(i)].c_str());
    }
    wattroff(msgWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    wrefresh(msgWin);
}

int selector_component(char* prompt_text,
                       char** option_texts,
                       int* option_values,
                       int number_of_options,
                       int default_value,
                       int y_offset,
                       int width,
                       int height) {
    (void)y_offset;
    auto clipToWidth = [](const char* text, int maxWidth) -> std::string {
        if (!text || maxWidth <= 0) {
            return "";
        }
        std::string value(text);
        if (static_cast<int>(value.size()) <= maxWidth) {
            return value;
        }
        return value.substr(0, static_cast<std::size_t>(maxWidth));
    };

    if (number_of_options <= 0 || !option_texts || !option_values) {
        return default_value;
    }

    int termHeight = 0;
    int termWidth = 0;
    getmaxyx(stdscr, termHeight, termWidth);
    int desiredWidth = std::max(width, prompt_text ? static_cast<int>(std::strlen(prompt_text)) : 0);
    for (int i = 0; i < number_of_options; ++i) {
        desiredWidth = std::max(desiredWidth, option_texts[i] ? static_cast<int>(std::strlen(option_texts[i])) : 0);
    }
    desiredWidth = std::max(12, desiredWidth);
    const int frameWidth = std::min(std::max(20, desiredWidth + 4), std::max(20, termWidth - 2));
    const int frameHeight = std::min(std::max(5, height + 4), std::max(5, termHeight - 2));

    WINDOW* popup = createCenteredWindow(frameHeight, frameWidth, 5, 20);
    if (!popup) {
        showTerminalSizeWarning(5, 20, has_colors());
        return default_value;
    }
    keypad(popup, TRUE);

    int chosen_option_value = default_value;
    int highlighted_option = 0;

    for (int i = 0; i < number_of_options; ++i) {
        if (option_values[i] == default_value) {
            highlighted_option = i;
            break;
        }
    }

    int topOption = 0;
    while (true) {
        int popupH = 0;
        int popupW = 0;
        getmaxyx(popup, popupH, popupW);
        const int contentWidth = std::max(1, popupW - 4);
        const int visibleOptions = std::max(1, popupH - 4);
        if (highlighted_option < topOption) {
            topOption = highlighted_option;
        } else if (highlighted_option >= topOption + visibleOptions) {
            topOption = highlighted_option - visibleOptions + 1;
        }

        werase(popup);
        drawBoxSafe(popup);
        wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        const std::string clippedPrompt = clipToWidth(prompt_text, contentWidth);
        mvwprintw(popup,
                  1,
                  2 + std::max(0, (contentWidth - static_cast<int>(clippedPrompt.size())) / 2),
                  "%s",
                  clippedPrompt.c_str());
        wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

        for (int row = 0; row < visibleOptions; ++row) {
            const int optionIndex = topOption + row;
            if (optionIndex >= number_of_options) {
                break;
            }
            if (optionIndex == highlighted_option) {
                wattron(popup, A_REVERSE | A_BOLD);
            }
            const std::string clippedOption = clipToWidth(option_texts[optionIndex], contentWidth);
            mvwprintw(popup, 2 + row, 2, "%-*s", contentWidth, clippedOption.c_str());
            if (optionIndex == highlighted_option) {
                wattroff(popup, A_REVERSE | A_BOLD);
            }
        }

        if (number_of_options > visibleOptions) {
            mvwprintw(popup, popupH - 2, 2, "Up/Down scroll  Enter select");
        }

        wrefresh(popup);
        const int input_character = wgetch(popup);
        if (input_character == KEY_UP) {
            highlighted_option = highlighted_option == 0 ? number_of_options - 1 : highlighted_option - 1;
        } else if (input_character == KEY_DOWN) {
            highlighted_option = highlighted_option == number_of_options - 1 ? 0 : highlighted_option + 1;
        } else if (input_character == '\n' || input_character == '\r' || input_character == KEY_ENTER) {
            chosen_option_value = option_values[highlighted_option];
            break;
        } else if (input_character == 27) {
            break;
        }
    }

    delwin(popup);
    touchwin(stdscr);
    refresh();
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
