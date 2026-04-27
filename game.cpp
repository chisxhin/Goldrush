#include "game.hpp"
#include "battleship.hpp"
#include "pong.hpp"
#include "hangman.hpp"
#include "memory.hpp"
#include "minesweeper.hpp"
#include "save_manager.hpp"
#include "spins.hpp"
#include "ui.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace {
std::string appendLoanText(const std::string& base, const PaymentResult& payment) {
    if (payment.loansTaken <= 0) {
        return base;
    }

    std::ostringstream out;
    out << base << " Auto-loan +" << payment.loansTaken << ".";
    return out.str();
}

std::string clipMenuText(const std::string& text, std::size_t width) {
    if (text.size() <= width) {
        return text;
    }
    if (width <= 3) {
        return text.substr(0, width);
    }
    return text.substr(0, width - 3) + "...";
}

std::string formatSaveSize(std::uintmax_t sizeBytes) {
    std::ostringstream out;
    if (sizeBytes >= 1024ULL * 1024ULL) {
        out << (sizeBytes / (1024ULL * 1024ULL)) << " MB";
    } else if (sizeBytes >= 1024ULL) {
        out << (sizeBytes / 1024ULL) << " KB";
    } else {
        out << sizeBytes << " B";
    }
    return out.str();
}

std::string displayNameFromPath(const std::string& path) {
    const std::string::size_type pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

std::string saveMenuStatusText(const SaveFileInfo& info) {
    if (!info.metadataValid) {
        return "! Save metadata is missing or invalid. Loading may fail.";
    }
    if (info.duplicateGameId && !info.assignedTarget) {
        return "* Duplicate copy. Future saves will overwrite " + info.assignedFilename + ".";
    }
    if (info.duplicateGameId) {
        return "Canonical save for " + info.gameId + ". Duplicate copies exist.";
    }
    if (!info.gameId.empty()) {
        return "Game " + info.gameId + " | Created " + info.createdText + ".";
    }
    return "Legacy save without a persistent game ID.";
}

std::string tileKindText(TileKind kind) {
    switch (kind) {
        case TILE_BLACK: return "Action";
        case TILE_START: return "Start";
        case TILE_SPLIT_START: return "Start split";
        case TILE_COLLEGE: return "College";
        case TILE_CAREER: return "Career";
        case TILE_GRADUATION: return "Graduation";
        case TILE_MARRIAGE: return "Marriage";
        case TILE_SPLIT_FAMILY: return "Family split";
        case TILE_FAMILY: return "Family";
        case TILE_NIGHT_SCHOOL: return "Night School";
        case TILE_SPLIT_RISK: return "Risk split";
        case TILE_SAFE: return "Safe";
        case TILE_RISKY: return "Risky";
        case TILE_SPIN_AGAIN: return "Spin Again";
        case TILE_CAREER_2: return "Promotion";
        case TILE_PAYDAY: return "Payday";
        case TILE_BABY: return "Baby";
        case TILE_RETIREMENT: return "Retirement";
        case TILE_HOUSE: return "House";
        case TILE_EMPTY:
        default:
            return "Open road";
    }
}

std::string playerRouteText(const Player& player) {
    std::string route;
    if (player.startChoice == 0) {
        route = "College";
    } else if (player.startChoice == 1) {
        route = "Career";
    } else {
        route = "Undecided";
    }

    if (player.familyChoice == 0) {
        route += " / Family";
    } else if (player.familyChoice == 1) {
        route += " / Life";
    }

    if (player.riskChoice == 0) {
        route += " / Safe";
    } else if (player.riskChoice == 1) {
        route += " / Risky";
    }

    return route;
}

void drawPopupPulse(WINDOW* popup,
                    int y,
                    int x,
                    const std::string& text,
                    int colorPair,
                    bool hasColor) {
    const int attrs = A_BOLD | A_BLINK;
    if (hasColor) {
        wattron(popup, COLOR_PAIR(colorPair) | attrs);
    } else {
        wattron(popup, A_REVERSE | A_BOLD);
    }
    mvwprintw(popup, y, x, "%s", text.c_str());
    if (hasColor) {
        wattroff(popup, COLOR_PAIR(colorPair) | attrs);
    } else {
        wattroff(popup, A_REVERSE | A_BOLD);
    }
}
}

Game::Game()
    : rules(makeNormalRules()),
      rng(),
      cpu(rng),
      decks(rules, rng),
      bank(rules),
      sabotage(bank, rng),
      history(6),
      titleWin(nullptr),
      boardWin(nullptr),
      infoWin(nullptr),
      msgWin(nullptr),
      hasColor(has_colors()),
      retiredCount(0),
      currentPlayerIndex(0),
      turnCounter(0),
      gameId(),
      assignedSaveFilename(),
      createdTime(0),
      lastSavedTime(0),
      autoAdvanceUi(false),
      activeTraps() {
}

Game::Game(std::uint32_t seed)
    : rules(makeNormalRules()),
      rng(seed),
      cpu(rng),
      decks(rules, rng),
      bank(rules),
      sabotage(bank, rng),
      history(6),
      titleWin(nullptr),
      boardWin(nullptr),
      infoWin(nullptr),
      msgWin(nullptr),
      hasColor(has_colors()),
      retiredCount(0),
      currentPlayerIndex(0),
      turnCounter(0),
      gameId(),
      assignedSaveFilename(),
      createdTime(0),
      lastSavedTime(0),
      autoAdvanceUi(false),
      activeTraps() {
}

Game::~Game() {
    destroyWindows();
}

void Game::applyWindowBg(WINDOW* w) const {
    apply_ui_background(w);
}

void Game::addHistory(const std::string& entry) {
    history.add(entry);
}

bool Game::ensureMinSize() const {
    int h, w;
    timeout(200);
    while (true) {
        getmaxyx(stdscr, h, w);
        if (h >= MIN_H && w >= MIN_W) {
            timeout(-1);
            return true;
        }

        if (hasColor) {
            bkgd(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        }
        clear();
        const char* line1 = "Terminal too small - please resize";
        const char* line2 = "Minimum size: 122x40";
        const char* line3 = "Press Q to quit";
        int x1 = (w - static_cast<int>(std::strlen(line1))) / 2;
        int x2 = (w - static_cast<int>(std::strlen(line2))) / 2;
        int x3 = (w - static_cast<int>(std::strlen(line3))) / 2;
        int y = h / 2;
        if (x1 < 0) x1 = 0;
        if (x2 < 0) x2 = 0;
        if (x3 < 0) x3 = 0;
        mvprintw(y - 1, x1, "%s", line1);
        mvprintw(y, x2, "%s", line2);
        mvprintw(y + 1, x3, "%s", line3);
        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            timeout(-1);
            return false;
        }
    }
}

void Game::destroyWindows() {
    if (titleWin) { delwin(titleWin); titleWin = nullptr; }
    if (boardWin) { delwin(boardWin); boardWin = nullptr; }
    if (infoWin) { delwin(infoWin); infoWin = nullptr; }
    if (msgWin) { delwin(msgWin); msgWin = nullptr; }
}

void Game::createWindows() {
    int termH, termW;
    getmaxyx(stdscr, termH, termW);
    const int titleHeight = TITLE_H;
    int totalH = titleHeight + BOARD_H + MSG_H;
    int startY = (termH - totalH) / 2;
    int startX = (termW - TITLE_W) / 2;
    if (startY < 0) startY = 0;
    if (startX < 0) startX = 0;

    clear();
    refresh();

    titleWin = newwin(TITLE_H, TITLE_W, startY, startX);
    boardWin = newwin(BOARD_H, BOARD_W, startY + titleHeight, startX);
    infoWin = newwin(INFO_H, INFO_W, startY + titleHeight, startX + BOARD_W);
    msgWin = newwin(MSG_H, MSG_W, startY + titleHeight + BOARD_H, startX);

    keypad(infoWin, TRUE);
    keypad(msgWin, TRUE);
    applyWindowBg(titleWin);
    applyWindowBg(boardWin);
    applyWindowBg(infoWin);
    applyWindowBg(msgWin);
}

void Game::waitForEnter(WINDOW* w, int y, int x, const std::string& text) const {
    mvwprintw(w, y, x, "%s", text.c_str());
    wrefresh(w);
    int ch;
    do {
        ch = wgetch(w);
    } while (ch != '\n' && ch != KEY_ENTER && ch != '\r');
}

