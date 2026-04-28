#include "game_settings.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <vector>

#include <ncurses.h>

#include "input_helpers.h"
#include "ui.h"
#include "ui_helpers.h"

namespace {
int clampInt(int value, int low, int high) {
    return std::max(low, std::min(value, high));
}

std::string onOff(bool value) {
    return value ? "ON" : "OFF";
}

bool editNumericSetting(const std::string& label, int& value, int minValue, int maxValue, bool hasColor) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    const int popupW = 58;
    const int popupH = 7;
    WINDOW* popup = newwin(popupH, popupW, std::max(0, (h - popupH) / 2), std::max(0, (w - popupW) / 2));
    apply_ui_background(popup);
    keypad(popup, TRUE);
    echo();
    curs_set(1);

    char buffer[32] = {0};
    box(popup, 0, 0);
    if (hasColor) wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    mvwprintw(popup, 1, 2, "%s", clipUiText(label, 52).c_str());
    if (hasColor) wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    mvwprintw(popup, 2, 2, "Current: %d", value);
    mvwprintw(popup, 3, 2, "Range: %d - %d", minValue, maxValue);
    mvwprintw(popup, 5, 2, "New value: ");
    wrefresh(popup);
    wgetnstr(popup, buffer, 31);

    noecho();
    curs_set(0);
    delwin(popup);

    if (buffer[0] == '\0') {
        return false;
    }
    value = clampInt(std::atoi(buffer), minValue, maxValue);
    return true;
}

void drawSettingsLine(WINDOW* win, int y, int index, const std::string& label, bool selected) {
    if (selected) wattron(win, A_REVERSE);
    mvwprintw(win, y, 2, "%2d. %-68s", index + 1, clipUiText(label, 68).c_str());
    if (selected) wattroff(win, A_REVERSE);
}
}

GameSettings createRelaxModeSettings() {
    GameSettings settings;
    settings.modeName = "Relax Mode";
    settings.minJobSalary = 30000;
    settings.maxJobSalary = 140000;
    settings.collegeCost = 80000;
    settings.startingCash = 50000;
    settings.loanAmount = 50000;
    settings.loanPenalty = 50000;
    settings.maxLoans = 99;
    settings.paydayMultiplierPercent = 125;
    settings.taxMultiplierPercent = 75;
    settings.eventRewardMultiplierPercent = 125;
    settings.eventPenaltyMultiplierPercent = 75;
    settings.investmentMinReturnPercent = 110;
    settings.investmentMaxReturnPercent = 130;
    settings.babyCost = 0;
    settings.petCost = 0;
    settings.houseMinCost = 0;
    settings.houseMaxCost = 900000;
    settings.minigameReward = 150;
    settings.minigamePenalty = 0;
    settings.minigameDifficultyModifier = -10;
    settings.sabotageUnlockTurn = 4;
    settings.sabotagePenaltyMultiplierPercent = 75;
    return settings;
}

GameSettings createLifeModeSettings() {
    return GameSettings();
}

GameSettings createHellModeSettings() {
    GameSettings settings;
    settings.modeName = "Hell Mode";
    settings.minJobSalary = 15000;
    settings.maxJobSalary = 100000;
    settings.collegeCost = 120000;
    settings.startingCash = 5000;
    settings.loanAmount = 50000;
    settings.loanPenalty = 75000;
    settings.maxLoans = 10;
    settings.paydayMultiplierPercent = 75;
    settings.taxMultiplierPercent = 150;
    settings.eventRewardMultiplierPercent = 75;
    settings.eventPenaltyMultiplierPercent = 150;
    settings.investmentMinReturnPercent = 70;
    settings.investmentMaxReturnPercent = 90;
    settings.babyCost = 10000;
    settings.petCost = 5000;
    settings.houseMinCost = 50000;
    settings.houseMaxCost = 650000;
    settings.minigameReward = 75;
    settings.minigamePenalty = 5000;
    settings.minigameDifficultyModifier = 15;
    settings.sabotageUnlockTurn = 2;
    settings.sabotagePenaltyMultiplierPercent = 150;
    return settings;
}

