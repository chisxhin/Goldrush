#include "completed_history.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <ncurses.h>

#include "input_helpers.h"
#include "ui.h"
#include "ui_helpers.h"

namespace fs = std::filesystem;

namespace {
fs::path historyPath() {
    return fs::path("saves") / "completed_history.log";
}

std::string valueAfter(const std::string& line, const std::string& prefix) {
    if (line.rfind(prefix, 0) != 0) {
        return "";
    }
    return line.substr(prefix.size());
}
}

bool appendCompletedGameHistory(const CompletedGameEntry& entry, std::string& error) {
    std::error_code ec;
    fs::create_directories(historyPath().parent_path(), ec);
    if (ec) {
        error = "Could not create saves directory.";
        return false;
    }

    std::ofstream out(historyPath().string().c_str(), std::ios::app);
    if (!out) {
        error = "Could not write completed history.";
        return false;
    }

    out << "GAME_COMPLETED\n";
    out << "date=" << entry.date << "\n";
    out << "gameId=" << entry.gameId << "\n";
    out << "winner=" << entry.winner << "\n";
    out << "winnerScore=" << entry.winnerScore << "\n";
    out << "winnerCash=" << entry.winnerCash << "\n";
    out << "rounds=" << entry.rounds << "\n";
    out << "players=" << entry.players << "\n";
    out << "mode=" << entry.mode << "\n";
    out << "END\n";
    return true;
}

std::vector<CompletedGameEntry> loadCompletedGameHistory(std::string& error) {
    std::vector<CompletedGameEntry> entries;
    std::ifstream in(historyPath().string().c_str());
    if (!in) {
        return entries;
    }

    std::string line;
    CompletedGameEntry current;
    bool reading = false;
    while (std::getline(in, line)) {
        if (line == "GAME_COMPLETED") {
            current = CompletedGameEntry();
            reading = true;
            continue;
        }
        if (line == "END") {
            if (reading) {
                entries.push_back(current);
            }
            reading = false;
            continue;
        }
        if (!reading) {
            continue;
        }
        if (line.rfind("date=", 0) == 0) current.date = valueAfter(line, "date=");
        else if (line.rfind("gameId=", 0) == 0) current.gameId = valueAfter(line, "gameId=");
        else if (line.rfind("winner=", 0) == 0) current.winner = valueAfter(line, "winner=");
        else if (line.rfind("winnerScore=", 0) == 0) current.winnerScore = std::atoi(valueAfter(line, "winnerScore=").c_str());
        else if (line.rfind("winnerCash=", 0) == 0) current.winnerCash = std::atoi(valueAfter(line, "winnerCash=").c_str());
        else if (line.rfind("rounds=", 0) == 0) current.rounds = std::atoi(valueAfter(line, "rounds=").c_str());
        else if (line.rfind("players=", 0) == 0) current.players = valueAfter(line, "players=");
        else if (line.rfind("mode=", 0) == 0) current.mode = valueAfter(line, "mode=");
    }

    if (!in.eof()) {
        error = "Could not read completed history.";
    }
    return entries;
}

void showCompletedGameHistoryScreen(bool hasColor) {
    std::string error;
    const std::vector<CompletedGameEntry> entries = loadCompletedGameHistory(error);
    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    const int popupH = std::min(20, std::max(10, screenH - 2));
    const int popupW = std::min(86, std::max(48, screenW - 2));
    WINDOW* popup = newwin(popupH, popupW, (screenH - popupH) / 2, (screenW - popupW) / 2);
    apply_ui_background(popup);
    keypad(popup, TRUE);

    int selected = 0;
    while (true) {
        werase(popup);
        box(popup, 0, 0);
        if (hasColor) {
            wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }
        mvwprintw(popup, 1, 2, "COMPLETED GAME HISTORY");
        if (hasColor) {
            wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        if (!error.empty()) {
            mvwprintw(popup, 3, 2, "%s", error.c_str());
        } else if (entries.empty()) {
            mvwprintw(popup, 3, 2, "No completed games recorded yet.");
        } else {
            const int visible = popupH - 6;
            const int start = std::max(0, selected - visible + 1);
            for (int i = 0; i < visible && start + i < static_cast<int>(entries.size()); ++i) {
                const int index = start + i;
                const CompletedGameEntry& entry = entries[static_cast<std::size_t>(index)];
                std::ostringstream row;
                row << (index + 1) << ". " << entry.date << " - "
                    << entry.winner << " won with $" << entry.winnerScore;
                if (index == selected) {
                    wattron(popup, A_REVERSE);
                }
                mvwprintw(popup, 3 + i, 2, "%-*s", popupW - 4,
                          clipUiText(row.str(), static_cast<std::size_t>(popupW - 4)).c_str());
                if (index == selected) {
                    wattroff(popup, A_REVERSE);
                }
            }
            const CompletedGameEntry& entry = entries[static_cast<std::size_t>(selected)];
            mvwprintw(popup, popupH - 4, 2, "Mode: %s | Rounds: %d | Game: %s",
                      clipUiText(entry.mode, 16).c_str(),
                      entry.rounds,
                      clipUiText(entry.gameId, 22).c_str());
            mvwprintw(popup, popupH - 3, 2, "%s",
                      clipUiText(entry.players, static_cast<std::size_t>(popupW - 4)).c_str());
        }

        mvwprintw(popup, popupH - 2, 2, "Up/Down move  ESC back");
        wrefresh(popup);
        const int ch = wgetch(popup);
        if (isCancelKey(ch) || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            break;
        }
        if (!entries.empty() && ch == KEY_UP) {
            selected = selected == 0 ? static_cast<int>(entries.size()) - 1 : selected - 1;
        } else if (!entries.empty() && ch == KEY_DOWN) {
            selected = selected + 1 >= static_cast<int>(entries.size()) ? 0 : selected + 1;
        }
    }

    delwin(popup);
    touchwin(stdscr);
    refresh();
}