void Game::drawSetupTitle() const {
    if (!msgWin) {
        return;
    }

    const char* lines[] = {
        "  ________       .__       .___                   .__     ",
        " /  _____/  ____ |  |    __| _/______ __ __  _____|  |__  ",
        "/   \\  ___ /  _ \\|  |   / __ |\\_  __ \\  |  \\/  ___/  |  \\ ",
        "\\    \\_\\  (  <_> )  |__/ /_/ | |  | \\/  |  /\\___ \\|   Y  \\",
        " \\______  /\\____/|____/\\____ | |__|  |____//____  >___|  /",
        "        \\/                  \\/                  \\/     \\/ "
    };
    const int artW = 60;

    int h, w;
    getmaxyx(stdscr, h, w);
    int startY = (h / 2) - 6;
    int startX = (w - artW) / 2;
    if (startY < 1) startY = 1;
    if (startX < 0) startX = 0;

    if (hasColor) {
        attron(COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
    }
    for (int i = 0; i < 6; ++i) {
        mvprintw(startY + i, startX, "%s", lines[i]);
    }
    if (hasColor) {
        attroff(COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
        attron(COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    mvprintw(startY + 9, (w - 8) / 2, "GOLDRUSH");
    if (hasColor) {
        attroff(COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    refresh();
}

void Game::flashSpinResult(const std::string& title, int value) const {
    const int flashPairs[] = {
        GOLDRUSH_PLAYER_ONE,
        GOLDRUSH_PLAYER_TWO,
        GOLDRUSH_PLAYER_THREE,
        GOLDRUSH_PLAYER_FOUR
    };

    for (int flash = 0; flash < 4; ++flash) {
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "%s", title.c_str());

        int colorPair = flashPairs[flash % 4];
        if (hasColor) {
            wattron(msgWin, COLOR_PAIR(colorPair) | A_BOLD | ((flash % 2 == 0) ? A_BLINK : 0));
        } else if (flash % 2 == 0) {
            wattron(msgWin, A_REVERSE | A_BOLD);
        }

        mvwprintw(msgWin, 2, 2, "Spin result: %d!", value);

        if (hasColor) {
            wattroff(msgWin, COLOR_PAIR(colorPair) | A_BOLD | ((flash % 2 == 0) ? A_BLINK : 0));
        } else if (flash % 2 == 0) {
            wattroff(msgWin, A_REVERSE | A_BOLD);
        }

        wrefresh(msgWin);
        napms(130);
    }
}

bool Game::promptForFilename(const std::string& action,
                             const std::string& defaultName,
                             std::string& filename) {
    std::ostringstream prompt;
    prompt << action << " in saves/ [" << defaultName << "]: ";
    const std::string promptText = prompt.str();

    echo();
    curs_set(1);
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s", promptText.c_str());
    mvwprintw(msgWin, 2, 2, "Press ENTER for the default name or type a new filename.");
    wmove(msgWin, 1, 2 + static_cast<int>(promptText.size()));
    wrefresh(msgWin);

    char buffer[260] = {0};
    wgetnstr(msgWin, buffer, 259);

    noecho();
    curs_set(0);

    filename = buffer;
    if (filename.empty()) {
        filename = defaultName;
    }
    return true;
}

bool Game::chooseSaveFileToLoad(SaveFileInfo& selected) {
    SaveManager saveManager;
    std::string error;
    const std::vector<SaveFileInfo> saves = saveManager.listSaveFiles(error);
    if (!error.empty()) {
        showInfoPopup("Load failed", error);
        return false;
    }
    if (saves.empty()) {
        showInfoPopup("Load Game", "No save files were found in saves/.");
        return false;
    }

    int termH, termW;
    getmaxyx(stdscr, termH, termW);
    const int popupH = std::min(20, termH - 4);
    const int popupW = std::min(108, termW - 4);
    WINDOW* popup = newwin(popupH, popupW, (termH - popupH) / 2, (termW - popupW) / 2);
    applyWindowBg(popup);
    keypad(popup, TRUE);

    const int listTop = 4;
    const int listBottom = popupH - 4;
    const int visibleRows = std::max(1, listBottom - listTop + 1);
    const int availableWidth = popupW - 4;
    const int modifiedWidth = 19;
    const int sizeWidth = 9;
    int fileWidth = availableWidth - modifiedWidth - sizeWidth - 4;
    if (fileWidth < 20) {
        fileWidth = 20;
    }

    int selectedIndex = 0;
    int topIndex = 0;

    while (true) {
        if (selectedIndex < topIndex) {
            topIndex = selectedIndex;
        }
        if (selectedIndex >= topIndex + visibleRows) {
            topIndex = selectedIndex - visibleRows + 1;
        }

        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "Load Game");
        mvwprintw(popup, 2, 2, "Arrow keys move  Enter load  PgUp/PgDn scroll  Esc/Q cancel");
        mvwprintw(popup, 3, 2, "%-*s  %-19s  %9s", fileWidth, "File", "Modified", "Size");

        for (int row = 0; row < visibleRows; ++row) {
            const int index = topIndex + row;
            if (index >= static_cast<int>(saves.size())) {
                break;
            }

            const SaveFileInfo& info = saves[static_cast<std::size_t>(index)];
            std::string displayFilename = info.filename;
            if (!info.metadataValid) {
                displayFilename = "! " + displayFilename;
            } else if (info.duplicateGameId && !info.assignedTarget) {
                displayFilename = "* " + displayFilename;
            }
            const std::string filename = clipMenuText(displayFilename, static_cast<std::size_t>(fileWidth));
            const std::string sizeText = clipMenuText(formatSaveSize(info.sizeBytes), static_cast<std::size_t>(sizeWidth));

            if (index == selectedIndex) {
                wattron(popup, A_REVERSE);
            }
            mvwprintw(popup, listTop + row, 2, "%-*s  %-19s  %9s",
                      fileWidth,
                      filename.c_str(),
                      info.modifiedText.c_str(),
                      sizeText.c_str());
            if (index == selectedIndex) {
                wattroff(popup, A_REVERSE);
            }
        }

        const int shownFrom = topIndex + 1;
        const int shownTo = std::min(topIndex + visibleRows, static_cast<int>(saves.size()));
        mvwprintw(popup, popupH - 3, 2, "%s",
                  clipMenuText(saveMenuStatusText(saves[static_cast<std::size_t>(selectedIndex)]),
                               static_cast<std::size_t>(popupW - 4)).c_str());
        mvwprintw(popup, popupH - 2, 2, "Showing %d-%d of %d",
                  shownFrom, shownTo, static_cast<int>(saves.size()));
        mvwprintw(popup, popupH - 2, popupW / 2, "%s",
                  clipMenuText(saves[static_cast<std::size_t>(selectedIndex)].filename,
                               static_cast<std::size_t>(popupW / 2 - 4)).c_str());
        wrefresh(popup);

        const int ch = wgetch(popup);
        if (ch == KEY_UP) {
            if (selectedIndex > 0) {
                --selectedIndex;
            }
        } else if (ch == KEY_DOWN) {
            if (selectedIndex + 1 < static_cast<int>(saves.size())) {
                ++selectedIndex;
            }
        } else if (ch == KEY_PPAGE) {
            selectedIndex = std::max(0, selectedIndex - visibleRows);
        } else if (ch == KEY_NPAGE) {
            selectedIndex = std::min(static_cast<int>(saves.size()) - 1, selectedIndex + visibleRows);
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            selected = saves[static_cast<std::size_t>(selectedIndex)];
            delwin(popup);
            return true;
        } else if (ch == 27 || ch == 'q' || ch == 'Q' || ch == KEY_RESIZE) {
            delwin(popup);
            return false;
        }
    }
}

bool Game::saveCurrentGame() {
    SaveManager saveManager;
    std::string filename = assignedSaveFilename;
    if (filename.empty()) {
        promptForFilename("Save", saveManager.defaultSaveFilename(), filename);
    }

    const std::string previousAssignedFilename = assignedSaveFilename;
    const std::time_t previousLastSavedTime = lastSavedTime;
    if (gameId.empty()) {
        gameId = saveManager.generateGameId();
    }
    if (createdTime == 0) {
        createdTime = std::time(0);
    }
    // The save file name can change over time, but the game id stays stable so
    // duplicate copies of the same run can be detected and archived.
    assignedSaveFilename = saveManager.normalizeFilename(filename);
    lastSavedTime = std::time(0);

    std::string error;
    if (!saveManager.saveGame(*this, assignedSaveFilename, error)) {
        assignedSaveFilename = previousAssignedFilename;
        lastSavedTime = previousLastSavedTime;
        showInfoPopup("Save failed", error);
        return false;
    }

    int archivedCount = 0;
    std::string archiveError;
    const bool duplicatesArchived =
        saveManager.archiveDuplicateSaves(gameId, assignedSaveFilename, archivedCount, archiveError);

    const std::string resolvedPath = saveManager.resolvePath(assignedSaveFilename);
    addHistory("Saved game to " + resolvedPath);
    if (archivedCount > 0) {
        addHistory("Archived " + std::to_string(archivedCount) + " duplicate save copies");
    }
    if (!duplicatesArchived) {
        addHistory("Warning: " + archiveError);
        showInfoPopup("Game saved", archiveError);
        return true;
    }
    if (archivedCount > 0) {
        showInfoPopup("Game saved", "Archived " + std::to_string(archivedCount) + " duplicate save copies.");
        return true;
    }

    showInfoPopup("Game saved", resolvedPath);
    return true;
}

bool Game::loadSavedGame() {
    SaveManager saveManager;
    while (true) {
        SaveFileInfo selected;
        if (!chooseSaveFileToLoad(selected)) {
            return false;
        }

        std::string error;
        if (!saveManager.loadGame(*this, selected.path, error)) {
            showInfoPopup("Load failed", error);
            continue;
        }

        addHistory("Loaded game from " + selected.filename);
        if (!assignedSaveFilename.empty() && selected.filename != assignedSaveFilename) {
            addHistory("Loaded duplicate copy for " + gameId);
            showInfoPopup("Game loaded", "Future saves overwrite " + saveManager.resolvePath(assignedSaveFilename));
            return true;
        }
        if (selected.duplicateGameId) {
            showInfoPopup("Game loaded", "Canonical save slot: " + saveManager.resolvePath(assignedSaveFilename));
            return true;
        }
        showInfoPopup("Game loaded", displayNameFromPath(selected.path));
        return true;
    }
}

Game::StartChoice Game::showStartScreen() {
    const char* lines[] = {
        "  ________       .__       .___                   .__     ",
        " /  _____/  ____ |  |    __| _/______ __ __  _____|  |__  ",
        "/   \\  ___ /  _ \\|  |   / __ |\\_  __ \\  |  \\/  ___/  |  \\ ",
        "\\    \\_\\  (  <_> )  |__/ /_/ | |  | \\/  |  /\\___ \\|   Y  \\",
        " \\______  /\\____/|____/\\____ | |__|  |____//____  >___|  /",
        "        \\/                  \\/                  \\/     \\/ "
    };
    const int artLines = static_cast<int>(sizeof(lines) / sizeof(lines[0]));
    const int artW = 60;
    // The title screen is a two-step state machine: first choose new/load/quit,
    // then choose the ruleset for a new game.
    bool choosingMode = false;
    int highlightedMode = 0;

    while (true) {
        int h, w;
        getmaxyx(stdscr, h, w);
        clear();
        if (hasColor) bkgd(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

        int startY = (h / 2) - 6;
        int startX = (w - artW) / 2;
        if (startY < 1) startY = 1;
        if (startX < 0) startX = 0;

        if (hasColor) wattron(stdscr, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);
        for (int i = 0; i < artLines; ++i) {
            mvprintw(startY + i, startX, "%s", lines[i]);
        }
        if (hasColor) wattroff(stdscr, COLOR_PAIR(GOLDRUSH_GOLD_BLACK) | A_BOLD);

        if (hasColor) wattron(stdscr, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        mvprintw(startY + 9, (w - 8) / 2, "GOLDRUSH");
        if (hasColor) wattroff(stdscr, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);

        if (hasColor) wattron(stdscr, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
        if (!choosingMode) {
            mvprintw(startY + 11, (w - 34) / 2, "A Hasbro-style Life Journey");
            mvprintw(startY + 13, (w - 30) / 2, "N  New    L  Load    Q  Quit");
        } else {
            mvprintw(startY + 11, (w - 23) / 2, "ENTER select    Q back");
            const char* normal = "Normal Mode";
            const char* custom = "Custom Mode";
            const char* normalDesc = "Every optional system is enabled for the full game.";
            const char* customDesc = "Open the rules page and toggle features before starting.";
            int rowY = startY + 13;
            int normalX = (w / 2) - 18;
            int customX = (w / 2) + 4;
            if (highlightedMode == 0) attron(A_REVERSE);
            mvprintw(rowY, normalX, "%s", normal);
            if (highlightedMode == 0) attroff(A_REVERSE);
            if (highlightedMode == 1) attron(A_REVERSE);
            mvprintw(rowY, customX, "%s", custom);
            if (highlightedMode == 1) attroff(A_REVERSE);
            mvprintw(startY + 15, (w - 56) / 2, "%-56s", highlightedMode == 0 ? normalDesc : customDesc);
        }
        if (hasColor) wattroff(stdscr, COLOR_PAIR(GOLDRUSH_BROWN_SAND));
        refresh();

        int ch = getch();
        if (!choosingMode) {
            if (ch == 'n' || ch == 'N' || ch == 's' || ch == 'S') {
                choosingMode = true;
                highlightedMode = 0;
                continue;
            }
            if (ch == 'l' || ch == 'L') return START_LOAD_GAME;
            if (ch == 'q' || ch == 'Q') return START_QUIT_GAME;
        } else {
            if (ch == KEY_LEFT || ch == KEY_UP) {
                highlightedMode = highlightedMode == 0 ? 1 : 0;
                continue;
            }
            if (ch == KEY_RIGHT || ch == KEY_DOWN) {
                highlightedMode = highlightedMode == 1 ? 0 : 1;
                continue;
            }
            if (ch == 'q' || ch == 'Q') {
                choosingMode = false;
                continue;
            }
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                if (highlightedMode == 0) {
                    rules = makeNormalRules();
                    return START_NEW_GAME;
                }

                rules = makeCustomRules();
                if (configureCustomRules()) {
                    return START_NEW_GAME;
                }
                choosingMode = false;
                continue;
            }
        }
        if (ch == KEY_RESIZE && !ensureMinSize()) return START_QUIT_GAME;
    }
}

bool Game::configureCustomRules() {
    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(18, 72, (h - 18) / 2, (w - 72) / 2);
    applyWindowBg(popup);
    keypad(popup, TRUE);

    struct ToggleRow {
        const char* label;
        bool* value;
    };

    std::vector<ToggleRow> rows;
    rows.push_back({"Tutorial", &rules.toggles.tutorialEnabled});
    rows.push_back({"Family path", &rules.toggles.familyPathEnabled});
    rows.push_back({"Night school", &rules.toggles.nightSchoolEnabled});
    rows.push_back({"Risky route", &rules.toggles.riskyRouteEnabled});
    rows.push_back({"Investments", &rules.toggles.investmentEnabled});
    rows.push_back({"Pets", &rules.toggles.petsEnabled});
    rows.push_back({"Spin to Win", &rules.toggles.spinToWinEnabled});
    rows.push_back({"Electronic banking theme", &rules.toggles.electronicBankingEnabled});
    rows.push_back({"House sale spins", &rules.toggles.houseSaleSpinEnabled});
    rows.push_back({"Retirement bonuses", &rules.toggles.retirementBonusesEnabled});

    int highlight = 0;
    const int startRowIndex = static_cast<int>(rows.size());

    while (true) {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "Custom Mode");
        mvwprintw(popup, 2, 2, "Toggle rules with SPACE or ENTER. Select Start Game when ready.");

        for (size_t i = 0; i < rows.size(); ++i) {
            if (static_cast<int>(i) == highlight) wattron(popup, A_REVERSE);
            mvwprintw(popup, 4 + static_cast<int>(i), 2, "[%c] %s",
                      *rows[i].value ? 'x' : ' ', rows[i].label);
            if (static_cast<int>(i) == highlight) wattroff(popup, A_REVERSE);
        }

        if (highlight == startRowIndex) wattron(popup, A_REVERSE);
        mvwprintw(popup, 15, 2, "Start Game");
        if (highlight == startRowIndex) wattroff(popup, A_REVERSE);
        mvwprintw(popup, 16, 2, "Up/Down move  Enter/Space toggle  Q close config");
        wrefresh(popup);

        int ch = wgetch(popup);
        if (ch == KEY_UP) {
            highlight = highlight == 0 ? startRowIndex : highlight - 1;
        } else if (ch == KEY_DOWN) {
            highlight = highlight == startRowIndex ? 0 : highlight + 1;
        } else if (ch == 'q' || ch == 'Q') {
            delwin(popup);
            return false;
        } else if (ch == KEY_RESIZE) {
            delwin(popup);
            return false;
        } else if (ch == ' ' || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            if (highlight == startRowIndex) {
                break;
            }
            *rows[highlight].value = !*rows[highlight].value;
        }
    }

    delwin(popup);

    rules.components.investCards = rules.toggles.investmentEnabled ? 4 : 0;
    rules.components.petCards = rules.toggles.petsEnabled ? 12 : 0;
    rules.components.spinToWinTokens = rules.toggles.spinToWinEnabled ? 5 : 0;
    return true;
}

void Game::showTutorial() {
    if (!rules.toggles.tutorialEnabled) {
        return;
    }

    std::vector<std::vector<std::string> > pages;
    pages.push_back(std::vector<std::string>());
    pages[0].push_back("Reach retirement with the highest final wealth.");
    pages[0].push_back("Spinner rolls move your car from Start to Retirement.");
    pages[0].push_back("STOP spaces end movement immediately and resolve on the spot.");
    pages[0].push_back("College/Career, Family/Life, and Safe/Risky are branch decisions.");
    pages[0].push_back("Choosing College charges tuition immediately before that route begins.");
    pages[0].push_back("Paydays add salary. Action cards shake up your money.");
    pages[0].push_back("Marriage, babies, and house sales use special event spins.");
    pages[0].push_back("Retirement lets you choose MM or CA and awards order bonuses.");
    pages[0].push_back("During turns: B sabotage/defense, TAB scoreboard, G tile guide, K controls.");

    std::vector<std::string> legend = board.tutorialLegend();
    pages.push_back(std::vector<std::string>(legend.begin(), legend.begin() + 9));
    pages.push_back(std::vector<std::string>(legend.begin() + 9, legend.end()));

    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(18, 78, (h - 18) / 2, (w - 78) / 2);
    applyWindowBg(popup);
    keypad(popup, TRUE);

    for (size_t page = 0; page < pages.size(); ++page) {
        while (true) {
            werase(popup);
            box(popup, 0, 0);
            mvwprintw(popup, 1, 2, "Quick Tutorial (%d/%d)", static_cast<int>(page + 1), static_cast<int>(pages.size()));
            for (size_t line = 0; line < pages[page].size() && line < 12; ++line) {
                mvwprintw(popup, 3 + static_cast<int>(line), 2, "%s", pages[page][line].c_str());
            }
            mvwprintw(popup, 16, 2, "ENTER next  S skip");
            wrefresh(popup);

            int ch = wgetch(popup);
            if (ch == 's' || ch == 'S') {
                delwin(popup);
                addHistory("Tutorial skipped");
                return;
            }
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                break;
            }
        }
    }

    delwin(popup);
    addHistory("Tutorial reviewed");
}

void Game::showControlsPopup() const {
    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(16, 70, (h - 16) / 2, (w - 70) / 2);
    applyWindowBg(popup);
    box(popup, 0, 0);
    mvwprintw(popup, 1, 2, "Controls");
    mvwprintw(popup, 3, 2, "ENTER  Confirm a menu or start your turn spin");
    mvwprintw(popup, 4, 2, "SPACE  Hold/release to spin the wheel");
    mvwprintw(popup, 5, 2, "UP/DN  Move through menus and custom mode toggles");
    mvwprintw(popup, 6, 2, "TAB    Open the player scoreboard");
    mvwprintw(popup, 7, 2, "G      Open the tile abbreviation guide");
    mvwprintw(popup, 8, 2, "B      Open sabotage and defense actions");
    mvwprintw(popup, 9, 2, "S      Save the current game");
    mvwprintw(popup, 10, 2, "K/?    Open this controls popup");
    mvwprintw(popup, 11, 2, "Q      Quit the game");
    mvwprintw(popup, 12, 2, "STOP spaces end movement immediately.");
    mvwprintw(popup, 13, 2, "College tuition is paid as soon as College route is chosen.");
    mvwprintw(popup, 14, 2, "Press ENTER");
    wrefresh(popup);
    waitForEnter(popup, 14, 15, "");
    delwin(popup);
}

void Game::showScoreboardPopup() const {
    int h, w;
    getmaxyx(stdscr, h, w);
    const int popupH = 16;
    const int popupW = 104;
    WINDOW* popup = newwin(popupH, popupW, (h - popupH) / 2, (w - popupW) / 2);
    applyWindowBg(popup);
    box(popup, 0, 0);

    drawPopupPulse(popup, 1, 2, "Scoreboard", GOLDRUSH_GOLD_SAND, hasColor);
    mvwprintw(popup, 2, 2, "Rank  Player          Type      Tile        Cash       Loans      Worth est.  Route");

    std::vector<int> order;
    for (size_t i = 0; i < players.size(); ++i) {
        order.push_back(static_cast<int>(i));
    }
    std::sort(order.begin(), order.end(), [this](int a, int b) {
        return calculateFinalWorth(players[a]) > calculateFinalWorth(players[b]);
    });

    for (size_t row = 0; row < order.size(); ++row) {
        const int playerIndex = order[row];
        const Player& player = players[static_cast<std::size_t>(playerIndex)];
        const Tile& tile = board.tileAt(player.tile);
        const int y = 4 + static_cast<int>(row);
        const std::string name = clipMenuText(player.name, 14);
        const std::string tileText = std::to_string(player.tile) + " " + tile.label + " " + tileKindText(tile.kind);
        const std::string routeText = clipMenuText(playerRouteText(player), 24);
        const std::string typeText = player.type == PlayerType::CPU
            ? "CPU-" + cpuDifficultyLabel(player.cpuDifficulty)
            : "Human";

        if (playerIndex == currentPlayerIndex) {
            wattron(popup, A_REVERSE);
        }
        mvwprintw(popup,
                  y,
                  2,
                  "%-5d %-14s %-9s %-11s $%-9d %-10d $%-10d %-24s",
                  static_cast<int>(row + 1),
                  name.c_str(),
                  clipMenuText(typeText, 9).c_str(),
                  clipMenuText(tileText, 11).c_str(),
                  player.cash,
                  player.loans,
                  calculateFinalWorth(player),
                  routeText.c_str());
        if (playerIndex == currentPlayerIndex) {
            wattroff(popup, A_REVERSE);
        }
    }

    mvwprintw(popup, popupH - 4, 2, "Highlighted row is the current turn. Worth estimate includes loans and visible assets.");
    mvwprintw(popup, popupH - 3, 2, "Current turn: %s on tile %d [%s]",
              players[currentPlayerIndex].name.c_str(),
              players[currentPlayerIndex].tile,
              board.tileAt(players[currentPlayerIndex].tile).label.c_str());
    mvwprintw(popup, popupH - 2, 2, "Press ENTER");
    wrefresh(popup);
    waitForEnter(popup, popupH - 2, 15, "");
    delwin(popup);
}

void Game::showTileGuidePopup() const {
    int h, w;
    getmaxyx(stdscr, h, w);
    const std::vector<std::string> legend = board.tutorialLegend();
    const int popupH = 24;
    const int popupW = 82;
    WINDOW* popup = newwin(popupH, popupW, (h - popupH) / 2, (w - popupW) / 2);
    applyWindowBg(popup);
    box(popup, 0, 0);

    drawPopupPulse(popup, 1, 2, "Quick Tutorial: Tile Guide", GOLDRUSH_GOLD_SAND, hasColor);
    mvwprintw(popup, 2, 2, "Board abbreviations");

    for (size_t i = 0; i < legend.size(); ++i) {
        const int columnX = i < 10 ? 2 : 41;
        const int rowY = 4 + static_cast<int>(i % 10);
        mvwprintw(popup, rowY, columnX, "%s", legend[i].c_str());
    }

    mvwprintw(popup, popupH - 2, 2, "Press ENTER");
    wrefresh(popup);
    waitForEnter(popup, popupH - 2, 15, "");
    delwin(popup);
}

int Game::findPlayerIndex(const Player& player) const {
    for (size_t i = 0; i < players.size(); ++i) {
        if (&players[i] == &player) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Game::isCpuPlayer(int playerIndex) const {
    return playerIndex >= 0 &&
           playerIndex < static_cast<int>(players.size()) &&
           players[static_cast<std::size_t>(playerIndex)].type == PlayerType::CPU;
}

void Game::showCpuThinking(int playerIndex, const std::string& action) const {
    if (!isCpuPlayer(playerIndex) || !msgWin) {
        return;
    }

    const Player& player = players[static_cast<std::size_t>(playerIndex)];
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s [%s CPU]", player.name.c_str(),
              cpuDifficultyLabel(player.cpuDifficulty).c_str());
    mvwprintw(msgWin, 2, 2, "%s", action.c_str());
    wrefresh(msgWin);
    napms(550);
}

int Game::effectiveSalary(const Player& player) const {
    if (player.salaryReductionTurns <= 0 || player.salaryReductionPercent <= 0) {
        return player.salary;
    }
    const int reduction = (player.salary * player.salaryReductionPercent) / 100;
    return std::max(0, player.salary - reduction);
}

void Game::decrementTurnStatuses(Player& player) {
    if (player.movementPenaltyTurns > 0) {
        --player.movementPenaltyTurns;
        if (player.movementPenaltyTurns == 0) {
            player.movementPenaltyPercent = 0;
        }
    }
    if (player.salaryReductionTurns > 0) {
        --player.salaryReductionTurns;
        if (player.salaryReductionTurns == 0) {
            player.salaryReductionPercent = 0;
        }
    }
    if (player.sabotageCooldown > 0) {
        --player.sabotageCooldown;
    }
    if (player.itemDisableTurns > 0) {
        --player.itemDisableTurns;
    }
}

bool Game::resolveSkipTurn(int playerIndex) {
    Player& player = players[static_cast<std::size_t>(playerIndex)];
    if (!player.skipNextTurn) {
        return false;
    }
    player.skipNextTurn = false;
    addHistory(player.name + " skipped a turn from sabotage");
    renderGame(playerIndex, player.name + " skips this turn", "Sabotage effect resolved.");
    const bool previousAutoAdvance = autoAdvanceUi;
    autoAdvanceUi = isCpuPlayer(playerIndex);
    showInfoPopup("Skip Turn", player.name + " loses this turn to sabotage.");
    autoAdvanceUi = previousAutoAdvance;
    decrementTurnStatuses(player);
    return true;
}

int Game::chooseSabotageTarget(int attackerIndex) {
    std::vector<int> candidates;
    for (size_t i = 0; i < players.size(); ++i) {
        if (static_cast<int>(i) != attackerIndex && !players[i].retired) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) {
        showInfoPopup("Sabotage", "No valid targets.");
        return -1;
    }

    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(14, 68, (h - 14) / 2, (w - 68) / 2);
    applyWindowBg(popup);
    keypad(popup, TRUE);
    int selected = 0;
    while (true) {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "Choose Sabotage Target");
        for (size_t row = 0; row < candidates.size(); ++row) {
            const int playerIndex = candidates[row];
            if (static_cast<int>(row) == selected) {
                wattron(popup, A_REVERSE);
            }
            mvwprintw(popup, 3 + static_cast<int>(row), 2, "%s  cash $%d  worth $%d",
                      players[static_cast<std::size_t>(playerIndex)].name.c_str(),
                      players[static_cast<std::size_t>(playerIndex)].cash,
                      calculateFinalWorth(players[static_cast<std::size_t>(playerIndex)]));
            if (static_cast<int>(row) == selected) {
                wattroff(popup, A_REVERSE);
            }
        }
        mvwprintw(popup, 12, 2, "Up/Down move  ENTER select  Q cancel");
        wrefresh(popup);

        const int ch = wgetch(popup);
        if (ch == KEY_UP) {
            selected = selected == 0 ? static_cast<int>(candidates.size()) - 1 : selected - 1;
        } else if (ch == KEY_DOWN) {
            selected = selected + 1 >= static_cast<int>(candidates.size()) ? 0 : selected + 1;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            const int chosen = candidates[static_cast<std::size_t>(selected)];
            delwin(popup);
            return chosen;
        } else if (ch == 'q' || ch == 'Q' || ch == 27) {
            delwin(popup);
            return -1;
        }
    }
}

int Game::chooseTrapTile(int attackerIndex) {
    Player& player = players[static_cast<std::size_t>(attackerIndex)];
    echo();
    curs_set(1);
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "Trap tile id (0-%d) [%d]: ", TILE_COUNT - 1, player.tile);
    mvwprintw(msgWin, 2, 2, "Tip: choose a tile ahead of the leader.");
    wrefresh(msgWin);

    char buffer[16] = {0};
    wgetnstr(msgWin, buffer, 15);
    noecho();
    curs_set(0);

    if (std::strlen(buffer) == 0) {
        return player.tile;
    }
    const int tileId = std::atoi(buffer);
    if (tileId < 0 || tileId >= TILE_COUNT) {
        showInfoPopup("Trap Tile", "Invalid tile id.");
        return -1;
    }
    return tileId;
}

void Game::placeTrap(int attackerIndex, int tileId, SabotageType type) {
    if (attackerIndex < 0 ||
        attackerIndex >= static_cast<int>(players.size()) ||
        tileId < 0 ||
        tileId >= TILE_COUNT) {
        return;
    }
    Player& attacker = players[static_cast<std::size_t>(attackerIndex)];
    const SabotageCard card = sabotageCardByName("Trap Tile");
    PaymentResult payment = bank.charge(attacker, card.costToUse);

    ActiveTrap trap;
    trap.tileId = tileId;
    trap.ownerIndex = attackerIndex;
    trap.effectType = type;
    trap.strengthLevel = 2;
    trap.armed = true;
    activeTraps.push_back(trap);

    addHistory(attacker.name + " placed a " + sabotageTypeName(type) + " trap on tile " +
               std::to_string(tileId));
    std::string detail = "Trap armed on tile " + std::to_string(tileId) + ".";
    if (payment.loansTaken > 0) {
        detail += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
    }
    showInfoPopup("Trap Tile", detail);
}

void Game::executeSabotage(int attackerIndex, int targetIndex, SabotageType type) {
    if (attackerIndex < 0 ||
        targetIndex < 0 ||
        attackerIndex >= static_cast<int>(players.size()) ||
        targetIndex >= static_cast<int>(players.size()) ||
        attackerIndex == targetIndex) {
        return;
    }
    Player& attacker = players[static_cast<std::size_t>(attackerIndex)];
    Player& target = players[static_cast<std::size_t>(targetIndex)];
    if (attacker.sabotageCooldown > 0) {
        showInfoPopup("Sabotage cooldown",
                      attacker.name + " must wait " + std::to_string(attacker.sabotageCooldown) + " more turn(s).");
        return;
    }

    const SabotageCard card = type == SabotageType::MoneyLoss
        ? sabotageCardByName("Lawsuit")
        : sabotageCardForType(type);
    PaymentResult cost = bank.charge(attacker, card.costToUse);
    SabotageResult result = sabotage.applyDirectSabotage(card,
                                                         attacker,
                                                         target,
                                                         players,
                                                         attackerIndex,
                                                         targetIndex);
    if (type == SabotageType::PositionSwap && result.success) {
        const int cooldown = result.critical ? 3 : 4;
        attacker.sabotageCooldown = std::max(attacker.sabotageCooldown, cooldown + 1);
    } else if (result.success || result.blocked) {
        attacker.sabotageCooldown = std::max(attacker.sabotageCooldown, 2);
    }

    std::string detail = result.summary;
    if (cost.loansTaken > 0) {
        detail += " Cost auto-loans: " + std::to_string(cost.loansTaken) + ".";
    }
    addHistory(attacker.name + " used " + card.name + " on " + target.name);
    addHistory(detail);
    showInfoPopup(card.name, detail);
}

bool Game::promptSabotageMenu(int attackerIndex) {
    Player& attacker = players[static_cast<std::size_t>(attackerIndex)];
    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(19, 82, (h - 19) / 2, (w - 82) / 2);
    applyWindowBg(popup);
    keypad(popup, TRUE);

    while (true) {
        werase(popup);
        box(popup, 0, 0);
        mvwprintw(popup, 1, 2, "Sabotage Menu - %s", attacker.name.c_str());
        mvwprintw(popup, 2, 2, "Cash $%d  Shields %d  Insurance %d  Cooldown %d",
                  attacker.cash, attacker.shieldCards, attacker.insuranceUses, attacker.sabotageCooldown);
        mvwprintw(popup, 4, 2, "1 Trap Tile ($12000)");
        mvwprintw(popup, 5, 2, "2 Lawsuit ($15000)");
        mvwprintw(popup, 6, 2, "3 Traffic Jam ($10000)");
        mvwprintw(popup, 7, 2, "4 Steal Action Card ($18000)");
        mvwprintw(popup, 8, 2, "5 Forced Duel Minigame ($22000)");
        mvwprintw(popup, 9, 2, "6 Career Sabotage ($24000)");
        mvwprintw(popup, 10, 2, "7 Position Swap ($90000, cooldown)");
        mvwprintw(popup, 11, 2, "8 Debt Trap ($20000)");
        mvwprintw(popup, 12, 2, "9 Buy Shield Card ($15000)");
        mvwprintw(popup, 13, 2, "0 Buy Insurance ($20000, 2 uses)");
        mvwprintw(popup, 14, 2, "D Item Disable ($16000)");
        mvwprintw(popup, 17, 2, "Choose option or Q cancel");
        wrefresh(popup);

        const int ch = wgetch(popup);
        delwin(popup);
        if (ch == 'q' || ch == 'Q' || ch == 27) {
            return false;
        }
        if (ch == '9') {
            PaymentResult payment = bank.charge(attacker, 15000);
            ++attacker.shieldCards;
            std::string detail = "Shield Card added. Blocks one future sabotage.";
            if (payment.loansTaken > 0) {
                detail += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
            }
            showInfoPopup("Shield Card", detail);
            return true;
        }
        if (ch == '0') {
            PaymentResult payment = bank.charge(attacker, 20000);
            attacker.insuranceUses += 2;
            std::string detail = "Insurance added. Next 2 money/property hits are halved.";
            if (payment.loansTaken > 0) {
                detail += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
            }
            showInfoPopup("Insurance", detail);
            return true;
        }
        if (ch == '1') {
            const int tileId = chooseTrapTile(attackerIndex);
            if (tileId < 0) return false;
            werase(msgWin);
            box(msgWin, 0, 0);
            mvwprintw(msgWin, 1, 2, "Trap effect: 1 money  2 backward  3 skip  4 lose card  5 minigame");
            mvwprintw(msgWin, 2, 2, "Choose effect:");
            wrefresh(msgWin);
            const int trapChoice = wgetch(msgWin);
            SabotageType trapType = SabotageType::MoneyLoss;
            if (trapChoice == '2') trapType = SabotageType::MovementPenalty;
            else if (trapChoice == '3') trapType = SabotageType::SkipTurn;
            else if (trapChoice == '4') trapType = SabotageType::StealCard;
            else if (trapChoice == '5') trapType = SabotageType::ForceMinigame;
            placeTrap(attackerIndex, tileId, trapType);
            return true;
        }

        SabotageType type = SabotageType::MoneyLoss;
        if (ch == '2') type = SabotageType::MoneyLoss;
        else if (ch == '3') type = SabotageType::MovementPenalty;
        else if (ch == '4') type = SabotageType::StealCard;
        else if (ch == '5') type = SabotageType::ForceMinigame;
        else if (ch == '6') type = SabotageType::CareerPenalty;
        else if (ch == '7') type = SabotageType::PositionSwap;
        else if (ch == '8') type = SabotageType::DebtIncrease;
        else if (ch == 'd' || ch == 'D') type = SabotageType::ItemDisable;
        else return false;

        const int targetIndex = chooseSabotageTarget(attackerIndex);
        if (targetIndex >= 0) {
            executeSabotage(attackerIndex, targetIndex, type);
            return true;
        }
        return false;
    }
}

void Game::maybeCpuSabotage(int playerIndex) {
    if (!isCpuPlayer(playerIndex) || !cpu.shouldUseSabotage(players[playerIndex], turnCounter)) {
        return;
    }

    const int targetIndex = cpu.chooseSabotageTarget(players[playerIndex], players, playerIndex);
    if (targetIndex < 0) {
        return;
    }

    const SabotageType type = cpu.chooseSabotageType(players[playerIndex],
                                                    players[static_cast<std::size_t>(targetIndex)],
                                                    turnCounter);
    const bool previousAutoAdvance = autoAdvanceUi;
    autoAdvanceUi = true;
    showCpuThinking(playerIndex,
                    "CPU sabotage: " + sabotageTypeName(type) + " on " +
                    players[static_cast<std::size_t>(targetIndex)].name);
    executeSabotage(playerIndex, targetIndex, type);
    autoAdvanceUi = previousAutoAdvance;
}

void Game::checkTrapTrigger(int playerIndex) {
    Player& player = players[static_cast<std::size_t>(playerIndex)];
    for (size_t i = 0; i < activeTraps.size(); ++i) {
        ActiveTrap& trap = activeTraps[i];
        if (!trap.armed || trap.tileId != player.tile || trap.ownerIndex == playerIndex) {
            continue;
        }

        trap.armed = false;
        SabotageResult result = sabotage.triggerTrap(trap, player);
        addHistory(player.name + " triggered a trap on tile " + std::to_string(trap.tileId));
        addHistory(result.summary);
        showInfoPopup("Trap triggered", result.summary);

        if (result.success && trap.effectType == SabotageType::MovementPenalty) {
            const int stepsBack = result.critical ? 3 : 2;
            for (int step = 0; step < stepsBack; ++step) {
                player.tile = findPreviousTile(player, player.tile);
            }
            renderGame(playerIndex,
                       player.name + " was pushed backward by a trap",
                       "Moved back " + std::to_string(stepsBack) + " spaces.");
            napms(450);
        } else if (result.success && trap.effectType == SabotageType::ForceMinigame) {
            playBlackTileMinigame(playerIndex);
        }
        return;
    }
}

void Game::setupRules() {
    SaveManager saveManager;
    decks.reset(rules);
    bank.configure(rules);
    retiredCount = 0;
    currentPlayerIndex = 0;
    turnCounter = 0;
    gameId = saveManager.generateGameId();
    assignedSaveFilename.clear();
    createdTime = std::time(0);
    lastSavedTime = 0;
    activeTraps.clear();
    history.clear();
    addHistory("Mode: " + rules.editionName);
}

void Game::setupPlayers() {
    echo();
    curs_set(1);
    int numPlayers = 0;
    while (numPlayers < 2 || numPlayers > 4) {
        drawSetupTitle();
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "How many players? (2-4): ");
        wrefresh(msgWin);
        char buf[8] = {0};
        wgetnstr(msgWin, buf, 7);
        numPlayers = std::atoi(buf);
    }

    players.clear();
    players.reserve(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        drawSetupTitle();
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "Player %d type (H human / C CPU): ", i + 1);
        wrefresh(msgWin);
        char typeBuf[8] = {0};
        wgetnstr(msgWin, typeBuf, 7);

        PlayerType playerType = playerTypeFromText(typeBuf);
        CpuDifficulty difficulty = CpuDifficulty::Normal;
        if (playerType == PlayerType::CPU) {
            drawSetupTitle();
            werase(msgWin);
            box(msgWin, 0, 0);
            mvwprintw(msgWin, 1, 2, "CPU %d difficulty (E easy / N normal / H hard): ", i + 1);
            wrefresh(msgWin);
            char diffBuf[8] = {0};
            wgetnstr(msgWin, diffBuf, 7);
            difficulty = cpuDifficultyFromText(diffBuf);
        }

        drawSetupTitle();
        werase(msgWin);
        box(msgWin, 0, 0);
        if (playerType == PlayerType::CPU) {
            mvwprintw(msgWin, 1, 2, "CPU %d name [CPU %d]: ", i + 1, i + 1);
        } else {
            mvwprintw(msgWin, 1, 2, "Player %d name: ", i + 1);
        }
        wrefresh(msgWin);
        char nameBuf[32] = {0};
        wgetnstr(msgWin, nameBuf, 31);

        Player p;
        p.name = nameBuf;
        if (p.name.empty()) {
            p.name = playerType == PlayerType::CPU
                ? "CPU " + std::to_string(i + 1)
                : "Player " + std::to_string(i + 1);
        }
        p.token = tokenForName(p.name, i);
        p.tile = 0;
        p.cash = 10000;
        p.job = "Unemployed";
        p.salary = 0;
        p.married = false;
        p.kids = 0;
        p.collegeGraduate = false;
        p.usedNightSchool = false;
        p.hasHouse = false;
        p.houseName = "";
        p.houseValue = 0;
        p.loans = 0;
        p.investedNumber = 0;
        p.investPayout = 0;
        p.spinToWinTokens = 0;
        p.retirementPlace = 0;
        p.retirementBonus = 0;
        p.finalHouseSaleValue = 0;
        p.retirementHome = "";
        p.actionCards.clear();
        p.petCards.clear();
        p.retired = false;
        p.startChoice = -1;
        p.familyChoice = -1;
        p.riskChoice = -1;
        p.type = playerType;
        p.cpuDifficulty = difficulty;
        p.sabotageDebt = 0;
        p.shieldCards = 0;
        p.insuranceUses = 0;
        p.skipNextTurn = false;
        p.movementPenaltyTurns = 0;
        p.movementPenaltyPercent = 0;
        p.salaryReductionTurns = 0;
        p.salaryReductionPercent = 0;
        p.sabotageCooldown = 0;
        p.itemDisableTurns = 0;
        players.push_back(p);
        if (p.type == PlayerType::CPU) {
            addHistory("Joined CPU: " + p.name + " (" + cpuDifficultyLabel(p.cpuDifficulty) + ")");
        } else {
            addHistory("Joined: " + p.name);
        }
    }

    noecho();
    curs_set(0);
}

void Game::setupInvestments() {
    if (!rules.toggles.investmentEnabled) {
        return;
    }

    for (size_t i = 0; i < players.size(); ++i) {
        assignInvestment(players[i]);
    }
}

int Game::waitForTurnCommand(int currentPlayer) {
    if (isCpuPlayer(currentPlayer)) {
        showCpuThinking(currentPlayer, "CPU is planning its turn...");
        return '\n';
    }

    while (true) {
        int ch = wgetch(infoWin);
        if (ch == KEY_RESIZE) {
            if (!ensureMinSize()) {
                return 'q';
            }
            destroyWindows();
            createWindows();
            renderGame(currentPlayer,
                       players[currentPlayer].name + "'s turn",
                       "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
            continue;
        }
        if (ch == 'q' || ch == 'Q') return ch;
        if (ch == 's' || ch == 'S') return ch;
        if (ch == 'b' || ch == 'B') return ch;
        if (ch == '\t') {
            showScoreboardPopup();
            renderGame(currentPlayer,
                       players[currentPlayer].name + "'s turn",
                       "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
            continue;
        }
        if (ch == 'g' || ch == 'G') {
            showTileGuidePopup();
            renderGame(currentPlayer,
                       players[currentPlayer].name + "'s turn",
                       "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
            continue;
        }
        if (ch == 'k' || ch == 'K' || ch == '?') {
            showControlsPopup();
            renderGame(currentPlayer, players[currentPlayer].name + "'s turn", "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
            continue;
        }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            return ch;
        }
    }
}

void Game::renderHeader() const {
    if (!titleWin) {
        return;
    }
    draw_title_banner_ui(titleWin);
}

void Game::renderGame(int currentPlayer, const std::string& msg, const std::string& detail) const {
    renderHeader();
    draw_board_ui(boardWin, board, players, players[currentPlayer].tile);
    draw_sidebar_ui(infoWin, players, currentPlayer, history.recent(), rules);
    draw_message_ui(msgWin, msg, detail);
}

int Game::minRewardForTier(int tier) const {
    if (tier == 2) return 3000;
    if (tier >= 3) return 5000;
    return 1000;
}

int Game::maxRewardForTier(int tier) const {
    if (tier == 2) return 5000;
    if (tier >= 3) return 10000;
    return 2000;
}

void Game::showInfoPopup(const std::string& line1, const std::string& line2) const {
    for (int flash = 0; flash < 3; ++flash) {
        werase(msgWin);
        box(msgWin, 0, 0);
        if (flash % 2 == 0) {
            drawPopupPulse(msgWin, 1, 2, line1, GOLDRUSH_GOLD_SAND, hasColor);
        } else {
            mvwprintw(msgWin, 1, 2, "%s", line1.c_str());
        }
        mvwprintw(msgWin, 2, 2, "%s", line2.c_str());
        wrefresh(msgWin);
        napms(85);
    }
    werase(msgWin);
    box(msgWin, 0, 0);
    drawPopupPulse(msgWin, 1, 2, line1, GOLDRUSH_GOLD_SAND, hasColor);
    mvwprintw(msgWin, 2, 2, "%s", line2.c_str());
    wrefresh(msgWin);
    if (autoAdvanceUi) {
        napms(550);
        return;
    }
    waitForEnter(msgWin, 2, 2, "");
}

int Game::rollSpinner(const std::string& title, const std::string& detail) {
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s", title.c_str());
    mvwprintw(msgWin, 2, 2, "%s", detail.c_str());
    wrefresh(msgWin);

    if (autoAdvanceUi) {
        int value = 1;
        for (int flash = 0; flash < 6; ++flash) {
            value = rng.roll10();
            werase(msgWin);
            box(msgWin, 0, 0);
            mvwprintw(msgWin, 1, 2, "%s", title.c_str());
            mvwprintw(msgWin, 2, 2, "CPU rolling: %d", value);
            wrefresh(msgWin);
            napms(90);
        }
        flashSpinResult(title, value);
        addHistory("CPU auto-spin result: " + std::to_string(value));
        return value;
    }

    int ch;
    do {
        ch = wgetch(msgWin);
    } while (ch != ' ');

    nodelay(msgWin, TRUE);
    auto lastSpace = std::chrono::steady_clock::now();
    int value = 1;
    while (true) {
        value = rng.roll10();
        werase(msgWin);
        box(msgWin, 0, 0);
        mvwprintw(msgWin, 1, 2, "%s", title.c_str());
        mvwprintw(msgWin, 2, 2, "Rolling: %d  Release SPACE to stop", value);
        wrefresh(msgWin);
        napms(80);

        ch = wgetch(msgWin);
        if (ch == ' ') {
            lastSpace = std::chrono::steady_clock::now();
        }
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - lastSpace).count();
        if (elapsed > 240) break;
    }
    nodelay(msgWin, FALSE);

    flashSpinResult(title, value);
    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "%s", title.c_str());
    mvwprintw(msgWin, 2, 2, "Spin result: %d. Press ENTER to confirm", value);
    wrefresh(msgWin);
    waitForEnter(msgWin, 2, 2, "");
    return value;
}

int Game::showBranchPopup(const std::string& title,
                          const std::vector<std::string>& lines,
                          char a,
                          char b) {
    (void)a;
    (void)b;
    std::vector<int> values;
    values.push_back(0);
    values.push_back(1);
    return choose_branch_with_selector(title, lines, values, 0);
}

void Game::playBlackTileMinigame(int playerIndex) {
    Player& player = players[playerIndex];
    // Now 5 minigames: Pong (0), Battleship (1), Hangman (2), Memory (3), Minesweeper (4)
    const int minigameChoice = rng.uniformInt(0, 4);

    if (titleWin) touchwin(titleWin);
    if (boardWin) touchwin(boardWin);
    if (infoWin) touchwin(infoWin);
    if (msgWin) touchwin(msgWin);

    if (isCpuPlayer(playerIndex)) {
        const CpuMinigameResult cpuResult = cpu.playBlackTileMinigame(player, minigameChoice);
        const int payout = cpuResult.score * 100;
        if (payout > 0) {
            bank.credit(player, payout);
        }

        static const char* names[] = {
            "Pong",
            "Battleship",
            "Hangman",
            "Memory Match",
            "Minesweeper"
        };
        const std::string minigameName = names[minigameChoice];
        addHistory(player.name + " simulated " + minigameName + " and earned $" +
                   std::to_string(payout));
        renderGame(playerIndex,
                   player.name + " completed CPU " + minigameName,
                   cpuResult.summary + " | Earned $" + std::to_string(payout));
        showInfoPopup("CPU BLACK TILE: " + minigameName,
                      cpuResult.summary + " | Earned $" + std::to_string(payout));
        return;
    }

    //Player chooses PONG
    if (minigameChoice == 0) {
        addHistory(player.name + " landed on a black tile and entered Pong");
        showInfoPopup("BLACK TILE: PONG",
                      "One life. Each paddle return earns $100. Press ENTER to start.");

        const PongMinigameResult result = playPongMinigame(player.name, hasColor);

        if (titleWin) touchwin(titleWin);
        if (boardWin) touchwin(boardWin);
        if (infoWin) touchwin(infoWin);
        if (msgWin) touchwin(msgWin);

        if (result.abandoned) {
            addHistory(player.name + " left Pong before finishing");
            renderGame(playerIndex, player.name + " left the Pong sidegame", "No payout awarded");
            showInfoPopup("PONG sidegame", "Exited early. No payout awarded.");
            return;
        }

        const int payout = result.playerScore * 100;
        if (payout > 0) {
            bank.credit(player, payout);
        }

        std::ostringstream summary;
        summary << "Score " << result.playerScore
                << " | CPU " << result.cpuScore
                << " | Earned $" << payout;

        addHistory(player.name + " scored " + std::to_string(result.playerScore) +
                   " in Pong and earned $" + std::to_string(payout));
        renderGame(playerIndex, player.name + " finished the Pong sidegame", summary.str());
        showInfoPopup("PONG sidegame", summary.str());
        return;
    }

    //Player chooses BATTLESHIP
    else if (minigameChoice == 1) {
        addHistory(player.name + " landed on a black tile and entered Battleship");
        showInfoPopup("BLACK TILE: BATTLESHIP",
                    "Shoot the $ ships. One enemy hit ends the run. Each ship is worth $100.");

        const BattleshipMinigameResult result = playBattleshipMinigame(player.name, hasColor);

        if (titleWin) touchwin(titleWin);
        if (boardWin) touchwin(boardWin);
        if (infoWin) touchwin(infoWin);
        if (msgWin) touchwin(msgWin);

        if (result.abandoned) {
            addHistory(player.name + " left Battleship before finishing");
            renderGame(playerIndex, player.name + " left the Battleship sidegame", "No payout awarded");
            showInfoPopup("BATTLESHIP sidegame", "Exited early. No payout awarded.");
            return;
        }

        const int payout = result.shipsDestroyed * 100;
        if (payout > 0) {
            bank.credit(player, payout);
        }

        std::ostringstream summary;
        summary << "Destroyed " << result.shipsDestroyed
                << " ships"
                << (result.clearedWave ? " | Wave cleared" : " | Wave failed")
                << " | Earned $" << payout;

        addHistory(player.name + " destroyed " + std::to_string(result.shipsDestroyed) +
                " ships in Battleship and earned $" + std::to_string(payout));
        renderGame(playerIndex, player.name + " finished the Battleship sidegame", summary.str());
        showInfoPopup("BATTLESHIP sidegame", summary.str());
        return;
    }

    //PLayer chooses HANGMAN
    else if (minigameChoice == 2) {
        addHistory(player.name + " landed on a black tile and entered Hangman");
        showInfoPopup("BLACK TILE: HANGMAN",
                    "Guess the word. Hint unlocks after 5 misses. Each revealed letter is worth $100.");
        
        const HangmanResult result = playHangmanMinigame(player.name, hasColor);
        
        if (titleWin) touchwin(titleWin);
        if (boardWin) touchwin(boardWin);
        if (infoWin) touchwin(infoWin);
        if (msgWin) touchwin(msgWin);
        
        if (result.abandoned) {
            addHistory(player.name + " left Hangman before finishing");
            renderGame(playerIndex, player.name + " left the Hangman sidegame", "No payout awarded");
            showInfoPopup("HANGMAN sidegame", "Exited early. No payout awarded.");
            return;
        }
        
        const int payout = result.lettersGuessed * 100;
        if (payout > 0) {
            bank.credit(player, payout);
        }

        std::ostringstream summary;
        if (result.won) {
            summary << "Word guessed! Letters revealed: " << result.lettersGuessed
                    << " | Earned $" << payout;
        } else {
            summary << "Hangman completed | Letters revealed: " << result.lettersGuessed
                    << " | Earned $" << payout;
        }

        addHistory(player.name + (result.won
                   ? " won Hangman and earned $" + std::to_string(payout)
                   : " lost Hangman and earned $" + std::to_string(payout)));
        renderGame(playerIndex, player.name + " finished the Hangman sidegame", summary.str());
        showInfoPopup("HANGMAN sidegame", summary.str());
        return;
    }   
    
    //Player chooses MEMORY MATCH
    if (minigameChoice == 3) {
        addHistory(player.name + " landed on a black tile and entered Memory Match");
        showInfoPopup("BLACK TILE: MEMORY MATCH",
                      "Match all 8 pairs. Each wrong match costs a life. 20 lives total. Help button reveals grid.");
        
        const MemoryMatchResult result = playMemoryMatchMinigame(player.name, hasColor);
        
        if (titleWin) touchwin(titleWin);
        if (boardWin) touchwin(boardWin);
        if (infoWin) touchwin(infoWin);
        if (msgWin) touchwin(msgWin);
        
        if (result.abandoned) {
            addHistory(player.name + " left Memory Match before finishing");
            renderGame(playerIndex, player.name + " left the Memory Match sidegame", "No payout awarded");
            showInfoPopup("MEMORY MATCH sidegame", "Exited early. No payout awarded.");
            return;
        }
        
        const int payout = (result.pairsMatched * 100) + (result.won ? 200 : 0);
        if (payout > 0) {
            bank.credit(player, payout);
        }
        
        std::ostringstream summary;
        summary << "Pairs matched: " << result.pairsMatched << "/8"
                << " | Lives left: " << result.livesRemaining
                << " | Earned $" << payout;
        
        addHistory(player.name + " matched " + std::to_string(result.pairsMatched) +
                   " pairs in Memory Match and earned $" + std::to_string(payout));
        renderGame(playerIndex, player.name + " finished the Memory Match sidegame", summary.str());
        showInfoPopup("MEMORY MATCH sidegame", summary.str());
        return;
    }

    if (minigameChoice == 4) {
        const int minesweeperSafeTileTotal = 15;
        addHistory(player.name + " landed on a black tile and entered Minesweeper");
        showInfoPopup("BLACK TILE: MINESWEEPER",
                      "Reveal safe tiles for 60 seconds. One bomb ends the run. Each safe tile is worth $100.");

        const MinesweeperResult result = playMinesweeperMinigame(player.name, hasColor);

        if (titleWin) touchwin(titleWin);
        if (boardWin) touchwin(boardWin);
        if (infoWin) touchwin(infoWin);
        if (msgWin) touchwin(msgWin);

        if (result.abandoned) {
            addHistory(player.name + " left Minesweeper before finishing");
            renderGame(playerIndex, player.name + " left the Minesweeper sidegame", "No payout awarded");
            showInfoPopup("MINESWEEPER sidegame", "Exited early. No payout awarded.");
            return;
        }

        const int payout = result.safeTilesRevealed * 100;
        if (payout > 0) {
            bank.credit(player, payout);
        }

        std::ostringstream summary;
        summary << "Safe tiles: " << result.safeTilesRevealed << "/" << minesweeperSafeTileTotal;
        if (result.hitBomb) {
            summary << " | Bomb hit";
        } else if (result.safeTilesRevealed >= minesweeperSafeTileTotal) {
            summary << " | Board cleared";
        } else {
            summary << " | Time up";
        }
        summary << " | Earned $" << payout;

        addHistory(player.name + " cleared " + std::to_string(result.safeTilesRevealed) +
                   " safe tiles in Minesweeper and earned $" + std::to_string(payout));
        renderGame(playerIndex, player.name + " finished the Minesweeper sidegame", summary.str());
        showInfoPopup("MINESWEEPER sidegame", summary.str());
        return;
    }
}

int Game::findPreviousTile(const Player& player, int tileId) const {
    std::vector<int> candidates;
    for (int i = 0; i < TILE_COUNT; ++i) {
        const Tile& tile = board.tileAt(i);
        if (tile.next == tileId || tile.altNext == tileId) {
            candidates.push_back(i);
        }
    }

    if (candidates.empty()) {
        return tileId;
    }
    if (candidates.size() == 1) {
        return candidates[0];
    }

    if (tileId == 38) {
        return player.startChoice == 0 ? 24 : 37;
    }
    if (tileId == 86) {
        return player.riskChoice == 0 ? 82 : 85;
    }
    if (tileId == 87) {
        return player.familyChoice == 0 ? 72 : 86;
    }

    return candidates.back();
}

std::string Game::movePlayerByAction(int playerIndex, int steps) {
    Player& player = players[playerIndex];
    if (steps == 0) {
        return "No movement.";
    }

    const int totalSteps = steps > 0 ? steps : -steps;
    int moved = 0;
    bool stoppedOnSpace = false;

    for (int step = 0; step < totalSteps; ++step) {
        int nextTile = -1;
        if (steps > 0) {
            const Tile& current = board.tileAt(player.tile);
            nextTile = chooseNextTile(player, current);
        } else {
            nextTile = findPreviousTile(player, player.tile);
        }

        if (nextTile < 0 || nextTile == player.tile) {
            break;
        }

        player.tile = nextTile;
        ++moved;

        const Tile& landed = board.tileAt(player.tile);
        std::string message = player.name + " moved to " + landed.label;
        if (board.isStopSpace(landed)) {
            message = "STOP! " + player.name + " hit " + landed.label;
        }
        renderGame(playerIndex, message, "Action card movement...");
        napms(150);

        if (board.isStopSpace(landed)) {
            stoppedOnSpace = true;
            applyTileEffect(playerIndex, landed);
            break;
        }
    }

    std::ostringstream out;
    out << "Moved " << (steps > 0 ? "forward " : "back ") << moved
        << (moved == 1 ? " space" : " spaces")
        << " to " << board.tileAt(player.tile).label << ".";
    if (stoppedOnSpace) {
        out << " STOP resolved.";
    }
    return out.str();
}

std::string Game::applyActionEffect(int playerIndex,
                                    const Tile& tile,
                                    const ActionEffect& effect,
                                    int& amountDelta) {
    Player& player = players[playerIndex];
    amountDelta = 0;

    PaymentResult payment;
    payment.charged = 0;
    payment.loansTaken = 0;

    int amount = effect.amount;
    if (effect.useTileValue) {
        amount += tile.value * 2000;
    }

    switch (effect.kind) {
        case ACTION_NO_EFFECT:
            return "No payout this time.";
        case ACTION_GAIN_CASH:
            bank.credit(player, amount);
            amountDelta = amount;
            return "Collected $" + std::to_string(amount) + ".";
        case ACTION_PAY_CASH:
            payment = bank.charge(player, amount);
            amountDelta = -amount;
            return appendLoanText("Paid $" + std::to_string(amount) + ".", payment);
        case ACTION_GAIN_PER_KID:
            amount = player.kids * effect.amount;
            bank.credit(player, amount);
            amountDelta = amount;
            return "Collected $" + std::to_string(amount) + " for family bonuses.";
        case ACTION_PAY_PER_KID:
            amount = player.kids * effect.amount;
            payment = bank.charge(player, amount);
            amountDelta = -amount;
            return appendLoanText("Paid $" + std::to_string(amount) + " for family costs.", payment);
        case ACTION_GAIN_SALARY_BONUS:
            player.salary += effect.amount;
            bank.credit(player, effect.amount);
            amountDelta = effect.amount;
            return "Salary +$" + std::to_string(effect.amount) + " and immediate bonus paid.";
        case ACTION_BONUS_IF_MARRIED:
            if (player.married) {
                bank.credit(player, effect.amount);
                amountDelta = effect.amount;
                return "Marriage bonus paid $" + std::to_string(effect.amount) + ".";
            }
            return "No payout because you are not married yet.";
        case ACTION_MOVE_SPACES:
            return movePlayerByAction(playerIndex, effect.amount);
        default:
            return "No effect.";
    }
}

int Game::playActionCard(int playerIndex, const Tile& tile) {
    Player& player = players[playerIndex];
    ActionCard card;
    if (!decks.drawActionCard(card)) {
        showInfoPopup("Action Card", "No action cards are available.");
        return 0;
    }

    addHistory(player.name + " drew " + card.title);

    int amount = 0;
    int rollValue = 0;
    std::string branchText;
    std::string result;

    if (actionCardUsesRoll(card)) {
        rollValue = rollSpinner(card.title, "Hold SPACE to spin this card");
        addHistory(player.name + " rolled " + std::to_string(rollValue));

        const ActionRollOutcome* outcome = findMatchingRollOutcome(card, rollValue);
        if (outcome != 0) {
            branchText = outcome->text.empty()
                ? describeRollCondition(outcome->condition)
                : outcome->text;
            result = applyActionEffect(playerIndex, tile, outcome->effect, amount);
        } else {
            branchText = "No matching branch";
            result = "No effect.";
        }
    } else {
        result = applyActionEffect(playerIndex, tile, card.effect, amount);
    }

    if (card.keepAfterUse) {
        player.actionCards.push_back(card.title);
    }
    decks.resolveActionCard(card, card.keepAfterUse);
    if (branchText.empty()) {
        addHistory(player.name + " result: " + result);
    } else {
        addHistory(player.name + " result: " + branchText + " -> " + result);
    }

    int h, w;
    getmaxyx(stdscr, h, w);
    WINDOW* popup = newwin(12, 64, (h - 12) / 2, (w - 64) / 2);
    applyWindowBg(popup);
    werase(popup);
    box(popup, 0, 0);
    mvwprintw(popup, 1, 2, "ACTION CARD");
    mvwprintw(popup, 2, 2, "%s", card.title.c_str());
    mvwprintw(popup, 4, 2, "%s", card.description.c_str());
    if (rollValue > 0) {
        mvwprintw(popup, 5, 2, "Rolled: %d", rollValue);
    }
    if (!branchText.empty()) {
        mvwprintw(popup, 6, 2, "Branch: %s", branchText.c_str());
    }
    if (amount > 0) {
        drawPopupPulse(popup, 7, 2, "Result: " + result, GOLDRUSH_BLACK_FOREST, hasColor);
    } else if (amount < 0) {
        drawPopupPulse(popup, 7, 2, "Result: " + result, GOLDRUSH_GOLD_TERRA, hasColor);
    } else {
        mvwprintw(popup, 7, 2, "Result: %s", result.c_str());
    }
    mvwprintw(popup, 8, 2, "%s", card.keepAfterUse ? "Kept for endgame scoring." : "Discarded after use.");
    mvwprintw(popup, 9, 2, "Cash now: $%d  Loans: %d", player.cash, player.loans);
    mvwprintw(popup, 10, 2, "Press ENTER");
    wrefresh(popup);

    if (autoAdvanceUi) {
        napms(750);
    } else {
        int ch;
        do {
            ch = wgetch(popup);
        } while (ch != '\n' && ch != '\r' && ch != KEY_ENTER);
    }

    delwin(popup);
    touchwin(msgWin);
    wrefresh(msgWin);
    return amount;
}

void Game::chooseCareer(Player& player, bool requiresDegree) {
    std::vector<CareerCard> choices = decks.drawCareerChoices(requiresDegree, 2);
    if (choices.empty()) {
        showInfoPopup("Career Deck", "No career cards are available.");
        return;
    }

    std::vector<std::string> lines;
    for (size_t i = 0; i < choices.size(); ++i) {
        lines.push_back("- " + choices[i].title + " ($" + std::to_string(choices[i].salary) + ")");
    }

    int choice = 0;
    if (choices.size() > 1) {
        const int playerIndex = findPlayerIndex(player);
        if (isCpuPlayer(playerIndex)) {
            choice = cpu.chooseCareer(player, choices);
            showCpuThinking(playerIndex,
                            "Career choice: " + choices[static_cast<std::size_t>(choice)].title +
                            " ($" + std::to_string(choices[static_cast<std::size_t>(choice)].salary) + ")");
        } else {
            choice = showBranchPopup(requiresDegree ? "Choose a college career" : "Choose a career", lines, 'A', 'B');
        }
    }

    decks.resolveCareerChoices(requiresDegree, choices, choice);

    player.job = choices[choice].title;
    player.salary = choices[choice].salary;
    if (requiresDegree) {
        player.collegeGraduate = true;
    }

    addHistory(player.name + " became " + player.job);
    showInfoPopup(player.name + " became a " + player.job,
                  "Salary set to $" + std::to_string(player.salary));
}

void Game::resolveFamilyStop(Player& player) {
    if (!rules.toggles.familyPathEnabled) {
        player.familyChoice = 1;
        addHistory(player.name + " stays on the life path");
        showInfoPopup("Family STOP", "Family path is disabled. Staying on the life path.");
        return;
    }

    const int playerIndex = findPlayerIndex(player);
    int choice = 0;
    if (isCpuPlayer(playerIndex)) {
        choice = cpu.chooseFamilyRoute(player, rules);
        showCpuThinking(playerIndex,
                        choice == 0 ? "CPU chose Family path." : "CPU chose Life path.");
    } else {
        choice = showBranchPopup(
            "Family or Life path?",
            std::vector<std::string>{
                "- Family path: babies and houses",
                "- Life path: careers, safe/risky route"
            },
            'A',
            'B');
    }
    player.familyChoice = choice;
    addHistory(player.name + (choice == 0 ? " chose Family path" : " chose Life path"));
    showInfoPopup("Family STOP", choice == 0 ? "Family path selected." : "Life path selected.");
}

void Game::resolveNightSchool(Player& player) {
    if (!rules.toggles.nightSchoolEnabled) {
        addHistory(player.name + " passed Night School");
        showInfoPopup("Night School", "Night School is disabled in this mode.");
        return;
    }
    if (player.usedNightSchool) {
        showInfoPopup("Night School", "You already used Night School.");
        return;
    }

    const int playerIndex = findPlayerIndex(player);
    int choice = 0;
    if (isCpuPlayer(playerIndex)) {
        choice = cpu.chooseNightSchool(player, rules);
        showCpuThinking(playerIndex,
                        choice == 0 ? "CPU chose Night School." : "CPU kept current career.");
    } else {
        choice = showBranchPopup(
            "Night School?",
            std::vector<std::string>{
                "- Pay $100000 to draw a new college career",
                "- Keep your current career"
            },
            'A',
            'B');
    }
    if (choice == 0) {
        PaymentResult payment = bank.charge(player, 100000);
        player.usedNightSchool = true;
        addHistory(appendLoanText(player.name + " paid $100000 for Night School", payment));
        chooseCareer(player, true);
    } else {
        addHistory(player.name + " skipped Night School");
        showInfoPopup("Night School", "Current career kept.");
    }
}

void Game::resolveMarriageStop(Player& player) {
    if (!player.married) {
        player.married = true;
    }
    int spin = rollSpinner("Marriage Gifts", "Hold SPACE to spin wedding gifts");
    int gift = marriageGiftFromSpin(spin);
    bank.credit(player, gift);
    addHistory(player.name + " married and received $" + std::to_string(gift));
    maybeAwardPetCard(player, "Family edition bonus: a pet joined the family.");
    showInfoPopup("Get Married", "Gift spin paid $" + std::to_string(gift) + ".");
}

void Game::resolveBabyStop(Player& player, const Tile& tile) {
    int spin = rollSpinner("Baby Spin", "Hold SPACE to spin for 0 / 1 / 2 / 3 babies");
    int babies = babiesFromSpin(spin);
    player.kids += babies;
    addHistory(player.name + ": " + babiesLabel(babies) + " on " + tile.label);
    showInfoPopup(tile.label + " resolved", babiesLabel(babies));
}

void Game::resolveSafeRoute(Player& player) {
    int spin = rollSpinner("Safe Route", "Spin for a smaller guaranteed reward");
    int payout = safeRoutePayout(spin);
    bank.credit(player, payout);
    addHistory(player.name + " took Safe route for $" + std::to_string(payout));
    showInfoPopup("Safe Route", "Collected $" + std::to_string(payout) + ".");
}

void Game::resolveRiskyRoute(Player& player) {
    int spin = rollSpinner("Risky Route", "Spin for a big win or painful loss");
    int payout = riskyRoutePayout(spin);
    if (payout >= 0) {
        bank.credit(player, payout);
        addHistory(player.name + " won $" + std::to_string(payout) + " on Risky route");
        showInfoPopup("Risky Route", "You won $" + std::to_string(payout) + ".");
        return;
    }

    PaymentResult payment = bank.charge(player, -payout);
    addHistory(appendLoanText(player.name + " lost $" + std::to_string(-payout) + " on Risky route", payment));
    showInfoPopup("Risky Route", appendLoanText("You lost $" + std::to_string(-payout) + ".", payment));
}

void Game::resolveRetirement(int playerIndex) {
    Player& player = players[playerIndex];
    if (player.retired) {
        return;
    }

    int choice = 0;
    if (isCpuPlayer(playerIndex)) {
        choice = cpu.chooseRetirement(player);
        showCpuThinking(playerIndex,
                        choice == 0 ? "CPU chose Millionaire Mansion." : "CPU chose Countryside Acres.");
    } else {
        choice = showBranchPopup(
            "Choose retirement destination",
            std::vector<std::string>{
                "- Millionaire Mansion",
                "- Countryside Acres"
            },
            'A',
            'B');
    }

    player.retired = true;
    player.retirementHome = choice == 0 ? "MM" : "CA";
    player.tile = choice == 0 ? 87 : 88;
    ++retiredCount;
    player.retirementPlace = retiredCount;
    player.retirementBonus = rules.toggles.retirementBonusesEnabled ? retirementBonusForPlace(retiredCount) : 0;

    std::ostringstream line;
    line << "Place " << player.retirementPlace;
    if (player.retirementBonus > 0) {
        line << " bonus $" << player.retirementBonus;
    }
    addHistory(player.name + " retired to " + player.retirementHome);
    showInfoPopup("Retirement: " + player.retirementHome, line.str());
}

void Game::buyHouse(Player& player) {
    if (player.hasHouse) {
        showInfoPopup("House", "You already own " + player.houseName + ".");
        return;
    }

    HouseCard house;
    if (!decks.drawHouseCard(house)) {
        showInfoPopup("House Deck", "No house cards are available.");
        return;
    }
    PaymentResult payment = bank.charge(player, house.cost);
    player.hasHouse = true;
    player.houseName = house.title;
    player.houseValue = house.saleValue;
    player.finalHouseSaleValue = 0;
    addHistory(appendLoanText(player.name + " bought " + house.title, payment));
    maybeAwardPetCard(player, "House purchase bonus: a pet moved in.");
    showInfoPopup("House: " + house.title,
                  appendLoanText("Paid $" + std::to_string(house.cost) +
                                 ", spin sale base $" + std::to_string(house.saleValue) + ".", payment));
}

void Game::assignInvestment(Player& player) {
    InvestCard card;
    if (!decks.drawInvestCard(card) || card.number <= 0) {
        return;
    }

    player.investedNumber = card.number;
    player.investPayout = card.payout;
    addHistory(player.name + " invested on " + std::to_string(card.number));
}

void Game::resolveInvestmentPayouts(int spinnerValue) {
    if (!rules.toggles.investmentEnabled) {
        return;
    }

    std::ostringstream summary;
    bool anyMatch = false;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].investedNumber != spinnerValue || players[i].investPayout <= 0) {
            continue;
        }
        bank.credit(players[i], players[i].investPayout);
        addHistory(players[i].name + " investment matched " + std::to_string(spinnerValue));
        if (anyMatch) {
            summary << " | ";
        }
        summary << players[i].name << " +$" << players[i].investPayout;
        anyMatch = true;
    }

    if (anyMatch) {
        showInfoPopup("Investment payout on spin " + std::to_string(spinnerValue), summary.str());
    }
}

