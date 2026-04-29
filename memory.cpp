#include "memory.hpp"
#include "input_helpers.h"
#include "minigame_tutorials.h"
#include "timer_display.h"
#include "ui.h"
#include "ui_helpers.h"

#include <ncurses.h>
#include <cstring>
#include <algorithm>
#include <random>
#include <ctime>
#include <vector>

//Input: unicodeSupported (bool, unused here)
//Output: vector of symbols (A–H)
//Purpose: provides the set of symbols used for matching pairs
//Relation: used in shuffleGrid to populate the grid
std::vector<std::string> getMemoryMatchSymbols(bool unicodeSupported) {
    (void)unicodeSupported;
    return {"A", "B", "C", "D", "E", "F", "G", "H"};
}

namespace {

const int GRID_SIZE = 4;
const int TOTAL_PAIRS = 8;
const int TOTAL_CELLS = GRID_SIZE * GRID_SIZE;
const int STARTING_LIVES = 20;
const int MAX_HELP_USES = 5;
const int HELP_REVEAL_MS = 1000;
const int MATCH_REVEAL_MS = 800;
const int CELL_WIDTH = 7;
const int CELL_HEIGHT = 3;
const int TOTAL_GRID_WIDTH = GRID_SIZE * CELL_WIDTH;

[[maybe_unused]] const std::vector<std::string> SYMBOLS = {
    "A", "B", "C", "D", "E", "F", "G", "H"
};

const std::vector<std::string> MEMORY_TITLE = {
    " _      _____ _      ____  ____ ___  _ _____ ____  _      _____",
    " / \\__/|/  __// \\__/|/  _ \\/  __\\\\  \\///  __//  _ \\/ \\__/|/  __/",
    "| |\\/|||  \\  | |\\/||| / \\||  \\/| \\  / | |  _| / \\|| |\\/|||  \\  ",
    "| |  |||  /_ | |  ||| \\_/||    / / /  | |_//| |-||| |  |||  /_ ",
    "\\_/  \\|\\____\\\\_/  \\|\\____/\\_/\\_\\/_/   \\____\\\\_/ \\|\\_/  \\|\\____\\",
    "                                                               "
};

//Input: none (data container)
//Output: holds cell attributes (symbol, revealed state, matched state)
//Purpose: represents a single card in the memory grid
//Relation: used throughout grid setup, drawing, and matching logic
struct Cell {
    std::string symbol;
    bool revealed;
    bool matched;
};

//Input: grid reference (vector<Cell>&)
//Output: none
//Purpose: fills the grid with pairs of symbols, shuffles them randomly
//Relation: called at the start of playMemoryMatchMinigame
void shuffleGrid(std::vector<Cell>& grid) {
    const std::vector<std::string> symbolPool = getMemoryMatchSymbols(false);
    std::vector<std::string> symbols;
    for (int i = 0; i < TOTAL_PAIRS; ++i) {
        symbols.push_back(symbolPool[static_cast<std::size_t>(i)]);
        symbols.push_back(symbolPool[static_cast<std::size_t>(i)]);
    }
    
    std::shuffle(symbols.begin(), symbols.end(), std::mt19937(std::random_device()()));
    
    for (int i = 0; i < TOTAL_CELLS; ++i) {
        grid[i].symbol = symbols[i];
        grid[i].revealed = false;
        grid[i].matched = false;
    }
}

//Input: WINDOW pointer, coordinates, symbol string, flags (revealed, matched, selected, revealAll)
//Output: none
//Purpose: draws a single cell with its symbol, hidden state, or match indicator
//Relation: used in rendering loop of playMemoryMatchMinigame
void drawCell(WINDOW* win, int y, int x, const std::string& symbol, bool isRevealed, bool isMatched,
              bool isSelected, bool revealAll) {
    if (isSelected) {
        wattron(win, A_REVERSE);
    }

    drawBoxAtSafe(win, y, x, CELL_HEIGHT, CELL_WIDTH);

    for (int innerX = 1; innerX < CELL_WIDTH - 1; ++innerX) {
        mvwaddch(win, y + 1, x + innerX, ' ');
    }

    if (isMatched || revealAll) {
        wattron(win, A_BOLD);
        mvwprintw(win, y + 1, x + 2, "%s", symbol.c_str());
        wattroff(win, A_BOLD);
    } else if (isRevealed) {
        mvwprintw(win, y + 1, x + 2, "%s", symbol.c_str());
    } else {
        mvwprintw(win, y + 1, x + 3, "?");
    }

    if (isSelected) {
        wattroff(win, A_REVERSE);
    }
}

//Input: WINDOW pointer, screen width, hasColor flag
//Output: none
//Purpose: draws ASCII art title for Memory Match
//Relation: called at the start of playMemoryMatchMinigame for UI
void drawAsciiTitle(WINDOW* win, int screenW, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    for (std::size_t i = 0; i < MEMORY_TITLE.size(); ++i) {
        mvwprintw(win, static_cast<int>(i), (screenW - static_cast<int>(MEMORY_TITLE[i].size())) / 2,
                  "%s", MEMORY_TITLE[i].c_str());
    }
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
}

//Input: WINDOW pointer, coordinates, text string, positive flag, hasColor flag
//Output: none
//Purpose: displays feedback messages (MATCH! or NO MATCH!) with blinking effect
//Relation: used in playMemoryMatchMinigame after each pair attempt
void flashFeedback(WINDOW* win,
                   int y,
                   int screenW,
                   const std::string& text,
                   bool positive,
                   bool hasColor) {
    const int colorPair = positive ? GOLDRUSH_BLACK_FOREST : GOLDRUSH_GOLD_TERRA;
    const int textX = (screenW - static_cast<int>(text.size())) / 2;

    blinkIndicator(win, y, textX, text, hasColor, colorPair, 1, 220, static_cast<int>(text.size()));
}

} // anonymous namespace

