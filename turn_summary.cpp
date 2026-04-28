#include "turn_summary.h"

#include <algorithm>
#include <cstdlib>

#include <ncurses.h>

#include "ui.h"
#include "ui_helpers.h"

namespace {
struct SummaryLine {
    std::string label;
    std::string value;
    int colorPair;
};

void centerPrint(WINDOW* win, int y, const std::string& text, int attrs = A_NORMAL) {
    int h = 0;
    int w = 0;
    getmaxyx(win, h, w);
    (void)h;
    if (attrs != A_NORMAL) {
        wattron(win, attrs);
    }
    mvwprintw(win, y, std::max(1, (w - static_cast<int>(text.size())) / 2), "%s", text.c_str());
    if (attrs != A_NORMAL) {
        wattroff(win, attrs);
    }
}

void addChange(std::vector<SummaryLine>& lines,
               int amount,
               const std::string& gainLabel,
               const std::string& lossLabel,
               const std::string& unit,
               int gainColor,
               int lossColor) {
    if (amount == 0) {
        return;
    }
    lines.push_back({
        amount > 0 ? gainLabel : lossLabel,
        formatSignedChange(amount, unit),
        amount > 0 ? gainColor : lossColor
    });
}

void addText(std::vector<SummaryLine>& lines,
             const std::string& label,
             const std::string& value,
             int colorPair) {
    if (!value.empty()) {
        lines.push_back({label, value, colorPair});
    }
}

void drawSummarySection(WINDOW* win, int& y, const SummaryLine& line, int width) {
    if (y + 2 >= getmaxy(win)) {
        return;
    }
    wattron(win, COLOR_PAIR(line.colorPair) | A_BOLD);
    mvwprintw(win, y, 3, "%s", clipUiText(line.label, static_cast<std::size_t>(std::max(8, width - 6))).c_str());
    wattroff(win, COLOR_PAIR(line.colorPair) | A_BOLD);
    mvwprintw(win,
              y + 1,
              5,
              "%s",
              clipUiText(line.value, static_cast<std::size_t>(std::max(8, width - 10))).c_str());
    y += 3;
}
}

std::string formatMoney(int amount) {
    const int absolute = std::abs(amount);
    return std::string(amount < 0 ? "-$" : "$") + std::to_string(absolute);
}

std::string formatSignedChange(int amount, const std::string& unit) {
    const std::string sign = amount > 0 ? "+" : "-";
    return sign + std::to_string(std::abs(amount)) + (unit.empty() ? "" : " " + unit);
}

void showTurnSummaryReport(const TurnSummary& summary, bool hasColor) {
    std::vector<SummaryLine> lines;
    addChange(lines, summary.moneyChange, "MONEY GAIN", "MONEY LOSS", "cash", GOLDRUSH_BLACK_FOREST, GOLDRUSH_GOLD_TERRA);
    addChange(lines, summary.loanChange, "LOAN GAIN", "LOAN LOSS", "loan", GOLDRUSH_GOLD_TERRA, GOLDRUSH_BLACK_FOREST);
    addChange(lines, summary.babyChange, "BABY GAIN", "BABY LOSS", "baby", GOLDRUSH_PLAYER_TWO, GOLDRUSH_GOLD_TERRA);
    addChange(lines, summary.petChange, "PET GAIN", "PET LOSS", "pet", GOLDRUSH_TILE_MINIGAME, GOLDRUSH_GOLD_TERRA);
    addChange(lines, summary.investmentChange, "INVESTMENT GAIN", "INVESTMENT LOSS", "cash", GOLDRUSH_BLACK_FOREST, GOLDRUSH_GOLD_TERRA);
    addChange(lines, summary.shieldChange, "SHIELD GAIN", "SHIELD LOSS", "shield", GOLDRUSH_GOLD_SAND, GOLDRUSH_GOLD_TERRA);
    addChange(lines, summary.insuranceChange, "INSURANCE GAIN", "INSURANCE LOSS", "use", GOLDRUSH_GOLD_SAND, GOLDRUSH_GOLD_TERRA);

    if (summary.gotMarried) {
        addText(lines, "MARRIAGE EVENT", summary.playerName + " got married and begins a new chapter.", GOLDRUSH_GOLD_SAND);
    }
    if (summary.jobChanged) {
        addText(lines, "JOB CHANGE", summary.oldJob + " -> " + summary.newJob, GOLDRUSH_TILE_CAREER);
    }
    if (summary.houseChanged) {
        addText(lines, "HOUSE CHANGE", summary.oldHouse.empty() ? summary.newHouse : summary.oldHouse + " -> " + summary.newHouse, GOLDRUSH_TILE_HOME);
    }
    for (std::size_t i = 0; i < summary.cardsGained.size(); ++i) {
        addText(lines, "CARD GAIN", summary.cardsGained[i], GOLDRUSH_TILE_ACTION);
    }
    for (std::size_t i = 0; i < summary.cardsUsed.size(); ++i) {
        addText(lines, "CARD USED", summary.cardsUsed[i], GOLDRUSH_TILE_ACTION);
    }
    for (std::size_t i = 0; i < summary.minigameResults.size(); ++i) {
        addText(lines, "MINIGAME RESULT", summary.minigameResults[i], GOLDRUSH_TILE_MINIGAME);
    }
    for (std::size_t i = 0; i < summary.sabotageEvents.size(); ++i) {
        addText(lines, "SABOTAGE EVENT", summary.sabotageEvents[i], GOLDRUSH_GOLD_TERRA);
    }
    for (std::size_t i = 0; i < summary.importantEvents.size(); ++i) {
        addText(lines, "IMPORTANT EVENT", summary.importantEvents[i], GOLDRUSH_GOLD_SAND);
    }
    if (lines.empty()) {
        addText(lines, "IMPORTANT EVENT", "No tracked stat changes this turn.", GOLDRUSH_BROWN_CREAM);
    }

    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    const int popupW = std::min(76, std::max(48, screenW - 4));
    const int popupH = std::min(std::max(18, 8 + static_cast<int>(lines.size()) * 3), std::max(12, screenH - 2));
    WINDOW* popup = newwin(popupH,
                           popupW,
                           std::max(0, (screenH - popupH) / 2),
                           std::max(0, (screenW - popupW) / 2));
    apply_ui_background(popup);
    keypad(popup, TRUE);
    werase(popup);
    box(popup, 0, 0);

    if (hasColor) {
        wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    centerPrint(popup, 1, " _____ _   _ __  __ __  __    _    ______   __", A_BOLD);
    centerPrint(popup, 2, "/  ___| | | |  \\/  |  \\/  |  / \\  |  _ \\ \\ / /", A_BOLD);
    centerPrint(popup, 3, "\\___ \\| | | | |\\/| | |\\/| | / _ \\ | |_) \\ V /", A_BOLD);
    centerPrint(popup, 4, " ___) | |_| | |  | | |  | |/ ___ \\|  _ < | |", A_BOLD);
    centerPrint(popup, 5, "|____/ \\___/|_|  |_|_|  |_/_/   \\_\\_| \\_\\|_|", A_BOLD);
    if (hasColor) {
        wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    centerPrint(popup,
                7,
                summary.playerName + " End-of-Turn " + std::to_string(summary.turnNumber) + " Summary",
                A_BOLD);

    int y = 9;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        drawSummarySection(popup, y, lines[i], popupW);
    }

    mvwprintw(popup, popupH - 2, 3, "Press ENTER to continue...");
    wrefresh(popup);
    waitForEnterPrompt(popup, popupH - 2, 3, "Press ENTER to continue...");
    delwin(popup);
    touchwin(stdscr);
    refresh();
}