void Game::maybeAwardSpinToWin(Player& player, int spinnerValue) {
    if (!rules.toggles.spinToWinEnabled || spinnerValue != 10) {
        return;
    }

    ++player.spinToWinTokens;
    bank.credit(player, rules.spinToWinPrize);
    addHistory(player.name + " triggered Spin to Win");
    showInfoPopup("Spin to Win!", player.name + " gains a token and $" +
                  std::to_string(rules.spinToWinPrize) + ".");
}

void Game::maybeAwardPetCard(Player& player, const std::string& reason) {
    if (!rules.toggles.petsEnabled) {
        return;
    }

    PetCard pet;
    if (!decks.drawPetCard(pet) || pet.title.empty()) {
        return;
    }

    player.petCards.push_back(pet.title);
    addHistory(player.name + " adopted a " + pet.title);
    showInfoPopup(player.name + " adopted a " + pet.title, reason);
}

void Game::applyTileEffect(int playerIndex, const Tile& tile) {
    Player& player = players[playerIndex];
    std::string line = "Keep moving.";

    switch (tile.kind) {
        case TILE_START:
            line = "START: The journey begins.";
            addHistory(player.name + " started the journey");
            break;
        case TILE_BLACK:
            playBlackTileMinigame(playerIndex);
            return;
        case TILE_COLLEGE: {
            player.collegeGraduate = false;
            if (player.startChoice == 0) {
                line = "COLLEGE: tuition was paid when this route was chosen.";
                addHistory(player.name + " entered college");
            } else {
                PaymentResult payment = bank.charge(player, 100000);
                line = appendLoanText("COLLEGE: tuition/loan cost $100000.", payment);
                addHistory(appendLoanText(player.name + " entered college", payment));
            }
            break;
        }
        case TILE_CAREER:
            chooseCareer(player, false);
            return;
        case TILE_GRADUATION:
            if (player.startChoice == 0 || player.job == "Unemployed") {
                chooseCareer(player, true);
                return;
            }
            line = "GRADUATION STOP: career route keeps the current job.";
            addHistory(player.name + " cleared Graduation");
            break;
        case TILE_MARRIAGE:
            resolveMarriageStop(player);
            return;
        case TILE_FAMILY:
            resolveFamilyStop(player);
            return;
        case TILE_NIGHT_SCHOOL:
            resolveNightSchool(player);
            return;
        case TILE_SPLIT_RISK:
            if (!rules.toggles.riskyRouteEnabled) {
                player.riskChoice = 0;
                addHistory(player.name + " defaults to the Safe route");
                showInfoPopup("Risk split", "Risky route is disabled. Safe route selected.");
            } else {
                int riskChoice = 0;
                if (isCpuPlayer(playerIndex)) {
                    riskChoice = cpu.chooseRiskRoute(player, rules);
                    showCpuThinking(playerIndex,
                                    riskChoice == 0 ? "CPU chose Safe route." : "CPU chose Risky route.");
                } else {
                    riskChoice = showBranchPopup(
                        "Safe or Risky route?",
                        std::vector<std::string>{
                            "- Safe route: smaller payout, no huge swings",
                            "- Risky route: bigger wins and losses"
                        },
                        'A',
                        'B');
                }
                player.riskChoice = riskChoice;
                addHistory(player.name + (riskChoice == 0 ? " chose Safe route" : " chose Risky route"));
                showInfoPopup("Risk split", riskChoice == 0 ? "Safe route selected." : "Risky route selected.");
            }
            return;
        case TILE_SAFE:
            resolveSafeRoute(player);
            return;
        case TILE_RISKY:
            resolveRiskyRoute(player);
            return;
        case TILE_SPIN_AGAIN:
            addHistory(player.name + " hit Spin Again");
            showInfoPopup("Spin Again", "Take another full movement spin right now.");
            takeMovementSpin(playerIndex, "Spin Again");
            return;
        case TILE_CAREER_2:
            player.salary += tile.value;
            line = "PROMOTION: salary increased to $" + std::to_string(player.salary) + ".";
            addHistory(player.name + " got a promotion");
            break;
        case TILE_PAYDAY: {
            int payout = tile.value + effectiveSalary(player);
            bank.credit(player, payout);
            line = "PAYDAY: collected $" + std::to_string(payout) + ".";
            if (player.salaryReductionTurns > 0) {
                line += " Salary sabotage reduced this payday.";
            }
            addHistory(player.name + " collected payday $" + std::to_string(payout));
            break;
        }
        case TILE_BABY:
            resolveBabyStop(player, tile);
            return;
        case TILE_HOUSE:
            buyHouse(player);
            return;
        case TILE_RETIREMENT:
            resolveRetirement(playerIndex);
            return;
        case TILE_SPLIT_START:
        case TILE_SPLIT_FAMILY:
        case TILE_EMPTY:
        default:
            break;
    }

    showInfoPopup(tile.label + " resolved", line);
}