//Input: playerName(string), hasColor(bool)
//Output: MemoryMatchResult struct (pairsMatched, livesRemaining, abandoned, won)
//Purpose: runs the full Memory Match minigame loop, shows tutorial, initializes grid and shuffles symbols, handles input (movement, selection, help, quit), updates game state (pairs matched, lives remaining), renders UI (grid, feedback, status, instructions), ends with victory, game over, or abandonment
//Relation: main entry point for the minigame, integrates all helper functions
MemoryMatchResult playMemoryMatchMinigame(const std::string& playerName, bool hasColor) {
    MemoryMatchResult result;
    result.pairsMatched = 0;
    result.livesRemaining = STARTING_LIVES;
    result.abandoned = false;
    result.won = false;

    showMinigameTutorial("Memory Match",
                         "Memorize the grid, then find all matching pairs.",
                         "WASD or arrows move. Enter/Space selects. H reveals help. ESC exits.",
                         "Match all 8 pairs before running out of lives.",
                         "Each pair pays $100. Clearing the board adds a $200 bonus.",
                         hasColor);

    if (!terminalIsAtLeast(31, 74)) {
        showTerminalSizeWarning(31, 74, hasColor);
        result.abandoned = true;
        return result;
    }
    
    WINDOW* overlay = newwin(0, 0, 0, 0);
    if (!overlay) {
        showTerminalSizeWarning(31, 74, hasColor);
        result.abandoned = true;
        return result;
    }
    keypad(overlay, TRUE);

    std::vector<Cell> grid(TOTAL_CELLS);
    shuffleGrid(grid);
    
    int currentRow = 0;
    int currentCol = 0;
    
    int helpUses = 0;
    bool waitingForSecondMatch = false;
    int firstMatchIdx = -1;
    bool memorizationPhase = true;
    auto memorizationStart = std::time(nullptr);
    
    while (result.livesRemaining > 0 && result.pairsMatched < TOTAL_PAIRS) {
        // Get current terminal size
        int screenH, screenW;
        getmaxyx(stdscr, screenH, screenW);
        
        // Center the grid
        int gridStartY = (screenH - (GRID_SIZE * CELL_HEIGHT)) / 2;
        if (gridStartY < 4) gridStartY = 4;
        int gridStartX = (screenW - TOTAL_GRID_WIDTH) / 2;
        if (gridStartX < 2) gridStartX = 2;
        
        // Resize overlay to fill screen
        wresize(overlay, screenH, screenW);
        mvwin(overlay, 0, 0);
        
        werase(overlay);
        
        if (hasColor) {
            wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        }

        const int arenaWidth = 70;
        const int arenaHeight = 24;
        const int arenaLeft = (screenW - arenaWidth) / 2;
        const int arenaTop = 7;
        const int arenaBottom = arenaTop + arenaHeight - 1;
        
        drawAsciiTitle(overlay, screenW, hasColor);

        if (hasColor) {
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }
        drawBoxAtSafe(overlay, arenaTop, arenaLeft, arenaHeight, arenaWidth);
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }

        char status[100];
        snprintf(status, sizeof(status), "Player: %s  |  Pairs: %d/8  |  Lives: %d",
                playerName.c_str(), result.pairsMatched, result.livesRemaining);
        int statusLen = strlen(status);
        mvwprintw(overlay, 6, (screenW - statusLen) / 2, "%s", status);

        char helpText[100];
        snprintf(helpText, sizeof(helpText), "Help uses: %d/5 (press H)", MAX_HELP_USES - helpUses);
        int helpLen = strlen(helpText);
        mvwprintw(overlay, arenaTop + 1, (screenW - helpLen) / 2, "%s", helpText);

        const char* instructions = "W/A/S/D: Move  |  ENTER/Space: Select  |  H: Help  |  ESC: Quit";
        int instrLen = strlen(instructions);
        mvwprintw(overlay, arenaBottom - 2, (screenW - instrLen) / 2, "%s", instructions);
        mvwprintw(overlay, arenaBottom - 1, (screenW - 46) / 2,
                  "Match pairs for $100 each. Clear all for a $200 bonus.");

        gridStartY = arenaTop + 5;
        gridStartX = arenaLeft + (arenaWidth - TOTAL_GRID_WIDTH) / 2;
        
        if (memorizationPhase) {
            for (int row = 0; row < GRID_SIZE; ++row) {
                for (int col = 0; col < GRID_SIZE; ++col) {
                    int idx = row * GRID_SIZE + col;
                    int x = gridStartX + col * CELL_WIDTH;
                    int y = gridStartY + row * CELL_HEIGHT;
                    drawCell(overlay, y, x, grid[idx].symbol, true, false, false, true);
                }
            }
            
            const int remaining = std::max(0, 5 - static_cast<int>(std::time(nullptr) - memorizationStart));
            const std::string timer = countdownTimerText(remaining);
            mvwprintw(overlay, arenaBottom - 4, (screenW - 30) / 2,
                      "Memorize the positions!");
            drawCountdownTimer(overlay,
                               arenaBottom - 3,
                               (screenW - static_cast<int>(timer.size())) / 2,
                               remaining,
                               hasColor);
            wrefresh(overlay);
            
            if (std::time(nullptr) - memorizationStart >= 5) {
                memorizationPhase = false;
                for (int i = 0; i < TOTAL_CELLS; ++i) {
                    if (!grid[i].matched) {
                        grid[i].revealed = false;
                    }
                }
            }
            napms(50);
            continue;
        }
        
        for (int row = 0; row < GRID_SIZE; ++row) {
            for (int col = 0; col < GRID_SIZE; ++col) {
                int idx = row * GRID_SIZE + col;
                int x = gridStartX + col * CELL_WIDTH;
                int y = gridStartY + row * CELL_HEIGHT;
                bool isSelected = (row == currentRow && col == currentCol);
                drawCell(overlay, y, x, grid[idx].symbol, grid[idx].revealed, 
                        grid[idx].matched, isSelected, false);
            }
        }
        
        wrefresh(overlay);
        
        int ch = wgetch(overlay);
        
        const InputAction action = getInputAction(ch, ControlScheme::SinglePlayer);
        if (action == InputAction::Cancel) {
            result.abandoned = true;
            break;
        }
        
        if (ch == 'h' || ch == 'H') {
            if (helpUses < MAX_HELP_USES) {
                helpUses++;
                for (int row = 0; row < GRID_SIZE; ++row) {
                    for (int col = 0; col < GRID_SIZE; ++col) {
                        int idx = row * GRID_SIZE + col;
                        int x = gridStartX + col * CELL_WIDTH;
                        int y = gridStartY + row * CELL_HEIGHT;
                        drawCell(overlay, y, x, grid[idx].symbol, true, false, false, true);
                    }
                }
                wrefresh(overlay);
                napms(HELP_REVEAL_MS);
                continue;
            }
        }
        
        if (action == InputAction::Up) {
            currentRow = (currentRow - 1 + GRID_SIZE) % GRID_SIZE;
        } else if (action == InputAction::Down) {
            currentRow = (currentRow + 1) % GRID_SIZE;
        } else if (action == InputAction::Left) {
            currentCol = (currentCol - 1 + GRID_SIZE) % GRID_SIZE;
        } else if (action == InputAction::Right) {
            currentCol = (currentCol + 1) % GRID_SIZE;
        }
        else if (action == InputAction::Confirm || action == InputAction::Fire) {
            int cellIdx = currentRow * GRID_SIZE + currentCol;
            
            if (!grid[cellIdx].matched && !grid[cellIdx].revealed) {
                if (!waitingForSecondMatch) {
                    firstMatchIdx = cellIdx;
                    grid[cellIdx].revealed = true;
                    waitingForSecondMatch = true;
                } else if (cellIdx != firstMatchIdx) {
                    grid[cellIdx].revealed = true;
                    wrefresh(overlay);
                    napms(MATCH_REVEAL_MS);
                    
                    if (grid[firstMatchIdx].symbol == grid[cellIdx].symbol) {
                        grid[firstMatchIdx].matched = true;
                        grid[cellIdx].matched = true;
                        result.pairsMatched++;
                        flashFeedback(overlay,
                                      arenaBottom - 4,
                                      screenW,
                                      "MATCH! +$100",
                                      true,
                                      hasColor);
                    } else {
                        result.livesRemaining--;
                        flashFeedback(overlay,
                                      arenaBottom - 4,
                                      screenW,
                                      "NO MATCH! -1 life",
                                      false,
                                      hasColor);
                        grid[firstMatchIdx].revealed = false;
                        grid[cellIdx].revealed = false;
                    }
                    waitingForSecondMatch = false;
                    firstMatchIdx = -1;
                }
            }
        }
    }
    
    result.won = (result.pairsMatched == TOTAL_PAIRS);
    
    int screenH, screenW;
    getmaxyx(stdscr, screenH, screenW);
    const int arenaWidth = 70;
    const int arenaHeight = 24;
    const int arenaLeft = (screenW - arenaWidth) / 2;
    const int arenaTop = 2;
    
    werase(overlay);
    if (hasColor) {
        wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
    }
    drawBoxAtSafe(overlay, arenaTop, arenaLeft, arenaHeight, arenaWidth);
    if (hasColor) {
        wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
    }
    
    if (result.won) {
        if (hasColor) wattron(overlay, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
        mvwprintw(overlay, screenH/2 - 1, (screenW - 50) / 2, "You matched all %d pairs!", TOTAL_PAIRS);
        mvwprintw(overlay, screenH/2, (screenW - 46) / 2, "Lives remaining: %d  |  Earned $1000", result.livesRemaining);
        blinkIndicator(overlay,
                       screenH / 2 - 2,
                       (screenW - 8) / 2,
                       "VICTORY!",
                       hasColor,
                       GOLDRUSH_BLACK_FOREST,
                       2,
                       2000,
                       8);
        if (hasColor) wattroff(overlay, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
    } else if (result.abandoned) {
        mvwprintw(overlay, screenH/2 - 1, (screenW - 30) / 2, "Game abandoned.");
    } else {
        if (hasColor) wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
        mvwprintw(overlay, screenH/2 - 1, (screenW - 45) / 2, "You ran out of lives!");
        mvwprintw(overlay, screenH/2, (screenW - 46) / 2, "Pairs matched: %d/8  |  Earned $%d", result.pairsMatched, result.pairsMatched * 100);
        blinkIndicator(overlay,
                       screenH / 2 - 2,
                       (screenW - 10) / 2,
                       "GAME OVER!",
                       hasColor,
                       GOLDRUSH_GOLD_TERRA,
                       2,
                       2000,
                       10);
        if (hasColor) wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
    
    mvwprintw(overlay, screenH/2 + 2, (screenW - 30) / 2, "Press ENTER to continue.");
    wrefresh(overlay);
    
    int ch;
    do {
        ch = wgetch(overlay);
    } while (ch != '\n' && ch != '\r' && ch != KEY_ENTER);
    
    delwin(overlay);
    touchwin(stdscr);
    refresh();
    
    return result;
}
