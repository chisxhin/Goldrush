#include "game.hpp"
#include "battleship.hpp"
#include "pong.hpp"
#include "hangman.hpp"
#include "memory.hpp"
#include "minesweeper.hpp"
#include "tile_display.h"
#include "timer_display.h"
#include "save_manager.hpp"
#include "spins.hpp"
#include "ui.h"
#include "ui_helpers.h"
#include "completed_history.h"
#include "turn_summary.h"

#include <algorithm>
#include <cctype>
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
    out << base << " The bank covered it with " << payment.loansTaken
        << " automatic loan" << (payment.loansTaken == 1 ? "" : "s") << ".";
    return out.str();
}

std::string trimCopy(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool parseStrictInt(const std::string& text, int& value) {
    const std::string trimmed = trimCopy(text);
    if (trimmed.empty()) {
        return false;
    }
    std::size_t index = 0;
    if (trimmed[index] == '+' || trimmed[index] == '-') {
        ++index;
    }
    if (index >= trimmed.size()) {
        return false;
    }
    for (; index < trimmed.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(trimmed[index]))) {
            return false;
        }
    }
    char* end = nullptr;
    const long parsed = std::strtol(trimmed.c_str(), &end, 10);
    if (end == nullptr || *end != '\0' || parsed < -2147483647L - 1L || parsed > 2147483647L) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool confirmTitleQuit(bool hasColor) {
    return showQuitConfirmation(hasColor);
}

std::string careerDescriptionText(const CareerCard& card) {
    const std::string description = trimCopy(card.description);
    if (!description.empty() && description != card.title) {
        return description;
    }
    if (card.requiresDegree) {
        return "A degree-focused career with stronger long-term salary potential.";
    }
    return "A practical career path that starts earning steady pay right away.";
}

std::string turnPromptText() {
    return "ENTER begin turn | B sabotage | TAB scores+map | G guide | K keys | S save | ESC menu";
}

//shiny number
std::vector<std::string> bigRollNumberArt(int value) {
    switch (value) {
        case 1:
            return {"   __  ", 
                    "  /_ | ", 
                    "   | | ", 
                    "   | | ",
                    "   | | ",
                    "   |_| "};
        case 2:
            return {"  ___  ", 
                    " |__ \\  ", 
                    "    ) |  ", 
                    "   / /   ",
                    "  / /___ ",
                    " /_____// "};
        case 3:
            return {"  ____  ", 
                    " |___ \\", 
                    "   __) |", 
                    "  |__ < ", 
                    "  ___) |",
                    " |____/ "};
        case 4:
            return {"  _  _   ", 
                    " | || |  ", 
                    " | || |_ ", 
                    " |__   _|", 
                    "    | |  ",
                    "    |_|   "};
        case 5:
            return {"  _____ ", 
                    " | ____|", 
                    " | |__  ", 
                    " |___ \\", 
                    "  ___) |",
                    " |____/ "};
        case 6:
            return {"   __   ", 
                    "  / /   ", 
                    " / /_   ", 
                    "| '_ \\ ", 
                    "| (_) | ",
                    "\\___/  "};
        case 7:
            return {"  ______ ", 
                    " |____  |", 
                    "     / / ", 
                    "    / /  ", 
                    "   / /   ",
                    "  /_/    "};
        case 8:
            return {"   ___   ", 
                    "  / _ \\  ", 
                    " | (_) | ", 
                    "  > _ <  ", 
                    " | (_) | ",
                    " \\___/  "};
        case 9:
            return {"   ___   ", 
                    "  / _ \\  ", 
                    " | (_) | ", 
                    " \\__, | ", 
                    "    / /  ",
                    "   /_/   "};
        case 10:
            return {"  _   ___   ", 
                    "/_ | / _ \\ ", 
                    " | || | | | ", 
                    " | || | | | ", 
                    " | || |_| | ",
                    " |_|\\___// "};
        default:
            return {std::to_string(value)};
    }
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

}

std::string cpuReasonForAction(const Player& player, const std::string& action);
std::string describeTileEffectText(const Tile& tile);

Game::Game()
    : boardViewMode(BoardViewMode::FollowCamera),
      rules(makeNormalRules()),
      rng(),
      cpu(rng),
      decks(rules, rng),
      bank(rules),
      sabotage(bank, rng),
      history(8),
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
      sabotageUnlockAnnounced(false),
      tutorialFlags(),
      activeTraps() {
}