void validateGameSettings(GameSettings& settings) {
    settings.minJobSalary = clampInt(settings.minJobSalary, 0, 1000000);
    settings.maxJobSalary = clampInt(settings.maxJobSalary, settings.minJobSalary, 1500000);
    settings.collegeCost = clampInt(settings.collegeCost, 0, 1000000);
    settings.startingCash = clampInt(settings.startingCash, 0, 1000000);
    settings.loanAmount = clampInt(settings.loanAmount, 1000, 500000);
    settings.loanPenalty = clampInt(settings.loanPenalty, settings.loanAmount, 750000);
    settings.maxLoans = clampInt(settings.maxLoans, 0, 999);
    settings.paydayMultiplierPercent = clampInt(settings.paydayMultiplierPercent, 10, 500);
    settings.taxMultiplierPercent = clampInt(settings.taxMultiplierPercent, 10, 500);
    settings.eventRewardMultiplierPercent = clampInt(settings.eventRewardMultiplierPercent, 10, 500);
    settings.eventPenaltyMultiplierPercent = clampInt(settings.eventPenaltyMultiplierPercent, 10, 500);
    settings.investmentMinReturnPercent = clampInt(settings.investmentMinReturnPercent, 0, 500);
    settings.investmentMaxReturnPercent = clampInt(settings.investmentMaxReturnPercent,
                                                   settings.investmentMinReturnPercent,
                                                   500);
    settings.babyCost = clampInt(settings.babyCost, 0, 500000);
    settings.petCost = clampInt(settings.petCost, 0, 500000);
    settings.houseMinCost = clampInt(settings.houseMinCost, 0, 1000000);
    settings.houseMaxCost = clampInt(settings.houseMaxCost, settings.houseMinCost, 2000000);
    settings.minigameReward = clampInt(settings.minigameReward, 0, 100000);
    settings.minigamePenalty = clampInt(settings.minigamePenalty, 0, 1000000);
    settings.minigameDifficultyModifier = clampInt(settings.minigameDifficultyModifier, -50, 50);
    settings.sabotageUnlockTurn = clampInt(settings.sabotageUnlockTurn, 1, 99);
    settings.sabotagePenaltyMultiplierPercent = clampInt(settings.sabotagePenaltyMultiplierPercent, 10, 500);
}

void applyGameSettingsToRules(const GameSettings& settings, RuleSet& rules) {
    rules.editionName = settings.modeName;
    rules.loanUnit = settings.loanAmount;
    rules.loanRepaymentCost = settings.loanPenalty;
    rules.maxLoans = settings.maxLoans;
    rules.automaticLoansEnabled = settings.allowAutomaticLoans;
    rules.investmentMatchPayout = (25000 * settings.investmentMaxReturnPercent) / 100;
    rules.toggles.investmentEnabled = settings.allowInvestments;
    rules.toggles.petsEnabled = settings.allowPets;
    rules.components.investCards = settings.allowInvestments ? 4 : 0;
    rules.components.petCards = settings.allowPets ? 12 : 0;
}

std::string gameSettingsSummary(const GameSettings& settings) {
    std::ostringstream out;
    out << settings.modeName
        << " | Salaries $" << settings.minJobSalary << "-$" << settings.maxJobSalary
        << " | College $" << settings.collegeCost
        << " | Start $" << settings.startingCash
        << " | Loans $" << settings.loanAmount << "/$" << settings.loanPenalty;
    return out.str();
}

