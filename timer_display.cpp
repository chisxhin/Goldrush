#include "timer_display.h"

#include "ui.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

std::string countdownTimerText(int remainingSeconds) {
    return "Time Remaining: " + std::to_string(std::max(0, remainingSeconds));
}

std::string countdownTimerText(double remainingSeconds) {
    std::ostringstream out;
    out << "Time Remaining: " << std::fixed << std::setprecision(1)
        << std::max(0.0, remainingSeconds) << "s";
    return out.str();
}

void drawCountdownTimer(WINDOW* win, int y, int x, int remainingSeconds, bool hasColor) {
    if (!win) {
        return;
    }
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    } else {
        wattron(win, A_BOLD);
    }
    mvwprintw(win, y, x, "%s", countdownTimerText(remainingSeconds).c_str());
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    } else {
        wattroff(win, A_BOLD);
    }
}

void drawCountdownTimer(WINDOW* win, int y, int x, double remainingSeconds, bool hasColor) {
    if (!win) {
        return;
    }
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    } else {
        wattron(win, A_BOLD);
    }
    mvwprintw(win, y, x, "%s", countdownTimerText(remainingSeconds).c_str());
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    } else {
        wattroff(win, A_BOLD);
    }
}

void displayCountdownTimer(int seconds, bool hasColor) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    const int popupH = 7;
    const int popupW = 42;
    WINDOW* popup = newwin(popupH,
                           popupW,
                           std::max(0, (h - popupH) / 2),
                           std::max(0, (w - popupW) / 2));
    apply_ui_background(popup);

    for (int remaining = seconds; remaining >= 0; --remaining) {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "Countdown Timer Test");
        drawCountdownTimer(popup, 3, 2, remaining, hasColor);
        wrefresh(popup);
        napms(remaining == 0 ? 700 : 1000);
    }

    delwin(popup);
    touchwin(stdscr);
    refresh();
}