int Game::chooseNextTile(Player& player, const Tile& tile) {
    if (tile.kind == TILE_SPLIT_START && player.startChoice == -1) {
        const int playerIndex = findPlayerIndex(player);
        int c = 0;
        if (isCpuPlayer(playerIndex)) {
            c = cpu.chooseStartRoute(player, rules);
            showCpuThinking(playerIndex,
                            c == 0 ? "CPU chose College route." : "CPU chose Career route.");
        } else {
            c = showBranchPopup(
                "College or Career?",
                std::vector<std::string>{
                    "- College: pay $100000 now, stronger jobs later",
                    "- Career: choose a job right away"
                },
                'A',
                'B');
        }
        player.startChoice = c;
        if (c == 0) {
            PaymentResult payment = bank.charge(player, 100000);
            player.collegeGraduate = false;
            addHistory(appendLoanText(player.name + " chose College and paid $100000", payment));
            showInfoPopup("College route",
                          appendLoanText("Tuition paid before moving onto CO.", payment));
        } else {
            addHistory(player.name + " chose Career");
        }
    }

    if (tile.kind == TILE_SPLIT_FAMILY) {
        if (player.familyChoice == -1) {
            player.familyChoice = 1;
        }
        return player.familyChoice == 0 ? tile.next : tile.altNext;
    }

    if (tile.kind == TILE_SPLIT_RISK) {
        if (player.riskChoice == -1) {
            player.riskChoice = 0;
        }
        return player.riskChoice == 0 ? tile.next : tile.altNext;
    }

    if (tile.kind == TILE_SPLIT_START) {
        return player.startChoice == 0 ? tile.next : tile.altNext;
    }

    return tile.next;
}