Game::Game(std::uint32_t seed)
    : boardViewMode(BoardViewMode::FollowCamera),
      rules(makeNormalRules()),
      rng(seed),
      cpu(rng),
      decks(rules, rng),
      bank(rules),
      sabotage(bank, rng),
      history(8),
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
      sabotageUnlockAnnounced(false),
      tutorialFlags(),
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
    const int minHeight = minimumGameHeight();
    const int minWidth = minimumGameWidth();
    timeout(200);
    while (true) {
        getmaxyx(stdscr, h, w);
        if (h >= minHeight && w >= minWidth) {
            timeout(-1);
            return true;
        }

        if (hasColor) {
            bkgd(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        }
        clear();
        const char* line1 = "Please enter full-screen mode"; 
        std::ostringstream line2;
        line2 << "Minimum size: " << minWidth << "x" << minHeight;
        std::ostringstream line3;
        line3 << "Current size: " << w << "x" << h;
        const char* line4 = "Press ESC to quit";
        int x1 = (w - static_cast<int>(std::strlen(line1))) / 2;
        int x2 = (w - static_cast<int>(line2.str().size())) / 2;
        int x3 = (w - static_cast<int>(line3.str().size())) / 2;
        int x4 = (w - static_cast<int>(std::strlen(line4))) / 2;
        int y = h / 2;
        if (x1 < 0) x1 = 0;
        if (x2 < 0) x2 = 0;
        if (x3 < 0) x3 = 0;
        if (x4 < 0) x4 = 0;
        mvprintw(y - 1, x1, "%s", line1);
        mvprintw(y, x2, "%s", line2.str().c_str());
        mvprintw(y + 1, x3, "%s", line3.str().c_str());
        mvprintw(y + 2, x4, "%s", line4);
        refresh();

        int ch = getch();
        if (ch == 27) {
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
    const UILayout layout = calculateUILayout(termH, termW);
    if (layout.originY + layout.totalHeight > termH ||
        layout.originX + layout.totalWidth > termW) {
        showTerminalSizeWarning(layout.totalHeight, layout.totalWidth, hasColor);
        return;
    }

    clear();
    refresh();

    titleWin = newwin(layout.headerHeight,
                      layout.totalWidth,
                      layout.originY,
                      layout.originX);
    boardWin = newwin(layout.boardHeight,
                      layout.boardWidth,
                      layout.originY + layout.headerHeight,
                      layout.originX);
    infoWin = newwin(layout.sidePanelHeight,
                     layout.sidePanelWidth,
                     layout.originY + layout.headerHeight,
                     layout.originX + layout.boardWidth);
    msgWin = newwin(layout.messageHeight,
                    layout.totalWidth,
                    layout.originY + layout.headerHeight + layout.boardHeight,
                    layout.originX);
    if (!titleWin || !boardWin || !infoWin || !msgWin) {
        destroyWindows();
        showTerminalSizeWarning(layout.totalHeight, layout.totalWidth, hasColor);
        return;
    }

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
        "  ________         .__       .___                       .__      ",
        " /  _____/    ____ |  |    __| _/______ __ __     _____ |  |__   ",
        "/   \\  ___  /  _\\|  |   / __ |\\_  __ \\  |  \\/  ___/|     \\ ",
        "\\   \\_\\  (  <_> )  |__/ /_/ |  |  | \\/  |  /\\___ \\|   Y  \\",
        " \\______  /\\____/|____/\\____|  |__|   |____/  /____  >___|  / ",
        "        \\/                  \\/                     \\/    \\/  "
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
    static const int flashPairs[] = {
        GOLDRUSH_PLAYER_ONE,
        GOLDRUSH_PLAYER_TWO,
        GOLDRUSH_PLAYER_THREE,
        GOLDRUSH_PLAYER_FOUR
    };

    nodelay(msgWin, TRUE);
    for (int flash = 0; flash < 8; ++flash) {
        werase(msgWin);
        drawBoxSafe(msgWin);
        const int msgW = getmaxx(msgWin);
        const int contentW = std::max(1, msgW - 4);
        mvwprintw(msgWin, 1, 2, "%s", clipUiText(title, static_cast<std::size_t>(contentW)).c_str());

        const int colorPair = flashPairs[flash % 4];
        if (hasColor) {
            wattron(msgWin, COLOR_PAIR(colorPair) | A_BOLD | ((flash % 2 == 0) ? A_BLINK : 0));
        } else {
            wattron(msgWin, A_REVERSE | A_BOLD);
        }
        mvwprintw(msgWin, 2, 2, "Spin result: %d!", value);
        if (hasColor) {
            wattroff(msgWin, COLOR_PAIR(colorPair) | A_BOLD | ((flash % 2 == 0) ? A_BLINK : 0));
        } else {
            wattroff(msgWin, A_REVERSE | A_BOLD);
        }
        wrefresh(msgWin);

        const int ch = wgetch(msgWin);
        if (ch != ERR) {
            break;
        }
        napms(90);
    }
    nodelay(msgWin, FALSE);
}

//show shiny number
void Game::showRollResultPopup(int value) const {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    (void)h;

    if (titleWin) {
        touchwin(titleWin);
        wrefresh(titleWin);
    }
    if (boardWin) {
        touchwin(boardWin);
        wrefresh(boardWin);
    }
    if (infoWin) {
        touchwin(infoWin);
        wrefresh(infoWin);
    }
    if (msgWin) {
        touchwin(msgWin);
        wrefresh(msgWin);
    }
    //here
    const std::vector<std::string> banner = bigRollNumberArt(value);
    const std::string title = "YOU ROLLED A";
    static const int flashPairs[] = {
        GOLDRUSH_PLAYER_ONE,
        GOLDRUSH_PLAYER_TWO,
        GOLDRUSH_PLAYER_THREE,
        GOLDRUSH_PLAYER_FOUR
    };
    int contentWidth = static_cast<int>(title.size());
    for (std::size_t i = 0; i < banner.size(); ++i) {
        contentWidth = std::max(contentWidth, static_cast<int>(banner[i].size()));
    }

    const int popupW = std::min(std::max(32, contentWidth + 8), w - 2);
    const int popupH = static_cast<int>(banner.size()) + 6;
    WINDOW* popup = createCenteredWindow(popupH, popupW, 10, 28);
    if (!popup) {
        showTerminalSizeWarning(popupH, 28, hasColor);
        return;
    }
    const int innerWidth = getmaxx(popup) - 4;
    for (int flash = 0; flash < 10; ++flash) {
        werase(popup);
        drawBoxSafe(popup);
        const int colorPair = flashPairs[flash % 4];
        if (hasColor) {
            wattron(popup, COLOR_PAIR(colorPair) | A_BOLD | ((flash % 2 == 0) ? A_BLINK : 0));
        } else {
            wattron(popup, A_BOLD | ((flash % 2 == 0) ? A_REVERSE : 0));
        }
        const std::string clippedTitle = clipUiText(title, static_cast<std::size_t>(std::max(1, innerWidth)));
        mvwprintw(popup, 1, 2 + std::max(0, (innerWidth - static_cast<int>(clippedTitle.size())) / 2),
                  "%s", clippedTitle.c_str());
        for (std::size_t i = 0; i < banner.size(); ++i) {
            const std::string line = clipUiText(banner[i], static_cast<std::size_t>(std::max(1, innerWidth)));
            mvwprintw(popup,
                      3 + static_cast<int>(i),
                      2 + std::max(0, (innerWidth - static_cast<int>(line.size())) / 2),
                      "%s",
                      line.c_str());
        }
        if (hasColor) {
            wattroff(popup, COLOR_PAIR(colorPair) | A_BOLD | ((flash % 2 == 0) ? A_BLINK : 0));
        } else {
            wattroff(popup, A_BOLD | ((flash % 2 == 0) ? A_REVERSE : 0));
        }
        wrefresh(popup);
        napms(120);
    }
    delwin(popup);

    if (titleWin) {
        touchwin(titleWin);
        wrefresh(titleWin);
    }
    if (boardWin) {
        touchwin(boardWin);
        wrefresh(boardWin);
    }
    if (infoWin) {
        touchwin(infoWin);
        wrefresh(infoWin);
    }
    if (msgWin) {
        touchwin(msgWin);
        wrefresh(msgWin);
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
    drawBoxSafe(msgWin);
    const int msgW = getmaxx(msgWin);
    const std::string clippedPrompt = clipMenuText(promptText, static_cast<std::size_t>(std::max(8, msgW - 4)));
    mvwprintw(msgWin, 1, 2, "%s", clippedPrompt.c_str());
    mvwprintw(msgWin,
              2,
              2,
              "%s",
              clipMenuText("Press ENTER for the default name or type a new filename.",
                           static_cast<std::size_t>(std::max(8, msgW - 4))).c_str());
    wmove(msgWin, 1, std::min(msgW - 2, 2 + static_cast<int>(clippedPrompt.size())));
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
    const int popupW = std::min(108, std::max(64, termW - 4));
    WINDOW* popup = createCenteredWindow(popupH, popupW, 10, 64);
    if (!popup) {
        showTerminalSizeWarning(10, 64, hasColor);
        return false;
    }
    keypad(popup, TRUE);

    int popupActualH = 0;
    int popupActualW = 0;
    getmaxyx(popup, popupActualH, popupActualW);

    const int listTop = 4;
    const int listBottom = popupActualH - 4;
    const int visibleRows = std::max(1, listBottom - listTop + 1);
    const int availableWidth = popupActualW - 4;
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
        drawBoxSafe(popup);
        mvwprintw(popup, 1, 2, "Load Game");
        mvwprintw(popup, 2, 2, "%s",
                  clipMenuText("Arrow keys move  Enter load  PgUp/PgDn scroll  ESC cancel",
                               static_cast<std::size_t>(availableWidth)).c_str());
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
        mvwprintw(popup, popupActualH - 3, 2, "%s",
                  clipMenuText(saveMenuStatusText(saves[static_cast<std::size_t>(selectedIndex)]),
                               static_cast<std::size_t>(availableWidth)).c_str());
        mvwprintw(popup, popupActualH - 2, 2, "Showing %d-%d of %d",
                  shownFrom, shownTo, static_cast<int>(saves.size()));
        mvwprintw(popup, popupActualH - 2, popupActualW / 2, "%s",
                  clipMenuText(saves[static_cast<std::size_t>(selectedIndex)].filename,
                               static_cast<std::size_t>(std::max(1, popupActualW / 2 - 4))).c_str());
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
        } else if (ch == 27 || ch == KEY_RESIZE) {
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
        "  ________         .__       .___                       .__      ",
        " /  _____/    ____ |  |    __| _/______ __ __     _____ |  |__   ",
        "/   \\  ___  /  _\\|  |   / __ |\\_  __ \\  |  \\/  ___/|     \\ ",
        "\\   \\_\\  (  <_> )  |__/ /_/ |  |  | \\/  |  /\\___ \\|   Y  \\",
        " \\______  /\\____/|____/\\____|  |__|   |____/  /____  >___|  / ",
        "        \\/                  \\/                     \\/    \\/  "
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
            const std::string subtitle = "A Hasbro-style Life Journey";
    const std::string actions = "N New   L Load   G Guide   H History   ESC Quit";
            mvprintw(startY + 11, std::max(0, (w - static_cast<int>(subtitle.size())) / 2), "%s", subtitle.c_str());
            mvprintw(startY + 13, std::max(0, (w - static_cast<int>(actions.size())) / 2), "%s", actions.c_str());
        } else {
            const std::string help = "ENTER select    ESC back";
            mvprintw(startY + 11, std::max(0, (w - static_cast<int>(help.size())) / 2), "%s", help.c_str());
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
                rules = makeNormalRules();

                if (!chooseBoardViewMode()) {
                    continue;
                }
                return START_NEW_GAME;
            }
            if (ch == 'l' || ch == 'L') return START_LOAD_GAME;
            if (ch == 'g' || ch == 'G') {
                showGuidePopup();
                continue;
            }
            if (ch == 'h' || ch == 'H') {
                showCompletedGameHistoryScreen(hasColor);
                continue;
            }
            if (ch == 27) {
                if (confirmTitleQuit(hasColor)) {
                    return START_QUIT_GAME;
                }
                continue;
            }
        } else {
            if (ch == KEY_LEFT || ch == KEY_UP) {
                highlightedMode = highlightedMode == 0 ? 1 : 0;
                continue;
            }
            if (ch == KEY_RIGHT || ch == KEY_DOWN) {
                highlightedMode = highlightedMode == 1 ? 0 : 1;
                continue;
            }
            if (ch == 27) {
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

bool Game::chooseBoardViewMode() {
    int selected = boardViewMode == BoardViewMode::ClassicFull ? 1 : 0;

    while (true) {
        int h = 0;
        int w = 0;
        getmaxyx(stdscr, h, w);
        (void)h;
        const int popupW = std::min(78, std::max(60, w - 6));
        const int popupH = 14;
        WINDOW* popup = createCenteredWindow(popupH, popupW, 12, 50);
        if (!popup) {
            showTerminalSizeWarning(12, 50, hasColor);
            return false;
        }
        keypad(popup, TRUE);
        int popupActualH = 0;
        int popupActualW = 0;
        getmaxyx(popup, popupActualH, popupActualW);

        while (true) {
            werase(popup);
            drawBoxSafe(popup);
            if (hasColor) wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
            mvwprintw(popup, 1, 2, "Choose Board Display");
            if (hasColor) wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);

            const char* names[] = {
                "Follow Camera Mode",
                "Classic / Full Board Mode"
            };
            const char* descriptions[] = {
                "Zoomed route view centered on the current player. Minimap stays in the side panel.",
                "Full route overview using the classic board symbols, regions, and landmarks."
            };

            for (int i = 0; i < 2; ++i) {
                const int row = 4 + (i * 3);
                if (selected == i) wattron(popup, A_REVERSE | A_BOLD);
                mvwprintw(popup, row, 4, "%s",
                          clipUiText(std::to_string(i + 1) + ". " + names[i],
                                     static_cast<std::size_t>(std::max(1, popupActualW - 6))).c_str());
                if (selected == i) wattroff(popup, A_REVERSE | A_BOLD);
                mvwprintw(popup,
                          row + 1,
                          7,
                          "%s",
                          clipUiText(descriptions[i], static_cast<std::size_t>(popupActualW - 10)).c_str());
            }

            mvwprintw(popup, popupActualH - 3, 2, "%s",
                      clipUiText("Use UP/DOWN or A/D. ENTER select. ESC back.",
                                 static_cast<std::size_t>(std::max(1, popupActualW - 4))).c_str());
            mvwprintw(popup, popupActualH - 2, 2, "%s",
                      clipUiText(std::string("Selected: ") + names[selected],
                                 static_cast<std::size_t>(std::max(1, popupActualW - 4))).c_str());
            wrefresh(popup);

            const int ch = wgetch(popup);
            if (ch == KEY_RESIZE) {
                delwin(popup);
                if (!ensureMinSize()) {
                    return false;
                }
                break;
            }
            if (ch == KEY_UP || ch == KEY_DOWN ||
                ch == KEY_LEFT || ch == KEY_RIGHT ||
                ch == 'a' || ch == 'A' ||
                ch == 'd' || ch == 'D' ||
                ch == 'w' || ch == 'W' ||
                ch == 's' || ch == 'S') {
                selected = selected == 0 ? 1 : 0;
            } else if (ch == '1' || ch == '2') {
                selected = ch - '1';
            } else if (ch == 27) {
                delwin(popup);
                return false;
            } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                boardViewMode = selected == 0
                    ? BoardViewMode::FollowCamera
                    : BoardViewMode::ClassicFull;
                delwin(popup);
                return true;
            }
        }
    }
}

bool Game::configureCustomRules() {
    WINDOW* popup = createCenteredWindow(18, 72, 18, 56);
    if (!popup) {
        showTerminalSizeWarning(18, 56, hasColor);
        return false;
    }
    keypad(popup, TRUE);
    int popupH = 0;
    int popupW = 0;
    getmaxyx(popup, popupH, popupW);
    const int contentW = std::max(1, popupW - 4);

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
        drawBoxSafe(popup);
        mvwprintw(popup, 1, 2, "Custom Mode");
        mvwprintw(popup, 2, 2, "%s",
                  clipUiText("Toggle rules with SPACE or ENTER. Select Start Game when ready.",
                             static_cast<std::size_t>(contentW)).c_str());

        for (size_t i = 0; i < rows.size(); ++i) {
            if (static_cast<int>(i) == highlight) wattron(popup, A_REVERSE);
            mvwprintw(popup, 4 + static_cast<int>(i), 2, "%s",
                      clipUiText(std::string("[") + (*rows[i].value ? 'x' : ' ') + "] " + rows[i].label,
                                 static_cast<std::size_t>(contentW)).c_str());
            if (static_cast<int>(i) == highlight) wattroff(popup, A_REVERSE);
        }

        if (highlight == startRowIndex) wattron(popup, A_REVERSE);
        mvwprintw(popup, popupH - 3, 2, "Start Game");
        if (highlight == startRowIndex) wattroff(popup, A_REVERSE);
        mvwprintw(popup, popupH - 2, 2, "%s",
                  clipUiText("Up/Down move  Enter/Space toggle  ESC close config",
                             static_cast<std::size_t>(contentW)).c_str());
        wrefresh(popup);

        int ch = wgetch(popup);
        if (ch == KEY_UP) {
            highlight = highlight == 0 ? startRowIndex : highlight - 1;
        } else if (ch == KEY_DOWN) {
            highlight = highlight == startRowIndex ? 0 : highlight + 1;
        } else if (ch == 27) {
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
    showPreGameQuickGuide(hasColor);
    addHistory("Quick guide reviewed");
}

void Game::showGuidePopup() const {
    showFullGuide(board,
                  rules,
                  sabotageUnlockAnnounced,
                  hasColor);
}

void Game::resetTutorialFlags() {
    tutorialFlags = TutorialFlags();
    sabotageUnlockAnnounced = false;
}

void Game::maybeShowFirstTimeTutorial(TutorialTopic topic) {
    if (!rules.toggles.tutorialEnabled) {
        return;
    }
    bool& seen = tutorialFlagForTopic(tutorialFlags, topic);
    if (seen) {
        return;
    }
    showFirstTimeTutorial(topic, hasColor);
    seen = true;
}

bool Game::isSabotageUnlockedForPlayer(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= static_cast<int>(players.size())) {
        return false;
    }
    const int unlockTurn = std::max(1, settings.sabotageUnlockTurn);
    return players[static_cast<std::size_t>(playerIndex)].turnsTaken + 1 >= unlockTurn;
}

void Game::maybeShowSabotageUnlock(int playerIndex) {
    if (sabotageUnlockAnnounced || !isSabotageUnlockedForPlayer(playerIndex)) {
        return;
    }
    showSabotageUnlockAnimation(hasColor);
    showSabotageTutorial(hasColor);
    tutorialFlagForTopic(tutorialFlags, TutorialTopic::Sabotage) = true;
    sabotageUnlockAnnounced = true;
    addHistory("Sabotage unlocked from Turn " + std::to_string(std::max(1, settings.sabotageUnlockTurn)));
}

void Game::showControlsPopup() const {
    WINDOW* popup = createCenteredWindow(16, 70, 16, 50);
    if (!popup) {
        showTerminalSizeWarning(16, 50, hasColor);
        return;
    }
    drawBoxSafe(popup);
    int popupH = 0;
    int popupW = 0;
    getmaxyx(popup, popupH, popupW);
    const int contentW = std::max(1, popupW - 4);
    mvwprintw(popup, 1, 2, "Controls");
    const std::vector<std::string> lines{
        "ENTER  Confirm a menu or start your turn spin",
        "SPACE  Hold/release to spin the wheel",
        "UP/DN  Move through menus and custom mode toggles",
        "TAB    Open the player scoreboard and minimap",
        "G      Open the tile abbreviation guide",
        "B      Open sabotage and defense actions",
        "S      Save the current game",
        "K/?    Open this controls popup",
        "ESC    Open the quit menu / back out",
        "STOP spaces end movement immediately.",
        "College tuition is paid as soon as College route is chosen."
    };
    for (int i = 0; i < static_cast<int>(lines.size()) && 3 + i < popupH - 2; ++i) {
        mvwprintw(popup, 3 + i, 2, "%s",
                  clipUiText(lines[static_cast<std::size_t>(i)],
                             static_cast<std::size_t>(contentW)).c_str());
    }
    mvwprintw(popup, popupH - 2, 2, "%s",
              clipUiText("Press ENTER", static_cast<std::size_t>(contentW)).c_str());
    wrefresh(popup);
    waitForEnter(popup, popupH - 2, std::min(popupW - 2, 15), "");
    delwin(popup);
}

void Game::showScoreboardPopup() const {
    int h, w;
    getmaxyx(stdscr, h, w);
    const int popupH = std::min(28, std::max(18, h - 4));
    const int popupW = std::min(104, std::max(92, w - 8));
    WINDOW* popup = createCenteredWindow(popupH, popupW, 18, 70);
    if (!popup) {
        showTerminalSizeWarning(18, 70, hasColor);
        return;
    }
    keypad(popup, TRUE);
    drawBoxSafe(popup);
    int actualPopupH = 0;
    int actualPopupW = 0;
    getmaxyx(popup, actualPopupH, actualPopupW);

    WINDOW* scoreWin = derwin(popup, actualPopupH - 4, 56, 2, 2);
    WINDOW* mapWin = derwin(popup, actualPopupH - 4, actualPopupW - 61, 2, 59);
    if (!scoreWin || !mapWin) {
        if (mapWin) delwin(mapWin);
        if (scoreWin) delwin(scoreWin);
        delwin(popup);
        showTerminalSizeWarning(18, 70, hasColor);
        return;
    }
    applyWindowBg(scoreWin);
    applyWindowBg(mapWin);

    blinkIndicator(popup, 1, 2, "Scoreboard + Minimap", hasColor, GOLDRUSH_GOLD_SAND, 2, 0, actualPopupW - 4);

    werase(scoreWin);
    drawBoxSafe(scoreWin);
    const int scoreWinH = getmaxy(scoreWin);
    const int scoreWinW = getmaxx(scoreWin);
    mvwprintw(scoreWin, 1, 2, "SCOREBOARD");
    mvwprintw(scoreWin, 2, 2, "Rk %-10s %-7s %-7s %-7s %-4s %-7s",
              "Player", "Type", "Space", "Cash", "Loan", "Worth");

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
        const std::string name = clipMenuText(player.name, 10);
        const std::string tileText = clipMenuText(std::to_string(player.tile) + " " + getTileAbbreviation(tile), 7);
        const std::string typeText = player.type == PlayerType::CPU
            ? clipMenuText("CPU-" + cpuDifficultyLabel(player.cpuDifficulty), 7)
            : "Human";

        if (y >= scoreWinH - 5) {
            break;
        }

        if (playerIndex == currentPlayerIndex) {
            wattron(scoreWin, A_REVERSE);
        }
        mvwprintw(scoreWin,
                  y,
                  2,
                  "%-2d %-10s %-7s %-7s $%-6d %-4d $%-6d",
                  static_cast<int>(row + 1),
                  name.c_str(),
                  typeText.c_str(),
                  tileText.c_str(),
                  player.cash,
                  player.loans,
                  calculateFinalWorth(player));
        if (playerIndex == currentPlayerIndex) {
            wattroff(scoreWin, A_REVERSE);
        }
    }

    mvwprintw(scoreWin, scoreWinH - 4, 2, "%-*s", scoreWinW - 4, "Highlighted row has the current turn.");
    mvwprintw(scoreWin, scoreWinH - 3, 2, "Route: %s",
              clipMenuText(playerRouteText(players[currentPlayerIndex]), scoreWinW - 11).c_str());
    mvwprintw(scoreWin, scoreWinH - 2, 2, "Now: %s on %d - %s",
              players[currentPlayerIndex].name.c_str(),
              players[currentPlayerIndex].tile,
              clipMenuText(getTileDisplayName(board.tileAt(players[currentPlayerIndex].tile)), scoreWinW - 18).c_str());
    wrefresh(scoreWin);

    drawMinimapPanel(mapWin, board, players, currentPlayerIndex);

    mvwprintw(popup, actualPopupH - 2, 2, "Close with TAB, ENTER, or ESC.");
    wrefresh(popup);

    while (true) {
        const int ch = wgetch(popup);
        if (ch == '\t' || ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 27) {
            break;
        }
    }

    delwin(mapWin);
    delwin(scoreWin);
    delwin(popup);
}

void Game::showTileGuidePopup() const {
    int h, w;
    getmaxyx(stdscr, h, w);
    (void)h;
    (void)w;
    const std::vector<std::string> legend = board.tutorialLegend();
    const int popupH = 24;
    const int popupW = 82;
    WINDOW* popup = createCenteredWindow(popupH, popupW, 16, 54);
    if (!popup) {
        showTerminalSizeWarning(16, 54, hasColor);
        return;
    }
    drawBoxSafe(popup);
    int actualH = 0;
    int actualW = 0;
    getmaxyx(popup, actualH, actualW);

    blinkIndicator(popup, 1, 2, "Quick Tutorial: Tile Guide", hasColor, GOLDRUSH_GOLD_SAND, 2, 0, actualW - 4);
    mvwprintw(popup, 2, 2, "%s",
              clipUiText("Board abbreviations show as Full Name (ABBR)",
                         static_cast<std::size_t>(std::max(1, actualW - 4))).c_str());

    const bool twoColumns = actualW >= 78;
    const int rowsPerColumn = twoColumns ? 10 : std::max(1, actualH - 6);
    const int columnWidth = twoColumns ? std::max(1, (actualW - 6) / 2) : std::max(1, actualW - 4);
    for (size_t i = 0; i < legend.size(); ++i) {
        const int column = twoColumns ? static_cast<int>(i) / rowsPerColumn : 0;
        if (column > 1) {
            break;
        }
        const int columnX = twoColumns ? (2 + column * (columnWidth + 2)) : 2;
        const int rowY = 4 + static_cast<int>(i % rowsPerColumn);
        if (rowY >= actualH - 2) {
            break;
        }
        mvwprintw(popup, rowY, columnX, "%s",
                  clipUiText(legend[i], static_cast<std::size_t>(columnWidth)).c_str());
    }

    mvwprintw(popup, actualH - 2, 2, "%s",
              clipUiText("Press ENTER", static_cast<std::size_t>(std::max(1, actualW - 4))).c_str());
    wrefresh(popup);
    waitForEnter(popup, actualH - 2, 15, "");
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
    const std::string detail =
        "CPU " + cpuDifficultyLabel(player.cpuDifficulty) + " | " + action +
        " | " + cpuReasonForAction(player, action);
    renderGame(playerIndex, player.name + " is thinking...", detail);
    napms(autoAdvanceUi ? 300 : 900);
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

std::string cpuReasonForAction(const Player& player, const std::string& action) {
    switch (player.cpuDifficulty) {
        case CpuDifficulty::Easy:
            return "Easy CPU uses simple, sometimes random choices.";
        case CpuDifficulty::Hard:
            if (action.find("Career") != std::string::npos ||
                action.find("career") != std::string::npos) {
                return "Hard CPU favors the highest salary or strongest long-term option available.";
            }
            if (action.find("Risk") != std::string::npos ||
                action.find("risky") != std::string::npos) {
                return "Hard CPU weighs cash, loans, and route upside before choosing.";
            }
            if (action.find("sabotage") != std::string::npos ||
                action.find("Sabotage") != std::string::npos) {
                return "Hard CPU targets the player with the strongest visible position.";
            }
            return "Hard CPU picks the option with the best expected long-term value.";
        case CpuDifficulty::Normal:
        default:
            return "Normal CPU chooses a practical option with a basic money or progress benefit.";
    }
}

std::string describeTileEffectText(const Tile& tile) {
    switch (tile.kind) {
        case TILE_BLACK:
            return "Starts a minigame. Payout depends on performance.";
        case TILE_START:
            return "The journey begins.";
        case TILE_SPLIT_START:
            return "Choose College or Career before moving ahead.";
        case TILE_COLLEGE:
            return "College route tuition is paid before the route begins.";
        case TILE_CAREER:
            return "Choose a career card and set salary.";
        case TILE_GRADUATION:
            return "College players choose a degree career.";
        case TILE_MARRIAGE:
            return "Resolve marriage and gift spin.";
        case TILE_SPLIT_FAMILY:
            return "Choose Family path or Life path.";
        case TILE_FAMILY:
            return "Resolve the Family path choice.";
        case TILE_NIGHT_SCHOOL:
            return "Optional career upgrade if Night School is enabled.";
        case TILE_SPLIT_RISK:
            return "Choose Safe route or Risky route.";
        case TILE_SAFE:
            return "Collect a safe-route payout.";
        case TILE_RISKY:
            return "Spin for a high-risk reward or penalty.";
        case TILE_SPIN_AGAIN:
            return "Take another movement spin immediately.";
        case TILE_CAREER_2:
            return "Promotion increases salary.";
        case TILE_PAYDAY:
            return "Receive salary plus this space payout.";
        case TILE_BABY:
            return "Family event adds children based on the tile.";
        case TILE_RETIREMENT:
            return "Choose retirement destination and final bonus.";
        case TILE_HOUSE:
            return "Draw and buy a house.";
        case TILE_EMPTY:
        default:
            return "No special effect.";
    }
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

    WINDOW* popup = createCenteredWindow(14, 68, 10, 42);
    if (!popup) {
        showTerminalSizeWarning(10, 42, hasColor);
        return -1;
    }
    keypad(popup, TRUE);
    int popupH = 0;
    int popupW = 0;
    getmaxyx(popup, popupH, popupW);
    const int contentW = std::max(1, popupW - 4);
    int selected = 0;
    while (true) {
        werase(popup);
        drawBoxSafe(popup);
        mvwprintw(popup, 1, 2, "Choose Sabotage Target");
        const int visibleRows = std::max(1, popupH - 5);
        for (size_t row = 0; row < candidates.size() && static_cast<int>(row) < visibleRows; ++row) {
            const int playerIndex = candidates[row];
            if (static_cast<int>(row) == selected) {
                wattron(popup, A_REVERSE);
            }
            const Player& target = players[static_cast<std::size_t>(playerIndex)];
            mvwprintw(popup, 3 + static_cast<int>(row), 2, "%s",
                      clipUiText(target.name + "  cash $" + std::to_string(target.cash) +
                                     "  worth $" + std::to_string(calculateFinalWorth(target)),
                                 static_cast<std::size_t>(contentW)).c_str());
            if (static_cast<int>(row) == selected) {
                wattroff(popup, A_REVERSE);
            }
        }
        mvwprintw(popup, popupH - 2, 2, "%s",
                  clipUiText("Up/Down move  ENTER select  ESC cancel",
                             static_cast<std::size_t>(contentW)).c_str());
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
        } else if (ch == 27) {
            delwin(popup);
            return -1;
        }
    }
}

int Game::chooseTrapTile(int attackerIndex) {
    Player& player = players[static_cast<std::size_t>(attackerIndex)];
    std::string errorText;
    while (true) {
        echo();
        curs_set(1);
        werase(msgWin);
        drawBoxSafe(msgWin);
        mvwprintw(msgWin, 1, 2, "Trap tile id (0-%d) [%d]: ", TILE_COUNT - 1, player.tile);
        mvwprintw(msgWin, 2, 2, "Tip: choose a tile ahead of the leader. Blank uses your current tile.");
        if (!errorText.empty()) {
            mvwprintw(msgWin, 3, 2, "%s", errorText.c_str());
        }
        wrefresh(msgWin);

        char buffer[16] = {0};
        wgetnstr(msgWin, buffer, 15);
        noecho();
        curs_set(0);

        if (std::strlen(buffer) == 0) {
            return player.tile;
        }

        int tileId = -1;
        if (!parseStrictInt(buffer, tileId) || tileId < 0 || tileId >= TILE_COUNT) {
            errorText = "Invalid tile id. Enter a number from 0 to " + std::to_string(TILE_COUNT - 1) + ".";
            continue;
        }
        return tileId;
    }
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
        detail += " Automatic loans: " + std::to_string(payment.loansTaken) + ".";
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
    SabotageResult result;
    const bool humanDrivenSabotage = !isCpuPlayer(attackerIndex);
    if (type == SabotageType::ForceMinigame && humanDrivenSabotage) {
        std::string shieldText;
        if (sabotage.consumeShield(target, shieldText)) {
            result.blocked = true;
            result.summary = shieldText;
        } else {
            const int duelRoll = rollSpinner(card.name, "Spin to see if the duel challenge lands");
            result.attempted = true;
            result.roll = duelRoll;
            result.critical = duelRoll >= 8;
            if (duelRoll <= 3) {
                result.summary = card.name + " failed on roll " + std::to_string(duelRoll) + ".";
            } else {
                result.summary = resolveDuelMinigameAction(attackerIndex, result.amount, targetIndex, 40000);
                result.success = result.amount >= 0;
            }
        }
    } else if (card.name == "Lawsuit" && humanDrivenSabotage) {
        std::string shieldText;
        if (sabotage.consumeShield(target, shieldText)) {
            result.blocked = true;
            result.summary = shieldText;
        } else {
            const int attackerRoll = rollSpinner("Lawsuit", attacker.name + " spins the case");
            const int targetRoll = isCpuPlayer(targetIndex)
                ? rng.roll10()
                : rollSpinner("Lawsuit Defense", target.name + " spins the defense");
            result = sabotage.resolveLawsuit(attacker, target, attackerRoll, targetRoll);
        }
    } else if (card.requiresDiceRoll && humanDrivenSabotage) {
        const int sabotageRoll = rollSpinner(card.name, "Spin to see whether the sabotage lands");
        result = sabotage.applyDirectSabotage(card,
                                              attacker,
                                              target,
                                              players,
                                              attackerIndex,
                                              targetIndex,
                                              sabotageRoll);
    } else {
        result = sabotage.applyDirectSabotage(card,
                                              attacker,
                                              target,
                                              players,
                                              attackerIndex,
                                              targetIndex);
    }
    if (type == SabotageType::PositionSwap && result.success) {
        const int cooldown = result.critical ? 3 : 4;
        attacker.sabotageCooldown = std::max(attacker.sabotageCooldown, cooldown + 1);
    } else if (result.success || result.blocked) {
        attacker.sabotageCooldown = std::max(attacker.sabotageCooldown, 2);
    }

    std::string detail = result.summary;
    if (cost.loansTaken > 0) {
        detail += " Cost automatic loans: " + std::to_string(cost.loansTaken) + ".";
    }
    addHistory(attacker.name + " used " + card.name + " on " + target.name);
    addHistory(detail);
    showInfoPopup(card.name, detail);
}

bool Game::promptSabotageMenu(int attackerIndex) {
    Player& attacker = players[static_cast<std::size_t>(attackerIndex)];
    if (!isSabotageUnlockedForPlayer(attackerIndex)) {
        showInfoPopup("Sabotage locked",
                      "Sabotage unlocks on Turn " + std::to_string(std::max(1, settings.sabotageUnlockTurn)) + ".");
        return false;
    }
    WINDOW* popup = createCenteredWindow(19, 82, 19, 58);
    if (!popup) {
        showTerminalSizeWarning(19, 58, hasColor);
        return false;
    }
    keypad(popup, TRUE);
    int popupH = 0;
    int popupW = 0;
    getmaxyx(popup, popupH, popupW);
    const int contentW = std::max(1, popupW - 4);
    std::string status;

    while (true) {
        werase(popup);
        drawBoxSafe(popup);
        mvwprintw(popup, 1, 2, "%s",
                  clipUiText("Sabotage Menu - " + attacker.name, static_cast<std::size_t>(contentW)).c_str());
        mvwprintw(popup, 2, 2, "%s",
                  clipUiText("Cash $" + std::to_string(attacker.cash) +
                                 "  Shields " + std::to_string(attacker.shieldCards) +
                                 "  Insurance " + std::to_string(attacker.insuranceUses) +
                                 "  Cooldown " + std::to_string(attacker.sabotageCooldown),
                             static_cast<std::size_t>(contentW)).c_str());
        const std::vector<std::string> options{
            "1 Trap Tile ($12000)",
            "2 Lawsuit ($15000)",
            "3 Traffic Jam ($10000)",
            "4 Steal Action Card ($18000)",
            "5 Forced Duel Minigame ($22000)",
            "6 Career Sabotage ($24000)",
            "7 Position Swap ($90000, cooldown)",
            "8 Debt Trap ($20000)",
            "9 Buy Shield Card ($15000)",
            "0 Buy Insurance ($20000, 2 uses)",
            "D Item Disable ($16000)"
        };
        for (int i = 0; i < static_cast<int>(options.size()) && 4 + i < popupH - 4; ++i) {
            mvwprintw(popup, 4 + i, 2, "%s",
                      clipUiText(options[static_cast<std::size_t>(i)],
                                 static_cast<std::size_t>(contentW)).c_str());
        }
        if (!status.empty()) {
            mvwprintw(popup, popupH - 3, 2, "%s",
                      clipUiText(status, static_cast<std::size_t>(contentW)).c_str());
        }
        mvwprintw(popup, popupH - 2, 2, "%s",
                  clipUiText("Choose option or ESC cancel", static_cast<std::size_t>(contentW)).c_str());
        wrefresh(popup);

        const int ch = wgetch(popup);
        if (ch == 27) {
            delwin(popup);
            return false;
        }
        if (ch == '9') {
            delwin(popup);
            PaymentResult payment = bank.charge(attacker, 15000);
            ++attacker.shieldCards;
            std::string detail = "Shield Card added. Blocks one future sabotage.";
            if (payment.loansTaken > 0) {
                detail += " Automatic loans: " + std::to_string(payment.loansTaken) + ".";
            }
            showInfoPopup("Shield Card", detail);
            return true;
        }
        if (ch == '0') {
            delwin(popup);
            PaymentResult payment = bank.charge(attacker, 20000);
            attacker.insuranceUses += 2;
            std::string detail = "Insurance added. Next 2 money/property hits are halved.";
            if (payment.loansTaken > 0) {
                detail += " Automatic loans: " + std::to_string(payment.loansTaken) + ".";
            }
            showInfoPopup("Insurance", detail);
            return true;
        }
        if (ch == '1') {
            delwin(popup);
            const int tileId = chooseTrapTile(attackerIndex);
            if (tileId < 0) return false;
            const int trapChoice = showBranchPopup(
                "Choose Trap Effect",
                std::vector<std::string>{
                    "- Money loss: target pays cash",
                    "- Backward move: target is pushed back",
                    "- Skip turn: target loses their next turn",
                    "- Lose card: target drops an action card",
                    "- Minigame: target is forced into a duel"
                },
                'A',
                'B');
            SabotageType trapType = SabotageType::MoneyLoss;
            if (trapChoice == 1) trapType = SabotageType::MovementPenalty;
            else if (trapChoice == 2) trapType = SabotageType::SkipTurn;
            else if (trapChoice == 3) trapType = SabotageType::StealCard;
            else if (trapChoice == 4) trapType = SabotageType::ForceMinigame;
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
        else {
            status = "Invalid choice. Use 0-9, D, or ESC.";
            continue;
        }

        delwin(popup);
        const int targetIndex = chooseSabotageTarget(attackerIndex);
        if (targetIndex >= 0) {
            executeSabotage(attackerIndex, targetIndex, type);
            return true;
        }
        return false;
    }
}

void Game::maybeCpuSabotage(int playerIndex) {
    if (!isCpuPlayer(playerIndex) ||
        !isSabotageUnlockedForPlayer(playerIndex) ||
        !cpu.shouldUseSabotage(players[playerIndex], players[playerIndex].turnsTaken)) {
        return;
    }

    const int targetIndex = cpu.chooseSabotageTarget(players[playerIndex], players, playerIndex);
    if (targetIndex < 0) {
        return;
    }

    const SabotageType type = cpu.chooseSabotageType(players[playerIndex],
                                                    players[static_cast<std::size_t>(targetIndex)],
                                                    players[playerIndex].turnsTaken);
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
        SabotageResult result = isCpuPlayer(playerIndex)
            ? sabotage.triggerTrap(trap, player)
            : sabotage.triggerTrap(trap,
                                   player,
                                   rollSpinner("Trap Trigger", "Spin to see whether the trap catches you"));
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
    validateGameSettings(settings);
    rules = makeNormalRules();
    applyGameSettingsToRules(settings, rules);
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
    noecho();
    curs_set(0);
    drawSetupTitle();
    int numPlayers = 0;
    while (numPlayers < 2 || numPlayers > 4) {
        werase(msgWin);
        drawBoxSafe(msgWin);
        const int msgW = getmaxx(msgWin);
        const int contentW = std::max(1, msgW - 4);
        mvwprintw(msgWin, 1, std::max(2, (msgW - 16) / 2), "Players (2-4):");
        mvwprintw(msgWin, 3, 2, "%s",
                  clipUiText("Enter number of players: ", static_cast<std::size_t>(contentW)).c_str());
        wrefresh(msgWin);

        echo();
        curs_set(1);
        char inputBuf[8] = {0};
        wgetnstr(msgWin, inputBuf, 7);
        noecho();
        curs_set(0);

        if (inputBuf[0] >= '2' && inputBuf[0] <= '4' && inputBuf[1] == '\0') {
            numPlayers = inputBuf[0] - '0';
            break;
        }

        werase(msgWin);
        drawBoxSafe(msgWin);
        mvwprintw(msgWin, 1, 2, "%s",
                  clipUiText("Invalid input. Please enter 2, 3, or 4.",
                             static_cast<std::size_t>(contentW)).c_str());
        mvwprintw(msgWin, 2, 2, "%s",
                  clipUiText("Press any key to try again.", static_cast<std::size_t>(contentW)).c_str());
        wrefresh(msgWin);
        wgetch(msgWin);
        drawSetupTitle();
    }
    showInfoPopup("Game setup",
                  "You chose " + std::to_string(numPlayers) +
                  " players. Next, choose each player's type and name.");

    players.clear();
    players.reserve(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        drawSetupTitle();
        const int typeChoice = showBranchPopup(
            "Choose Player " + std::to_string(i + 1) + " of " + std::to_string(numPlayers) + " type",
            std::vector<std::string>{
                "- Human: this player uses keyboard input",
                "- CPU: computer-controlled opponent"
            },
            'A',
            'B');
        PlayerType playerType = typeChoice == 0 ? PlayerType::Human : PlayerType::CPU;
        CpuDifficulty difficulty = CpuDifficulty::Normal;
        if (playerType == PlayerType::CPU) {
            drawSetupTitle();
            const int difficultyChoice = showBranchPopup(
                "Choose CPU " + std::to_string(i + 1) + " difficulty",
                std::vector<std::string>{
                    "- Easy: more mistakes and simpler choices",
                    "- Normal: balanced decisions",
                    "- Hard: stronger money and route decisions"
                },
                'A',
                'B');
            if (difficultyChoice == 0) {
                difficulty = CpuDifficulty::Easy;
            } else if (difficultyChoice == 2) {
                difficulty = CpuDifficulty::Hard;
            } else {
                difficulty = CpuDifficulty::Normal;
            }
        }

        drawSetupTitle();
        echo();
        curs_set(1);
        werase(msgWin);
        drawBoxSafe(msgWin);
        const int msgW = getmaxx(msgWin);
        const int contentW = std::max(1, msgW - 4);
        mvwprintw(msgWin,
                  1,
                  std::max(2, (msgW - 26) / 2),
                  "Setting up player %d of %d",
                  i + 1,
                  numPlayers);
        if (playerType == PlayerType::CPU) {
            mvwprintw(msgWin, 3, 2, "%s",
                      clipUiText("CPU " + std::to_string(i + 1) +
                                     " name [CPU " + std::to_string(i + 1) + "]: ",
                                 static_cast<std::size_t>(contentW)).c_str());
        } else {
            mvwprintw(msgWin, 3, 2, "%s",
                      clipUiText("Player " + std::to_string(i + 1) + " name: ",
                                 static_cast<std::size_t>(contentW)).c_str());
        }
        wrefresh(msgWin);
        char nameBuf[32] = {0};
        wgetnstr(msgWin, nameBuf, 31);
        noecho();
        curs_set(0);

        Player p;
        p.name = nameBuf;
        if (p.name.empty()) {
            p.name = playerType == PlayerType::CPU
                ? "CPU " + std::to_string(i + 1)
                : "Player " + std::to_string(i + 1);
        }
        p.token = tokenForName(p.name, i);
        p.tile = 0;
        p.cash = settings.startingCash;
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
                return 27;
            }
            destroyWindows();
            createWindows();
            renderGame(currentPlayer,
                       players[currentPlayer].name + "'s turn",
                       turnPromptText());
            continue;
        }
        if (ch == 27) return 27;
        if (ch == 's' || ch == 'S') return ch;
        if (ch == 'b' || ch == 'B') return ch;
        if (ch == '\t') {
            showScoreboardPopup();
            renderGame(currentPlayer,
                       players[currentPlayer].name + "'s turn",
                       turnPromptText());
            continue;
        }
        if (ch == 'g' || ch == 'G') {
            showGuidePopup();
            renderGame(currentPlayer,
                       players[currentPlayer].name + "'s turn",
                       turnPromptText());
            continue;
        }
        if (ch == 'k' || ch == 'K' || ch == '?') {
            showControlsPopup();
            renderGame(currentPlayer, players[currentPlayer].name + "'s turn", turnPromptText());
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
    draw_board_ui(boardWin, board, players, currentPlayer, players[currentPlayer].tile, boardViewMode);
    draw_sidebar_ui(infoWin, board, players, currentPlayer, history.recent(), rules);
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
    int msgHeight = 0;
    int msgWidth = 0;
    getmaxyx(msgWin, msgHeight, msgWidth);
    drawEventMessage(msgWin,
                     clipUiText(line1, static_cast<std::size_t>(std::max(8, msgWidth - 12))),
                     clipUiText(line2, static_cast<std::size_t>(std::max(8, msgWidth - 4))));
    blinkIndicator(msgWin,
                   1,
                   2,
                   clipUiText(line1, static_cast<std::size_t>(std::max(8, msgWidth - 12))),
                   hasColor,
                   GOLDRUSH_GOLD_SAND,
                   2,
                   2000,
                   std::max(8, msgWidth - 4));
    if (autoAdvanceUi) {
        return;
    }
    const std::string prompt = "Press ENTER to continue...";
    waitForEnterPrompt(msgWin,
                       std::max(1, msgHeight - 2),
                       std::max(2, (msgWidth - static_cast<int>(prompt.size())) / 2),
                       prompt);
}

void Game::showTurnSummaryPopup(int playerIndex,
                                int rolledValue,
                                int movementValue,
                                int startTile,
                                int endTile,
                                int startingCash,
                                int startingLoans,
                                const std::string& reason) const {
    if (playerIndex < 0 || playerIndex >= static_cast<int>(players.size())) {
        return;
    }

    const Player& player = players[static_cast<std::size_t>(playerIndex)];
    const Tile& start = board.tileAt(startTile);
    const Tile& end = board.tileAt(endTile);
    std::vector<std::string> lines;
    lines.push_back(player.name + "'s Turn");
    lines.push_back("Player Type: " + playerTypeLabel(player.type) +
                    (player.type == PlayerType::CPU
                         ? " (" + cpuDifficultyLabel(player.cpuDifficulty) + ")"
                         : ""));
    lines.push_back("Start Money: $" + std::to_string(startingCash) +
                    " | Loans: " + std::to_string(startingLoans));
    lines.push_back("Start Position: Space " + std::to_string(startTile) +
                    " - " + getTileDisplayName(start));
    lines.push_back(reason + ": rolled " + std::to_string(rolledValue) +
                    (movementValue != rolledValue
                         ? " | movement after effects " + std::to_string(movementValue)
                         : ""));
    lines.push_back(player.name + " moved from Space " + std::to_string(startTile) +
                    " to Space " + std::to_string(endTile) + ".");
    lines.push_back("Landed On: " + getTileDisplayName(end));
    lines.push_back("Effect: " + describeTileEffectText(end));
    const int cashDelta = player.cash - startingCash;
    const int loanDelta = player.loans - startingLoans;
    std::string delta = "Money Change: ";
    if (cashDelta >= 0) {
        delta += "+$" + std::to_string(cashDelta);
    } else {
        delta += "-$" + std::to_string(-cashDelta);
    }
    delta += " | Loan Change: ";
    delta += loanDelta >= 0 ? "+" + std::to_string(loanDelta) : std::to_string(loanDelta);
    lines.push_back(delta);
    lines.push_back("End Money: $" + std::to_string(player.cash) +
                    " | Loans: " + std::to_string(player.loans));
    TurnSummary summary;
    summary.playerName = player.name;
    summary.turnNumber = player.turnsTaken + 1;
    summary.moneyChange = player.cash - startingCash;
    summary.loanChange = player.loans - startingLoans;
    summary.babyChange = 0;
    summary.petChange = 0;
    summary.investmentChange = 0;
    summary.shieldChange = 0;
    summary.insuranceChange = 0;
    summary.gotMarried = player.married && board.tileAt(endTile).kind == TILE_MARRIAGE;
    summary.jobChanged = false;
    summary.houseChanged = false;
    summary.importantEvents.push_back(reason + ": rolled " + std::to_string(rolledValue) +
                                      (movementValue != rolledValue ? " and moved " + std::to_string(movementValue) : ""));
    summary.importantEvents.push_back("Moved from Space " + std::to_string(startTile) +
                                      " to Space " + std::to_string(endTile) + ".");
    summary.importantEvents.push_back("Landed on " + getTileDisplayName(end) + ".");
    summary.importantEvents.push_back(describeTileEffectText(end));
    showTurnSummaryReport(summary, hasColor);
    if (titleWin) touchwin(titleWin);
    if (boardWin) touchwin(boardWin);
    if (infoWin) touchwin(infoWin);
    if (msgWin) touchwin(msgWin);
}

int Game::rollSpinner(const std::string& title, const std::string& detail) {
    int msgH = 0;
    int msgW = 0;
    getmaxyx(msgWin, msgH, msgW);
    (void)msgH;
    const int contentW = std::max(1, msgW - 4);
    werase(msgWin);
    drawBoxSafe(msgWin);
    mvwprintw(msgWin, 1, 2, "%s", clipUiText(title, static_cast<std::size_t>(contentW)).c_str());
    mvwprintw(msgWin, 2, 2, "%s", clipUiText(detail, static_cast<std::size_t>(contentW)).c_str());
    wrefresh(msgWin);

    if (autoAdvanceUi) {
        int value = 1;
        for (int flash = 0; flash < 6; ++flash) {
            value = rng.roll10();
            werase(msgWin);
            drawBoxSafe(msgWin);
            mvwprintw(msgWin, 1, 2, "%s", clipUiText(title, static_cast<std::size_t>(contentW)).c_str());
            mvwprintw(msgWin, 2, 2, "CPU rolling: %d", value);
            wrefresh(msgWin);
            napms(90);
        }
        flashSpinResult(title, value);
        showRollResultPopup(value);
        addHistory("CPU auto-spin result: " + std::to_string(value));
        return value;
    }

    int ch;
    do {
        ch = wgetch(msgWin);
    } while (ch != ' ' && ch != '\n' && ch != '\r' && ch != KEY_ENTER);

    nodelay(msgWin, TRUE);
    auto lastSpace = std::chrono::steady_clock::now();
    int value = 1;
    while (true) {
        value = rng.roll10();
        werase(msgWin);
        drawBoxSafe(msgWin);
        mvwprintw(msgWin, 1, 2, "%s", clipUiText(title, static_cast<std::size_t>(contentW)).c_str());
        mvwprintw(msgWin, 2, 2, "%s",
                  clipUiText("Rolling: " + std::to_string(value) + "  Release SPACE to stop",
                             static_cast<std::size_t>(contentW)).c_str());
        wrefresh(msgWin);
        napms(80);

        ch = wgetch(msgWin);
        if (ch == ' ') {
            lastSpace = std::chrono::steady_clock::now();
        } else if (ch != ERR) {
            break;
        }
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - lastSpace).count();
        if (elapsed > 240) break;
    }
    nodelay(msgWin, FALSE);

    flashSpinResult(title, value);
    showRollResultPopup(value);
    werase(msgWin);
    drawBoxSafe(msgWin);
    const std::string settle = "The wheel settles on " + std::to_string(value) + ". Press ENTER to continue the story.";
    const std::string clippedTitle = clipUiText(title, static_cast<std::size_t>(contentW));
    const std::string clippedSettle = clipUiText(settle, static_cast<std::size_t>(contentW));
    mvwprintw(msgWin, 1, std::max(2, (msgW - static_cast<int>(clippedTitle.size())) / 2), "%s", clippedTitle.c_str());
    mvwprintw(msgWin, 2, std::max(2, (msgW - static_cast<int>(clippedSettle.size())) / 2), "%s", clippedSettle.c_str());
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
    for (std::size_t i = 0; i < lines.size(); ++i) {
        values.push_back(static_cast<int>(i));
    }
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

int Game::chooseRandomOpponentIndex(int currentPlayer) {
    std::vector<int> candidates;
    for (size_t i = 0; i < players.size(); ++i) {
        if (static_cast<int>(i) != currentPlayer && !players[i].retired) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) {
        return -1;
    }
    return candidates[static_cast<std::size_t>(rng.uniformInt(0, static_cast<int>(candidates.size()) - 1))];
}

int Game::simulateDuelMinigameScore(const Player& player) {
    if (player.type != PlayerType::CPU) {
        return rng.uniformInt(40, 85);
    }

    switch (player.cpuDifficulty) {
        case CpuDifficulty::Easy:
            return rng.uniformInt(20, 55);
        case CpuDifficulty::Hard:
            return rng.uniformInt(65, 95);
        case CpuDifficulty::Normal:
        default:
            return rng.uniformInt(45, 75);
    }
}

int Game::playDuelMinigameScore(int playerIndex, int minigameChoice) {
    Player& player = players[static_cast<std::size_t>(playerIndex)];
    static const char* names[] = {
        "Pong",
        "Battleship",
        "Hangman",
        "Memory Match",
        "Minesweeper"
    };

    if (isCpuPlayer(playerIndex)) {
        const int score = simulateDuelMinigameScore(player);
        showPopupMessage(player.name + " CPU Minigame Result",
                         cpuDifficultyLabel(player.cpuDifficulty) + " CPU scored " +
                         std::to_string(score) + "/100 in " + names[minigameChoice] + ".",
                         hasColor,
                         autoAdvanceUi);
        return score;
    }

    int score = 0;
    if (minigameChoice == 0) {
        const PongMinigameResult result = playPongMinigame(player.name, hasColor);
        score = result.abandoned ? 0 : std::min(100, result.playerScore * 10);
    } else if (minigameChoice == 1) {
        const BattleshipMinigameResult result = playBattleshipMinigame(player.name, hasColor);
        score = result.abandoned ? 0 : std::min(100, result.shipsDestroyed * 10);
    } else if (minigameChoice == 2) {
        const HangmanResult result = playHangmanMinigame(player.name, hasColor);
        score = result.abandoned ? 0 : std::min(100, result.lettersGuessed * 8);
    } else if (minigameChoice == 3) {
        const MemoryMatchResult result = playMemoryMatchMinigame(player.name, hasColor);
        score = result.abandoned ? 0 : std::min(100, result.pairsMatched * 12 + (result.won ? 4 : 0));
    } else {
        const MinesweeperResult result = playMinesweeperMinigame(player.name, hasColor);
        score = result.abandoned ? 0 : std::min(100, result.safeTilesRevealed * 6);
    }

    if (titleWin) touchwin(titleWin);
    if (boardWin) touchwin(boardWin);
    if (infoWin) touchwin(infoWin);
    if (msgWin) touchwin(msgWin);

    showInfoPopup(player.name + " duel score",
                  std::string(names[minigameChoice]) + " score: " + std::to_string(score) + "/100.");
    return score;
}

std::string Game::resolveDuelMinigameAction(int playerIndex,
                                            int& amountDelta,
                                            int forcedOpponentIndex,
                                            int pot) {
    Player& player = players[static_cast<std::size_t>(playerIndex)];
    const int opponentIndex = forcedOpponentIndex >= 0
        ? forcedOpponentIndex
        : chooseRandomOpponentIndex(playerIndex);
    if (opponentIndex < 0) {
        amountDelta = 0;
        return "No valid opponent is available.";
    }

    Player& opponent = players[static_cast<std::size_t>(opponentIndex)];
    const int minigameChoice = rng.uniformInt(0, 4);
    static const char* names[] = {
        "Pong",
        "Battleship",
        "Hangman",
        "Memory Match",
        "Minesweeper"
    };

    std::vector<std::string> lines;
    lines.push_back(player.name + " used a Duel Minigame Card!");
    lines.push_back("Random opponent draw begins...");
    lines.push_back("Opponent selected: " + opponent.name);
    lines.push_back(player.name + " will play against " + opponent.name + ".");
    if (opponent.type == PlayerType::CPU) {
        lines.push_back("Opponent CPU difficulty: " + cpuDifficultyLabel(opponent.cpuDifficulty));
    } else {
        lines.push_back("Opponent is human and will play normally.");
    }
    lines.push_back("Minigame selected: " + std::string(names[minigameChoice]));
    showPopupMessage("Duel Minigame Card", lines, hasColor, autoAdvanceUi);

    int playerScore = 0;
    int opponentScore = 0;
    if (minigameChoice == 0) {
        const PongDuelResult duelResult = playPongDuelMinigame(player.name,
                                                               opponent.name,
                                                               hasColor,
                                                               isCpuPlayer(opponentIndex));
        if (duelResult.abandoned) {
            amountDelta = 0;
            return "The duel ended early with no payout.";
        }
        playerScore = duelResult.winnerSide == 0 ? 100 : 0;
        opponentScore = duelResult.winnerSide == 1 ? 100 : 0;
    } else {
        playerScore = playDuelMinigameScore(playerIndex, minigameChoice);
        opponentScore = playDuelMinigameScore(opponentIndex, minigameChoice);
    }

    std::ostringstream result;
    result << names[minigameChoice] << " duel score "
           << player.name << " " << playerScore
           << " - " << opponent.name << " " << opponentScore << ". ";

    if (playerScore >= opponentScore) {
        PaymentResult payment = bank.charge(opponent, pot);
        bank.credit(player, pot);
        amountDelta = pot;
        result << player.name << " wins $" << pot << ".";
        if (payment.loansTaken > 0) {
            result << " " << opponent.name << " automatic loans: " << payment.loansTaken << ".";
        }
    } else {
        PaymentResult payment = bank.charge(player, pot);
        bank.credit(opponent, pot);
        amountDelta = -pot;
        result << opponent.name << " wins $" << pot << ".";
        if (payment.loansTaken > 0) {
            result << " Automatic loans: " << payment.loansTaken << ".";
        }
    }

    addHistory(player.name + " dueled " + opponent.name + " in " + names[minigameChoice]);
    addHistory(result.str());
    showPopupMessage("Duel Result", result.str(), hasColor, autoAdvanceUi);
    return result.str();
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
    if (tileId == 79) {
        return player.familyChoice == 0 ? 68 : 78;
    }
    if (tileId == 86) {
        return player.riskChoice == 0 ? 82 : 85;
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
        napms(200);

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
        case ACTION_DUEL_MINIGAME:
            return resolveDuelMinigameAction(playerIndex, amountDelta);
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

    WINDOW* popup = createCenteredWindow(12, 64, 12, 44);
    if (!popup) {
        showTerminalSizeWarning(12, 44, hasColor);
        return amount;
    }
    int popupH = 0;
    int popupW = 0;
    getmaxyx(popup, popupH, popupW);
    const int contentW = std::max(1, popupW - 4);
    werase(popup);
    drawBoxSafe(popup);
    mvwprintw(popup, 1, 2, "ACTION CARD");
    mvwprintw(popup, 2, 2, "%s", clipUiText(card.title, static_cast<std::size_t>(contentW)).c_str());
    mvwprintw(popup, 4, 2, "%s", clipUiText(card.description, static_cast<std::size_t>(contentW)).c_str());
    if (rollValue > 0) {
        mvwprintw(popup, 5, 2, "Rolled: %d", rollValue);
    }
    if (!branchText.empty()) {
        mvwprintw(popup, 6, 2, "%s",
                  clipUiText("Branch: " + branchText, static_cast<std::size_t>(contentW)).c_str());
    }
    if (amount > 0) {
        blinkIndicator(popup, 7, 2, clipUiText("Result: " + result, static_cast<std::size_t>(contentW)),
                       hasColor, GOLDRUSH_BLACK_FOREST, 2, 2000, contentW);
    } else if (amount < 0) {
        blinkIndicator(popup, 7, 2, clipUiText("Result: " + result, static_cast<std::size_t>(contentW)),
                       hasColor, GOLDRUSH_GOLD_TERRA, 2, 2000, contentW);
    } else {
        mvwprintw(popup, 7, 2, "%s",
                  clipUiText("Result: " + result, static_cast<std::size_t>(contentW)).c_str());
    }
    mvwprintw(popup, 8, 2, "%s",
              clipUiText(card.keepAfterUse ? "Kept for endgame scoring." : "Discarded after use.",
                         static_cast<std::size_t>(contentW)).c_str());
    mvwprintw(popup, popupH - 3, 2, "%s",
              clipUiText("Cash now: $" + std::to_string(player.cash) +
                             "  Loans: " + std::to_string(player.loans),
                         static_cast<std::size_t>(contentW)).c_str());
    mvwprintw(popup, popupH - 2, 2, "Press ENTER");
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
        lines.push_back("- " + choices[i].title + ": " +
                        careerDescriptionText(choices[i]) +
                        " Salary $" + std::to_string(choices[i].salary) +
                        (choices[i].requiresDegree ? ". Degree required." : ". No degree required."));
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
                  careerDescriptionText(choices[static_cast<std::size_t>(choice)]) +
                  " Salary: $" + std::to_string(player.salary) +
                  (choices[static_cast<std::size_t>(choice)].requiresDegree
                       ? ". Degree required."
                       : ". No degree required."));
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
    player.retirementHome = choice == 0 ? "Millionaire Mansion (MM)" : "Countryside Acres (CA)";
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
    addHistory(player.name + " invested on spinner " + std::to_string(card.number));
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
                line = "College begins. Tuition was already paid when this route was chosen.";
                addHistory(player.name + " entered college");
            } else {
                PaymentResult payment = bank.charge(player, 100000);
                line = appendLoanText("College opens a late door, but tuition costs $100000.", payment);
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
            line = "Graduation day passes by. Career-route players keep building the job they already chose.";
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
            line = "A promotion lands at the right time. Salary rises to $" + std::to_string(player.salary) + ".";
            addHistory(player.name + " got a promotion");
            break;
        case TILE_PAYDAY: {
            int payout = tile.value + effectiveSalary(player);
            bank.credit(player, payout);
            line = "Payday arrives. " + player.name + " collects $" + std::to_string(payout) + ".";
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

    showInfoPopup(getTileDisplayName(tile) + " resolved", line);
}

int Game::chooseNextTile(Player& player, const Tile& tile) {
    if ((tile.kind == TILE_SPLIT_START || tile.kind == TILE_START) && player.startChoice == -1) {
        const int playerIndex = findPlayerIndex(player);
        int c = 0;
        if (isCpuPlayer(playerIndex)) {
            c = cpu.chooseStartRoute(player, rules);
            showCpuThinking(playerIndex,
                            c == 0
                                ? "CPU weighs early debt against stronger future jobs, then chooses College."
                                : "CPU chooses Career to start earning sooner.");
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
            showDecisionPopup(player.name,
                              "College Route",
                              appendLoanText("A bigger future starts with a bigger bill. Tuition is paid before moving onto College Route (CO).", payment),
                              hasColor,
                              autoAdvanceUi);
        } else {
            addHistory(player.name + " chose Career");
            showDecisionPopup(player.name,
                              "Career Path",
                              "The faster road begins now: choose a job immediately and start earning salary sooner.",
                              hasColor,
                              autoAdvanceUi);
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

    if (tile.kind == TILE_SPLIT_START || tile.kind == TILE_START) {
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
        std::string message = player.name + " moved to " + getTileDisplayName(landed);
        if (board.isStopSpace(landed)) {
            message = "STOP! " + player.name + " hit " + getTileDisplayName(landed);
        }
        renderGame(currentPlayer, message, "Movement in progress...");
        napms(200);
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
    const int startTile = player.tile;
    const int startingCash = player.cash;
    const int startingLoans = player.loans;
    const Tile& start = board.tileAt(startTile);
    std::string typeLine = playerTypeLabel(player.type);
    if (player.type == PlayerType::CPU) {
        typeLine += " " + cpuDifficultyLabel(player.cpuDifficulty);
    }
    showInfoPopup(player.name + "'s Turn - " + typeLine,
                  "Money $" + std::to_string(player.cash) + " | Space " +
                  std::to_string(startTile) + " - " + getTileDisplayName(start));

    int roll = rollSpinner(reason, "Hold SPACE to spin movement");
    const int originalRoll = roll;
    addHistory(player.name + " spun " + std::to_string(roll));
    maybeAwardSpinToWin(player, roll);
    resolveInvestmentPayouts(roll);

    if (player.movementPenaltyTurns > 0 && player.movementPenaltyPercent > 0) {
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
        showTurnSummaryPopup(currentPlayer,
                             originalRoll,
                             roll,
                             startTile,
                             player.tile,
                             startingCash,
                             startingLoans,
                             reason);
        return;
    }

    bool stoppedEarly = animateMove(currentPlayer, roll);
    if (!stoppedEarly) {
        applyTileEffect(currentPlayer, board.tileAt(player.tile));
    }
    showTurnSummaryPopup(currentPlayer,
                         originalRoll,
                         roll,
                         startTile,
                         player.tile,
                         startingCash,
                         startingLoans,
                         reason);
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
                ++players[currentPlayerIndex].turnsTaken;
                ++turnCounter;
                currentPlayerIndex = (currentPlayerIndex + 1) % static_cast<int>(players.size());
                continue;
            }

            renderGame(currentPlayerIndex,
                       players[currentPlayerIndex].name + "'s turn",
                       turnPromptText());
            maybeShowSabotageUnlock(currentPlayerIndex);
            maybeCpuSabotage(currentPlayerIndex);
            int command = waitForTurnCommand(currentPlayerIndex);
            if (command == 27) {
                const int quitChoice = showBranchPopup(
                    "Leave the game from the ESC menu?",
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
                               turnPromptText());
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
                           turnPromptText());
                if (!sabotageUsed) {
                    continue;
                }
            }

            autoAdvanceUi = isCpuPlayer(currentPlayerIndex);
            takeMovementSpin(currentPlayerIndex, "Movement Spin");
            autoAdvanceUi = false;
            decrementTurnStatuses(players[currentPlayerIndex]);
            ++players[currentPlayerIndex].turnsTaken;
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

        const int winnerHouse = players[winner].finalHouseSaleValue > 0
            ? players[winner].finalHouseSaleValue
            : players[winner].houseValue;
        const int winnerActionScore = static_cast<int>(players[winner].actionCards.size()) * 100000;
        const int winnerPetScore = static_cast<int>(players[winner].petCards.size()) * 100000;
        const int winnerFamilyScore = players[winner].kids * 50000;
        const int winnerLoanPenalty = bank.totalLoanDebt(players[winner]);

        std::vector<std::string> endgameLines;
        endgameLines.push_back(players[winner].name + " reaches the finish with the highest final score: $" +
                               std::to_string(best) + ".");
        endgameLines.push_back("");
        endgameLines.push_back("FINAL SCORES");
        for (std::size_t i = 0; i < players.size(); ++i) {
            endgameLines.push_back(players[i].name + ": $" + std::to_string(calculateFinalWorth(players[i])));
        }
        endgameLines.push_back("");
        endgameLines.push_back("WINNER BREAKDOWN");
        endgameLines.push_back("Cash: $" + std::to_string(players[winner].cash));
        endgameLines.push_back("House sale: $" + std::to_string(winnerHouse));
        endgameLines.push_back("Action cards: $" + std::to_string(winnerActionScore));
        endgameLines.push_back("Pet cards: $" + std::to_string(winnerPetScore));
        endgameLines.push_back("Children/family bonus: $" + std::to_string(winnerFamilyScore));
        endgameLines.push_back("Retirement bonus: $" + std::to_string(players[winner].retirementBonus));
        endgameLines.push_back("Loan penalty: -$" + std::to_string(winnerLoanPenalty));
        endgameLines.push_back("Press ENTER to return to the main menu.");

        addHistory("Completed game: " + players[winner].name + " won with $" + std::to_string(best));
        showPopupMessage("Player " + std::to_string(winner + 1) + " Wins!",
                         endgameLines,
                         hasColor,
                         false);

        destroyWindows();
        players.clear();
        currentPlayerIndex = 0;
        retiredCount = 0;
        turnCounter = 0;
        activeTraps.clear();
        history.clear();
        clear();
        refresh();
    }
}
