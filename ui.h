#pragma once

#include <ncurses.h>
#include <string>
#include <vector>

#include "board.hpp"
#include "player.hpp"
#include "rules.hpp"

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

struct GoldrushUiPosition {
    int x;
    int y;
};

void initialize_game_ui();
void destroy_game_ui();
void apply_ui_background(WINDOW* window);
void draw_board_ui(WINDOW* boardWin,
                   const Board& board,
                   const std::vector<Player>& players,
                   int currentPlayer,
                   int highlightedTile);
void draw_sidebar_ui(WINDOW* panelWin,
                     const Board& board,
                     const std::vector<Player>& players,
                     int currentPlayer,
                     const std::vector<std::string>& historyLines,
                     const RuleSet& rules);
void draw_message_ui(WINDOW* msgWin, const std::string& line1, const std::string& line2);
void draw_title_banner_ui(WINDOW* titleWin);
int selector_component(char* prompt_text, char** option_texts, int* option_values, int number_of_options, int default_value, int y_offset, int width, int height);
int choose_branch_with_selector(const std::string& prompt_title, const std::vector<std::string>& option_lines, const std::vector<int>& option_values, int default_value);
void update_position_highlights(WINDOW* boardWin, const Board& board, const std::vector<Player>& players, int current_position, int previous_position, int playerIndex);
int ui_player_color_pair(int playerIndex);