bool Game::animateMove(int currentPlayer, int steps) {
    Player& player = players[currentPlayer];
    for (int step = 0; step < steps; ++step) {
        const Tile& current = board.tileAt(player.tile);
        int nextTile = chooseNextTile(player, current);
        if (nextTile < 0) break;
        player.tile = nextTile;

        const Tile& landed = board.tileAt(player.tile);
        std::string message = player.name + " moved to " + landed.label;
        if (board.isStopSpace(landed)) {
            message = "STOP! " + player.name + " hit " + landed.label;
        }
        renderGame(currentPlayer, message, "Movement in progress...");
        napms(170);
        checkTrapTrigger(currentPlayer);
        const Tile& afterTrap = board.tileAt(player.tile);

        if (board.isStopSpace(afterTrap)) {
            applyTileEffect(currentPlayer, afterTrap);
            return true;
        }
    }
    return false;
}

void Game::takeMovementSpin(int currentPlayer, const std::string& reason) {
    Player& player = players[currentPlayer];
    int roll = rollSpinner(reason, "Hold SPACE to spin movement");
    addHistory(player.name + " spun " + std::to_string(roll));
    maybeAwardSpinToWin(player, roll);
    resolveInvestmentPayouts(roll);

    if (player.movementPenaltyTurns > 0 && player.movementPenaltyPercent > 0) {
        const int originalRoll = roll;
        if (player.movementPenaltyPercent >= 100) {
            roll = 0;
        } else {
            roll = std::max(1, (roll * (100 - player.movementPenaltyPercent)) / 100);
        }
        addHistory(player.name + " movement reduced from " +
                   std::to_string(originalRoll) + " to " + std::to_string(roll));
        showInfoPopup("Traffic Jam", "Movement reduced from " +
                      std::to_string(originalRoll) + " to " + std::to_string(roll) + ".");
    }

    if (roll <= 0) {
        addHistory(player.name + " could not move because of sabotage");
        showInfoPopup("Traffic Jam", "Movement cancelled. No tile effect is resolved.");
        return;
    }

    bool stoppedEarly = animateMove(currentPlayer, roll);
    if (!stoppedEarly) {
        applyTileEffect(currentPlayer, board.tileAt(player.tile));
    }
}