bool showCustomSettingsMenu(GameSettings& settings, bool hasColor) {
    validateGameSettings(settings);
    int selected = 0;

    while (true) {
        int h = 0;
        int w = 0;
        getmaxyx(stdscr, h, w);
        const int popupW = std::min(86, std::max(72, w - 6));
        const int popupH = std::min(24, std::max(20, h - 4));
        WINDOW* popup = newwin(popupH, popupW, std::max(0, (h - popupH) / 2), std::max(0, (w - popupW) / 2));
        apply_ui_background(popup);
        keypad(popup, TRUE);

        bool redraw = true;
        while (redraw) {
            werase(popup);
            box(popup, 0, 0);
            if (hasColor) wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
            mvwprintw(popup, 1, 2, "CUSTOM MODE SETTINGS");
            if (hasColor) wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);

            std::vector<std::string> rows;
            rows.push_back("Minimum Job Salary: $" + std::to_string(settings.minJobSalary));
            rows.push_back("Maximum Job Salary: $" + std::to_string(settings.maxJobSalary));
            rows.push_back("College Cost: $" + std::to_string(settings.collegeCost));
            rows.push_back("Starting Cash: $" + std::to_string(settings.startingCash));
            rows.push_back("Loan Amount: $" + std::to_string(settings.loanAmount));
            rows.push_back("Loan Penalty: $" + std::to_string(settings.loanPenalty));
            rows.push_back("Max Loans: " + std::to_string(settings.maxLoans));
            rows.push_back("Payday Multiplier: " + std::to_string(settings.paydayMultiplierPercent) + "%");
            rows.push_back("Tax Multiplier: " + std::to_string(settings.taxMultiplierPercent) + "%");
            rows.push_back("Event Reward Multiplier: " + std::to_string(settings.eventRewardMultiplierPercent) + "%");
            rows.push_back("Event Penalty Multiplier: " + std::to_string(settings.eventPenaltyMultiplierPercent) + "%");
            rows.push_back("Minigame Reward Unit: $" + std::to_string(settings.minigameReward));
            rows.push_back("Sabotage Unlock Turn: " + std::to_string(settings.sabotageUnlockTurn));
            rows.push_back("Automatic Loans: " + onOff(settings.allowAutomaticLoans));
            rows.push_back("Sabotage: " + onOff(settings.allowSabotage));
            rows.push_back("Investments: " + onOff(settings.allowInvestments));
            rows.push_back("Pets: " + onOff(settings.allowPets));
            rows.push_back("Random Events: " + onOff(settings.allowRandomEvents));

            const int visible = popupH - 6;
            const int top = std::max(0, std::min(selected - visible + 1,
                                                 static_cast<int>(rows.size()) - visible));
            for (int i = 0; i < visible && top + i < static_cast<int>(rows.size()); ++i) {
                drawSettingsLine(popup, 3 + i, top + i, rows[static_cast<std::size_t>(top + i)],
                                 top + i == selected);
            }

            mvwprintw(popup, popupH - 3, 2, "ENTER edit/toggle  S start  1 Relax  2 Life  3 Hell  ESC back");
            mvwprintw(popup, popupH - 2, 2, "%s", clipUiText(gameSettingsSummary(settings),
                                                             static_cast<std::size_t>(popupW - 4)).c_str());
            wrefresh(popup);

            const int ch = wgetch(popup);
            if (ch == KEY_UP) {
                selected = selected == 0 ? static_cast<int>(rows.size()) - 1 : selected - 1;
            } else if (ch == KEY_DOWN) {
                selected = selected + 1 >= static_cast<int>(rows.size()) ? 0 : selected + 1;
            } else if (ch == '1') {
                settings = createRelaxModeSettings();
                settings.customMode = true;
                selected = 0;
            } else if (ch == '2') {
                settings = createLifeModeSettings();
                settings.customMode = true;
                settings.modeName = "Custom Mode";
                selected = 0;
            } else if (ch == '3') {
                settings = createHellModeSettings();
                settings.customMode = true;
                settings.modeName = "Custom Mode";
                selected = 0;
            } else if (ch == 's' || ch == 'S') {
                validateGameSettings(settings);
                delwin(popup);
                return true;
            } else if (isCancelKey(ch)) {
                delwin(popup);
                return false;
            } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == ' ') {
                bool toggled = false;
                if (selected == 13) { settings.allowAutomaticLoans = !settings.allowAutomaticLoans; toggled = true; }
                else if (selected == 14) { settings.allowSabotage = !settings.allowSabotage; toggled = true; }
                else if (selected == 15) { settings.allowInvestments = !settings.allowInvestments; toggled = true; }
                else if (selected == 16) { settings.allowPets = !settings.allowPets; toggled = true; }
                else if (selected == 17) { settings.allowRandomEvents = !settings.allowRandomEvents; toggled = true; }

                if (!toggled) {
                    delwin(popup);
                    if (selected == 0) editNumericSetting("Minimum Job Salary", settings.minJobSalary, 0, 1000000, hasColor);
                    else if (selected == 1) editNumericSetting("Maximum Job Salary", settings.maxJobSalary, 0, 1500000, hasColor);
                    else if (selected == 2) editNumericSetting("College Cost", settings.collegeCost, 0, 1000000, hasColor);
                    else if (selected == 3) editNumericSetting("Starting Cash", settings.startingCash, 0, 1000000, hasColor);
                    else if (selected == 4) editNumericSetting("Loan Amount", settings.loanAmount, 1000, 500000, hasColor);
                    else if (selected == 5) editNumericSetting("Loan Penalty", settings.loanPenalty, 1000, 750000, hasColor);
                    else if (selected == 6) editNumericSetting("Max Loans", settings.maxLoans, 0, 999, hasColor);
                    else if (selected == 7) editNumericSetting("Payday Multiplier", settings.paydayMultiplierPercent, 10, 500, hasColor);
                    else if (selected == 8) editNumericSetting("Tax Multiplier", settings.taxMultiplierPercent, 10, 500, hasColor);
                    else if (selected == 9) editNumericSetting("Event Reward Multiplier", settings.eventRewardMultiplierPercent, 10, 500, hasColor);
                    else if (selected == 10) editNumericSetting("Event Penalty Multiplier", settings.eventPenaltyMultiplierPercent, 10, 500, hasColor);
                    else if (selected == 11) editNumericSetting("Minigame Reward Unit", settings.minigameReward, 0, 100000, hasColor);
                    else if (selected == 12) editNumericSetting("Sabotage Unlock Turn", settings.sabotageUnlockTurn, 1, 99, hasColor);
                    settings.customMode = true;
                    settings.modeName = "Custom Mode";
                    validateGameSettings(settings);
                    redraw = false;
                } else {
                    settings.customMode = true;
                    settings.modeName = "Custom Mode";
                    validateGameSettings(settings);
                }
            }
        }
    }
}

