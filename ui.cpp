#include "ui.h"

#include <algorithm>
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
    init_pair(GOLDRUSH_PLAYER_TWO, COLOR_CYAN, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_THREE, terra, COLOR_BLACK);
    init_pair(GOLDRUSH_PLAYER_FOUR, forest, COLOR_BLACK);
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
}  // namespace

void initialize_game_ui() {
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

void draw_board_ui(WINDOW* boardWin, const Board& board, const std::vector<Player>& players, int highlightedTile) {
    (void)highlightedTile;
    board.render(boardWin, players, has_colors());
}

void draw_sidebar_ui(WINDOW* panelWin,
                     const std::vector<Player>& players,
                     int currentPlayer,
                     const std::vector<std::string>& historyLines,
                     const RuleSet& rules) {
    werase(panelWin);
    box(panelWin, 0, 0);

    wattron(panelWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    mvwprintw(panelWin, 1, 2, "PLAYERS");
    mvwprintw(panelWin, 15, 2, "HISTORY");
    mvwprintw(panelWin, 23, 2, "MODE");
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);

    int row = 2;
    for (size_t i = 0; i < players.size(); ++i) {
        const Player& player = players[i];
        std::string marker = (static_cast<int>(i) == currentPlayer) ? ">" : " ";
        std::string home = player.retirementHome.empty() ? "--" : player.retirementHome;
        std::string invest = player.investedNumber > 0 ? std::to_string(player.investedNumber) : "-";

        wattron(panelWin, COLOR_PAIR(ui_player_color_pair(static_cast<int>(i))) | A_BOLD);
        mvwprintw(panelWin, row, 2, "%sP%d %.12s [%c]", marker.c_str(), static_cast<int>(i) + 1, player.name.c_str(), player.token);
        wattroff(panelWin, COLOR_PAIR(ui_player_color_pair(static_cast<int>(i))) | A_BOLD);

        wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        mvwprintw(panelWin, row + 1, 2, "$%d L:%d S:%s K:%d P:%d", player.cash, player.loans, player.married ? "Y" : "N", player.kids, static_cast<int>(player.petCards.size()));
        mvwprintw(panelWin, row + 2, 2, "J:%-.10s I:%s H:%-.8s", player.job.c_str(), invest.c_str(), home.c_str());
        wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        row += 3;
    }

    wattron(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    for (size_t i = 0; i < historyLines.size() && i < 6; ++i) {
        mvwprintw(panelWin, 16 + static_cast<int>(i), 2, "%-.34s", historyLines[i].c_str());
    }

    mvwprintw(panelWin, 24, 2, "%-.34s", rules.editionName.c_str());
    mvwprintw(panelWin, 25, 2, "Tut:%s Inv:%s Pets:%s",
              rules.toggles.tutorialEnabled ? "Y" : "N",
              rules.toggles.investmentEnabled ? "Y" : "N",
              rules.toggles.petsEnabled ? "Y" : "N");
    mvwprintw(panelWin, 26, 2, "Risk:%s Night:%s Spin:%s",
              rules.toggles.riskyRouteEnabled ? "Y" : "N",
              rules.toggles.nightSchoolEnabled ? "Y" : "N",
              rules.toggles.spinToWinEnabled ? "Y" : "N");
    wattroff(panelWin, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));

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
    (void)playerIndex;
    board.render(boardWin, players, has_colors());
}
