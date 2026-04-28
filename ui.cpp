#include "ui.h"

#include <algorithm>
#include <cctype>
#include <clocale>
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

std::string trimChoicePrefix(const std::string& text) {
    std::string out = text;
    while (!out.empty() && (out[0] == '-' || std::isspace(static_cast<unsigned char>(out[0])))) {
        out.erase(out.begin());
    }
    return out;
}

void drawChoiceCard(int y,
                    int x,
                    int width,
                    int height,
                    const std::string& optionText,
                    bool highlighted) {
    const std::string clean = trimChoicePrefix(optionText);
    const std::string::size_type colon = clean.find(':');
    const std::string title = colon == std::string::npos ? clean : clean.substr(0, colon);
    const std::string detail = colon == std::string::npos ? "" : clean.substr(colon + 1);

    if (highlighted) {
        attron(A_REVERSE | A_BOLD);
    } else {
        attron(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    }
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x + 1, ACS_HLINE, width - 2);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    for (int row = 1; row < height - 1; ++row) {
        mvaddch(y + row, x, ACS_VLINE);
        mvprintw(y + row, x + 1, "%*s", width - 2, "");
        mvaddch(y + row, x + width - 1, ACS_VLINE);
    }
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
    if (highlighted) {
        attroff(A_REVERSE | A_BOLD);
    } else {
        attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    }

    attron(A_BOLD);
    mvprintw(y + 1, x + 2, "%-*s", width - 4,
             clipUiText(title, static_cast<std::size_t>(width - 4)).c_str());
    attroff(A_BOLD);
    mvhline(y + 2, x + 2, ACS_HLINE, width - 4);

    const std::vector<std::string> detailLines =
        wrapUiText(detail.empty() ? clean : detail, static_cast<std::size_t>(width - 4));
    for (int i = 0; i < height - 5 && i < static_cast<int>(detailLines.size()); ++i) {
        mvprintw(y + 4 + i, x + 2, "%-*s", width - 4,
                 clipUiText(detailLines[static_cast<std::size_t>(i)],
                            static_cast<std::size_t>(width - 4)).c_str());
    }
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
    init_pair(GOLDRUSH_PLAYER_TWO, COLOR_CYAN, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_THREE, terra, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_FOUR, forest, COLOR_BLACK);
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
            const std::string subtitle = "Goal: Retire with the highest net worth";
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
        const std::string goal = "Goal: Retire with the highest net worth";
        mvwprintw(titleWin,
                  7,
                  std::max(2, (width - static_cast<int>(goal.size())) / 2),
                  "%s",
                  goal.c_str());
    }
    wattroff(titleWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    wrefresh(titleWin);
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

void draw_board_ui(WINDOW* boardWin,
                   const Board& board,
                   const std::vector<Player>& players,
                   int highlightedTile,
                   int currentPlayerIndex) {
    board.render(boardWin, players, has_colors());

    if (currentPlayerIndex < 0 ||
        currentPlayerIndex >= static_cast<int>(players.size()) ||
        highlightedTile < 0 ||
        highlightedTile >= TILE_COUNT) {
        return;
    }

    const Tile& tile = board.tileAt(highlightedTile);
    int occupants = 0;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (players[i].tile == highlightedTile) {
            ++occupants;
        }
    }

    std::string marker = occupants > 1
        ? "[2P]"
        : "<P" + std::to_string(currentPlayerIndex + 1) + ">";

    if (marker.size() > 4) {
        marker = "<P" + std::to_string(currentPlayerIndex + 1);
        if (marker.size() > 4) {
            marker = "[P+]";
        }
    }

    static int lastAnimatedPlayer = -1;
    if (lastAnimatedPlayer != currentPlayerIndex) {
        blinkIndicator(boardWin,
                       tile.y,
                       tile.x,
                       marker,
                       has_colors(),
                       ui_player_color_pair(currentPlayerIndex),
                       2,
                       2000,
                       4);
        lastAnimatedPlayer = currentPlayerIndex;
    } else {
        wattron(boardWin, COLOR_PAIR(ui_player_color_pair(currentPlayerIndex)) | A_BOLD | A_REVERSE);
        mvwprintw(boardWin, tile.y, tile.x, "%-4s", marker.c_str());
        wattroff(boardWin, COLOR_PAIR(ui_player_color_pair(currentPlayerIndex)) | A_BOLD | A_REVERSE);
        wrefresh(boardWin);
    }
}

void draw_sidebar_ui(WINDOW* panelWin,
                     const Board& board,
                     const std::vector<Player>& players,
                     int currentPlayer,
                     const std::vector<std::string>& historyLines,
                     const RuleSet& rules) {
    (void)rules;
    int panelHeight = 0;
    int panelWidth = 0;
    getmaxyx(panelWin, panelHeight, panelWidth);
    const bool compact = panelHeight < 31 || panelWidth < 42;
    const int historyHeaderY = compact ? 21 : 23;
    const int historyStartY = compact ? 22 : 24;
    const int historyMaxLines = compact ? 5 : 5;
    const int historyWidth = std::max(12, panelWidth - 5);

    werase(panelWin);
    box(panelWin, 0, 0);

    if (!players.empty() && currentPlayer >= 0 && currentPlayer < static_cast<int>(players.size())) {
        drawPlayerPanel(panelWin, board, players, currentPlayer);
    }

    drawPanelHeader(panelWin, historyHeaderY, "HISTORY");
    for (std::size_t i = 0; i < historyLines.size() && i < static_cast<std::size_t>(historyMaxLines); ++i) {
        const int colorPair = getHistoryEventColor(historyLines[i]);
        const std::string formatted = clipPanelText(formatHistoryEvent(historyLines[i]), static_cast<std::size_t>(historyWidth));
        wattron(panelWin, COLOR_PAIR(colorPair));
        mvwprintw(panelWin, historyStartY + static_cast<int>(i), 2, "%-*s", historyWidth, formatted.c_str());
        wattroff(panelWin, COLOR_PAIR(colorPair));
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

    if (number_of_options == 2) {
        const int cardW = 34;
        const int cardH = 10;
        const int gap = 4;
        const int popupW = (cardW * 2) + gap;
        const int popupH = cardH + 5;
        const int popupX = std::max(0, (termWidth - popupW) / 2);
        const int popupY = std::max(1, (termHeight - popupH) / 2);

        while (true) {
            attron(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
            mvprintw(popupY, popupX, "%-*s", popupW,
                     clipUiText(prompt_text, static_cast<std::size_t>(popupW)).c_str());
            attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

            drawChoiceCard(popupY + 2, popupX, cardW, cardH, option_texts[0], highlighted_option == 0);
            drawChoiceCard(popupY + 2, popupX + cardW + gap, cardW, cardH, option_texts[1], highlighted_option == 1);

            mvprintw(popupY + popupH - 2, popupX, "%-*s", popupW,
                     "Use LEFT/RIGHT or A/D to choose. Press ENTER to confirm.");
            refresh();

            const int input_character = getch();
            if (input_character == KEY_LEFT || input_character == KEY_RIGHT ||
                input_character == 'a' || input_character == 'A' ||
                input_character == 'd' || input_character == 'D' ||
                input_character == KEY_UP || input_character == KEY_DOWN ||
                input_character == 'w' || input_character == 'W' ||
                input_character == 's' || input_character == 'S') {
                highlighted_option = highlighted_option == 0 ? 1 : 0;
            } else if (input_character == '\n' || input_character == '\r' || input_character == KEY_ENTER) {
                return option_values[highlighted_option];
            }
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
    draw_board_ui(boardWin, board, players, current_position, playerIndex);
}