bool Game::allPlayersRetired() const {
    for (size_t i = 0; i < players.size(); ++i) {
        if (!players[i].retired) {
            return false;
        }
    }
    return true;
}

void Game::finalizeScoring() {
    for (size_t i = 0; i < players.size(); ++i) {
        Player& player = players[i];
        const bool previousAutoAdvance = autoAdvanceUi;
        autoAdvanceUi = isCpuPlayer(static_cast<int>(i));
        if (player.hasHouse) {
            if (rules.toggles.houseSaleSpinEnabled) {
                int spin = rollSpinner(player.name + " house sale", "Spin to sell your house");
                player.finalHouseSaleValue = houseSaleValueFromSpin(player.houseValue, spin);
            } else {
                player.finalHouseSaleValue = player.houseValue;
            }
            addHistory(player.name + " sold " + player.houseName + " for $" + std::to_string(player.finalHouseSaleValue));
        }

        int actionScore = static_cast<int>(player.actionCards.size()) * 100000;
        int petScore = static_cast<int>(player.petCards.size()) * 100000;
        int babyScore = player.kids * 50000;
        int loanPenalty = bank.totalLoanDebt(player);
        int houseScore = player.finalHouseSaleValue;

        std::ostringstream line1;
        std::ostringstream line2;
        line1 << player.name << ": cash $" << player.cash
              << " + house $" << houseScore
              << " + actions $" << actionScore;
        line2 << "pets $" << petScore
              << " + babies $" << babyScore
              << " + retire $" << player.retirementBonus
              << " - loans $" << loanPenalty;
        showInfoPopup(line1.str(), line2.str());
        autoAdvanceUi = previousAutoAdvance;
    }
}

