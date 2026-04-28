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
    (void)screenH;
    int popupW = std::min(42, std::max(28, screenW - 4));
    int popupH = 13;
    WINDOW* popup = createCenteredWindow(popupH, popupW, 13, 28);
    if (!popup) {
        showTerminalSizeWarning(13, 28, hasColor);
        return;
    }
    keypad(popup, TRUE);
    getmaxyx(popup, popupH, popupW);
    const int contentW = std::max(1, popupW - 4);

    const std::vector<std::string> face = getAsciiDiceFace(roll);
    for (int frame = 0; frame < 4; ++frame) {
        werase(popup);
        drawBoxSafe(popup);
        if (hasColor) {
            wattron(popup, COLOR_PAIR(frame % 2 == 0 ? GOLDRUSH_GOLD_SAND : GOLDRUSH_PLAYER_TWO) | A_BOLD);
        }
        const std::string clippedTitle = clipUiText(title, static_cast<std::size_t>(contentW));
        mvwprintw(popup, 1, 2 + std::max(0, (contentW - static_cast<int>(clippedTitle.size())) / 2),
                  "%s", clippedTitle.c_str());
        for (std::size_t i = 0; i < face.size(); ++i) {
            const std::string line = clipUiText(face[i], static_cast<std::size_t>(contentW));
            mvwprintw(popup,
                      3 + static_cast<int>(i),
                      2 + std::max(0, (contentW - static_cast<int>(line.size())) / 2),
                      "%s",
                      line.c_str());
        }
        mvwprintw(popup, 9, 2 + std::max(0, (contentW - 1) / 2), "%d", roll);
        if (hasColor) {
            wattroff(popup, COLOR_PAIR(frame % 2 == 0 ? GOLDRUSH_GOLD_SAND : GOLDRUSH_PLAYER_TWO) | A_BOLD);
        }
        wrefresh(popup);
        napms(150);
    }

    mvwprintw(popup, popupH - 2, 2, "%s",
              clipUiText("Press ENTER to continue...", static_cast<std::size_t>(contentW)).c_str());
    wrefresh(popup);
    waitForEnterPrompt(popup, popupH - 2, 2, "");
    delwin(popup);
    touchwin(stdscr);
    refresh();
}
