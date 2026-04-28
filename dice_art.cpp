#include "dice_art.h"

#include <algorithm>

#include <ncurses.h>

#include "ui.h"
#include "ui_helpers.h"

std::vector<std::string> getAsciiDiceFace(int roll) {
    const bool topLeft = roll >= 4;
    const bool topRight = roll >= 2;
    const bool middleLeft = roll == 6;
    const bool center = roll % 2 == 1;
    const bool middleRight = roll == 6;
    const bool bottomLeft = roll >= 2;
    const bool bottomRight = roll >= 4;

    std::vector<std::string> face;
    face.push_back("+-----------+");
    face.push_back(std::string("|  ") + (topLeft ? "O" : " ") + "     " + (topRight ? "O" : " ") + "  |");
    face.push_back(std::string("|  ") + (middleLeft ? "O" : " ") + "  " + (center ? "O" : " ") + "  " + (middleRight ? "O" : " ") + "  |");
    face.push_back(std::string("|  ") + (bottomLeft ? "O" : " ") + "     " + (bottomRight ? "O" : " ") + "  |");
    face.push_back("+-----------+");
    return face;
}

void showDiceRollAnimation(const std::string& title, int roll, bool hasColor) {
    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    const int popupW = std::min(42, std::max(28, screenW - 4));
    const int popupH = 13;
    WINDOW* popup = newwin(popupH,
                           popupW,
                           std::max(0, (screenH - popupH) / 2),
                           std::max(0, (screenW - popupW) / 2));
    apply_ui_background(popup);
    keypad(popup, TRUE);

    const std::vector<std::string> face = getAsciiDiceFace(roll);
    for (int frame = 0; frame < 4; ++frame) {
        werase(popup);
        box(popup, 0, 0);
        if (hasColor) {
            wattron(popup, COLOR_PAIR(frame % 2 == 0 ? GOLDRUSH_GOLD_SAND : GOLDRUSH_PLAYER_TWO) | A_BOLD);
        }
        mvwprintw(popup, 1, std::max(2, (popupW - static_cast<int>(title.size())) / 2), "%s", title.c_str());
        for (std::size_t i = 0; i < face.size(); ++i) {
            mvwprintw(popup,
                      3 + static_cast<int>(i),
                      std::max(2, (popupW - static_cast<int>(face[i].size())) / 2),
                      "%s",
                      face[i].c_str());
        }
        mvwprintw(popup, 9, std::max(2, (popupW - 1) / 2), "%d", roll);
        if (hasColor) {
            wattroff(popup, COLOR_PAIR(frame % 2 == 0 ? GOLDRUSH_GOLD_SAND : GOLDRUSH_PLAYER_TWO) | A_BOLD);
        }
        wrefresh(popup);
        napms(150);
    }

    mvwprintw(popup, popupH - 2, 2, "Press ENTER to continue...");
    wrefresh(popup);
    waitForEnterPrompt(popup, popupH - 2, 2, "Press ENTER to continue...");
    delwin(popup);
    touchwin(stdscr);
    refresh();
}