int Game::calculateFinalWorth(const Player& player) const {
    int worth = player.cash;
    worth += player.finalHouseSaleValue > 0 ? player.finalHouseSaleValue : player.houseValue;
    worth += static_cast<int>(player.actionCards.size()) * 100000;
    worth += static_cast<int>(player.petCards.size()) * 100000;
    worth += player.kids * 50000;
    worth += player.retirementBonus;
    worth -= bank.totalLoanDebt(player);
    return worth;
}

bool Game::run() {
    if (!ensureMinSize()) return false;
    while (true) {
        const StartChoice startChoice = showStartScreen();
        if (startChoice == START_QUIT_GAME) {
            return false;
        }

        createWindows();
        if (startChoice == START_LOAD_GAME) {
            if (loadSavedGame()) {
                break;
            }
            destroyWindows();
            continue;
        }

        setupRules();
        setupPlayers();
        setupInvestments();
        showTutorial();
        break;
    }

    while (!allPlayersRetired()) {
        if (!ensureMinSize()) return false;
        destroyWindows();
        createWindows();

        if (players[currentPlayerIndex].retired) {
            currentPlayerIndex = (currentPlayerIndex + 1) % static_cast<int>(players.size());
            continue;
        }

        if (resolveSkipTurn(currentPlayerIndex)) {
            ++turnCounter;
            currentPlayerIndex = (currentPlayerIndex + 1) % static_cast<int>(players.size());
            continue;
        }

        renderGame(currentPlayerIndex,
                   players[currentPlayerIndex].name + "'s turn",
                   "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
        maybeCpuSabotage(currentPlayerIndex);
        int command = waitForTurnCommand(currentPlayerIndex);
        if (command == 'q' || command == 'Q') {
            const int quitChoice = showBranchPopup(
                "Do you want to save when pressing Q in game?",
                std::vector<std::string>{
                    "- Yes, save and quit",
                    "- No, quit without saving"
                },
                'A',
                'B');
            if (quitChoice == 0) {
                if (saveCurrentGame()) {
                    return false;
                }
                renderGame(currentPlayerIndex,
                           players[currentPlayerIndex].name + "'s turn",
                           "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
                continue;
            }
            return false;
        }
        if (command == 's' || command == 'S') {
            saveCurrentGame();
            continue;
        }
        if (command == 'b' || command == 'B') {
            const bool sabotageUsed = promptSabotageMenu(currentPlayerIndex);
            renderGame(currentPlayerIndex,
                       players[currentPlayerIndex].name + "'s turn",
                       "ENTER spin | B sabotage | TAB scores | G guide | K keys | S save | Q quit/save");
            if (!sabotageUsed) {
                continue;
            }
        }

        autoAdvanceUi = isCpuPlayer(currentPlayerIndex);
        takeMovementSpin(currentPlayerIndex, "Movement Spin");
        autoAdvanceUi = false;
        decrementTurnStatuses(players[currentPlayerIndex]);
        ++turnCounter;
        currentPlayerIndex = (currentPlayerIndex + 1) % static_cast<int>(players.size());
    }

    finalizeScoring();

    int winner = 0;
    int best = calculateFinalWorth(players[0]);
    for (size_t i = 1; i < players.size(); ++i) {
        int worth = calculateFinalWorth(players[i]);
        if (worth > best) {
            best = worth;
            winner = static_cast<int>(i);
        }
    }

    werase(msgWin);
    box(msgWin, 0, 0);
    mvwprintw(msgWin, 1, 2, "Game Over! %s wins with $%d.", players[winner].name.c_str(), best);
    mvwprintw(msgWin, 2, 2, "Press ENTER to exit");
    wrefresh(msgWin);
    waitForEnter(msgWin, 2, 2, "");
    return true;
}
