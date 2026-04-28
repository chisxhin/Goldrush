#include <ncurses.h>
#include <locale.h>

// Copy just the color constants from ui.h
enum GoldrushUiColorPair {
    GOLDRUSH_GOLD_BLACK = 1,
    GOLDRUSH_BROWN_SAND = 2,
    GOLDRUSH_GOLD_FOREST = 3,
    GOLDRUSH_BLACK_GOLD = 4,
    GOLDRUSH_BLACK_TERRA = 5,
    GOLDRUSH_BROWN_CREAM = 6,
    GOLDRUSH_CHARCOAL_BLACK = 7,
    GOLDRUSH_GOLD_SAND = 8,
    GOLDRUSH_PLAYER_ONE = 9,
    GOLDRUSH_PLAYER_TWO = 10,
    GOLDRUSH_PLAYER_THREE = 11,
    GOLDRUSH_PLAYER_FOUR = 12,
    GOLDRUSH_BLACK_FOREST = 13,
    GOLDRUSH_BLACK_CREAM = 14,
    GOLDRUSH_GOLD_TERRA = 15
};

void initialize_game_ui() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(GOLDRUSH_GOLD_BLACK, COLOR_YELLOW, COLOR_BLACK);
        init_pair(GOLDRUSH_BROWN_SAND, COLOR_WHITE, COLOR_BLACK);
        init_pair(GOLDRUSH_GOLD_FOREST, COLOR_YELLOW, COLOR_BLACK);
        init_pair(GOLDRUSH_BLACK_GOLD, COLOR_YELLOW, COLOR_BLACK);
        init_pair(GOLDRUSH_BLACK_TERRA, COLOR_RED, COLOR_BLACK);
        init_pair(GOLDRUSH_BROWN_CREAM, COLOR_WHITE, COLOR_BLACK);
        init_pair(GOLDRUSH_CHARCOAL_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair(GOLDRUSH_GOLD_SAND, COLOR_YELLOW, COLOR_BLACK);
        init_pair(GOLDRUSH_PLAYER_ONE, COLOR_YELLOW, COLOR_BLACK);
        init_pair(GOLDRUSH_PLAYER_TWO, COLOR_CYAN, COLOR_BLACK);
        init_pair(GOLDRUSH_PLAYER_THREE, COLOR_RED, COLOR_BLACK);
        init_pair(GOLDRUSH_PLAYER_FOUR, COLOR_GREEN, COLOR_BLACK);
        init_pair(GOLDRUSH_BLACK_FOREST, COLOR_GREEN, COLOR_BLACK);
        init_pair(GOLDRUSH_BLACK_CREAM, COLOR_WHITE, COLOR_BLACK);
        init_pair(GOLDRUSH_GOLD_TERRA, COLOR_RED, COLOR_BLACK);
        bkgd(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    }
}

void destroy_game_ui() {
    endwin();
}

bool has_colors_ui() {
    return has_colors();
}
