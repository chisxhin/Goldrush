#include "ui.h"

#include <algorithm>
#include <clocale>
#include <utility>
#include <string>
#include <vector>

namespace {
const short UI_COLOR_GOLD = 16;
const short UI_COLOR_BROWN = 17;
const short UI_COLOR_TERRA = 18;
const short UI_COLOR_FOREST = 19;

const int GOLDRUSH_LEFT_PANEL_X = 86;
const int GOLDRUSH_LEFT_PANEL_Y = 9;

short earthy_or(short custom, short fallback) {
    if (can_change_color() && COLORS >= 32) {
        return custom;
    }
    return fallback;
}

void init_theme_colors() {
    start_color();
#ifdef NCURSES_VERSION
    use_default_colors();
#endif

    if (can_change_color() && COLORS >= 32) {
        init_color(UI_COLOR_GOLD, 1000, 843, 0);
        init_color(UI_COLOR_BROWN, 361, 251, 200);
        init_color(UI_COLOR_TERRA, 886, 447, 357);
        init_color(UI_COLOR_FOREST, 133, 545, 133);
    }

    const short gold = earthy_or(UI_COLOR_GOLD, COLOR_YELLOW);
    const short brown = earthy_or(UI_COLOR_BROWN, COLOR_WHITE);
    const short terra = earthy_or(UI_COLOR_TERRA, COLOR_RED);
    const short forest = earthy_or(UI_COLOR_FOREST, COLOR_GREEN);

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
}  // namespace

void initialize_game_ui() {
    std::setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) {
        init_theme_colors();
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

void draw_title_banner_ui(WINDOW* titleWin) {
    werase(titleWin);
    box(titleWin, 0, 0);
    wattron(titleWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    mvwprintw(titleWin, 0, 2, " GOLDRUSH ");
    mvwprintw(titleWin, 1, 2, "  ________       .__       .___                   .__     ");
    mvwprintw(titleWin, 2, 2, " /  _____/  ____ |  |    __| _/______ __ __  _____|  |__  ");
    mvwprintw(titleWin, 3, 2, "/   \\  ___ /  _ \\|  |   / __ |\\_  __ \\  |  \\/  ___/  |  \\ ");
    mvwprintw(titleWin, 4, 2, "\\    \\_\\  (  <_> )  |__/ /_/ | |  | \\/  |  /\\___ \\|   Y  \\");
    mvwprintw(titleWin, 5, 2, " \\______  /\\____/|____/\\____ | |__|  |____//____  >___|  /");
    mvwprintw(titleWin, 6, 2, "        \\/                  \\/                  \\/     \\/ ");
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
    werase(msgWin);
    box(msgWin, 0, 0);
    wattron(msgWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    mvwprintw(msgWin, 1, 2, "%s", line1.c_str());
    wattroff(msgWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    if (!line2.empty()) {
        wattron(msgWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        mvwprintw(msgWin, 2, 2, "%s", line2.c_str());
        wattroff(msgWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    }
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
    const int x = GOLDRUSH_LEFT_PANEL_X;
    const int y = GOLDRUSH_LEFT_PANEL_Y + y_offset;

    int chosen_option_value = default_value;
    int input_character;
    bool is_menu_active = true;
    int highlighted_option = 0;

    for (int i = 0; i < number_of_options; ++i) {
        if (option_values[i] == default_value) {
            highlighted_option = i;
            break;
        }
    }

    while (true) {
        draw_menu_border(is_menu_active, x - 2, y - 1, width + 4, height + 2);
        attron(COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
        mvprintw(y, x, "%s", prompt_text);
        attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);

        for (int i = 0; i < number_of_options; ++i) {
            int option_y_offset = y + 1 + i;
            if (i == highlighted_option) {
                attron(A_REVERSE);
            }
            mvprintw(option_y_offset, x, "%-*s", width, option_texts[i]);
            if (i == highlighted_option) {
                attroff(A_REVERSE);
            }
        }

        refresh();
        if (!is_menu_active) break;

        input_character = getch();
        switch (input_character) {
            case KEY_UP:
                highlighted_option = (highlighted_option == 0) ? (number_of_options - 1) : (highlighted_option - 1);
                break;
            case KEY_DOWN:
                highlighted_option = (highlighted_option == number_of_options - 1) ? 0 : (highlighted_option + 1);
                break;
            case '\n':
            case '\r':
            case KEY_ENTER:
                chosen_option_value = option_values[highlighted_option];
                is_menu_active = false;
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
    (void)current_position;
    (void)previous_position;
    board.render(boardWin, players, playerIndex, current_position, has_colors());
}