GameSettings createCustomSettingsFromMenu(bool hasColor) {
    GameSettings settings = createLifeModeSettings();
    settings.customMode = true;
    settings.modeName = "Custom Mode";
    showCustomSettingsMenu(settings, hasColor);
    validateGameSettings(settings);
    return settings;
}

bool showGameModeMenu(GameSettings& settings, bool hasColor) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    const int popupW = 56;
    const int popupH = 12;
    WINDOW* popup = newwin(popupH, popupW, std::max(0, (h - popupH) / 2), std::max(0, (w - popupW) / 2));
    apply_ui_background(popup);
    keypad(popup, TRUE);
    int selected = 0;

    while (true) {
        werase(popup);
        box(popup, 0, 0);
        if (hasColor) wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        mvwprintw(popup, 1, 2, "Choose Game Mode");
        if (hasColor) wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        const char* modes[] = {"Relax Mode", "Life Mode", "Hell Mode", "Custom Mode"};
        for (int i = 0; i < 4; ++i) {
            if (i == selected) wattron(popup, A_REVERSE);
            mvwprintw(popup, 3 + i, 4, "%d. %s", i + 1, modes[i]);
            if (i == selected) wattroff(popup, A_REVERSE);
        }
        mvwprintw(popup, 9, 2, "ENTER select  ESC back");
        wrefresh(popup);

        const int ch = wgetch(popup);
        if (ch == KEY_UP) selected = selected == 0 ? 3 : selected - 1;
        else if (ch == KEY_DOWN) selected = selected == 3 ? 0 : selected + 1;
        else if (ch >= '1' && ch <= '4') selected = ch - '1';
        else if (isCancelKey(ch)) {
            delwin(popup);
            return false;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            delwin(popup);
            if (selected == 0) settings = createRelaxModeSettings();
            else if (selected == 1) settings = createLifeModeSettings();
            else if (selected == 2) settings = createHellModeSettings();
            else {
                settings = createLifeModeSettings();
                settings.customMode = true;
                settings.modeName = "Custom Mode";
                if (!showCustomSettingsMenu(settings, hasColor)) {
                    return false;
                }
            }
            validateGameSettings(settings);
            return true;
        }
    }
}
