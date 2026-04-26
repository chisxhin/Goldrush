#include "memory.hpp"
#include "ui.h"

#include <ncurses.h>
#include <cstring>
#include <algorithm>
#include <random>
#include <ctime>
#include <vector>

namespace {

const int GRID_SIZE = 4;
const int TOTAL_PAIRS = 8;
const int TOTAL_CELLS = GRID_SIZE * GRID_SIZE;
const int STARTING_LIVES = 20;
const int MAX_HELP_USES = 5;
const int HELP_REVEAL_SECONDS = 2;
const int CELL_WIDTH = 5;
const int CELL_HEIGHT = 3;
const int TOTAL_GRID_WIDTH = GRID_SIZE * CELL_WIDTH;

const std::vector<char> SYMBOLS = {
    '~', '@', '#', '$', '%', '&', '+', '*'
};

const std::vector<std::string> MEMORY_TITLE = {
    " _      _____ _      ____  ____ ___  _ _____ ____  _      _____",
    "/ \\__/|/  __// \\__/|/  _ \\/  __\\\\  \\///  __//  _ \\/ \\__/|/  __/",
    "| |\\/|||  \\  | |\\/||| / \\||  \\/| \\  / | |  _| / \\|| |\\/|||  \\  ",
    "| |  |||  /_ | |  ||| \\_/||    / / /  | |_//| |-||| |  |||  /_ ",
    "\\_/  \\|\\____\\\\_/  \\|\\____/\\_/\\_\\/_/   \\____\\\\_/ \\|\\_/  \\|\\____\\",
    "                                                               "
};

struct Cell {
    char symbol;
    bool revealed;
    bool matched;
};

void shuffleGrid(std::vector<Cell>& grid) {
    std::vector<char> symbols;
    for (int i = 0; i < TOTAL_PAIRS; ++i) {
        symbols.push_back(SYMBOLS[i]);
        symbols.push_back(SYMBOLS[i]);
    }
    
    std::shuffle(symbols.begin(), symbols.end(), std::mt19937(std::random_device()()));
    
    for (int i = 0; i < TOTAL_CELLS; ++i) {
        grid[i].symbol = symbols[i];
        grid[i].revealed = false;
        grid[i].matched = false;
    }
}

void drawCell(WINDOW* win, int y, int x, char symbol, bool isRevealed, bool isMatched, 
              bool isSelected, bool revealAll) {
    if (isSelected) {
        wattron(win, A_REVERSE);
    }

    for (int i = 0; i < CELL_WIDTH; ++i) {
        mvwaddch(win, y, x + i, ACS_HLINE);
        mvwaddch(win, y + CELL_HEIGHT - 1, x + i, ACS_HLINE);
    }
    for (int i = 0; i < CELL_HEIGHT; ++i) {
        mvwaddch(win, y + i, x, ACS_VLINE);
        mvwaddch(win, y + i, x + CELL_WIDTH - 1, ACS_VLINE);
    }
    mvwaddch(win, y, x, ACS_ULCORNER);
    mvwaddch(win, y, x + CELL_WIDTH - 1, ACS_URCORNER);
    mvwaddch(win, y + CELL_HEIGHT - 1, x, ACS_LLCORNER);
    mvwaddch(win, y + CELL_HEIGHT - 1, x + CELL_WIDTH - 1, ACS_LRCORNER);

    if (isMatched || revealAll) {
        wattron(win, A_BOLD);
        mvwprintw(win, y + 1, x + 2, "%c", symbol);
        wattroff(win, A_BOLD);
    } else if (isRevealed) {
        mvwprintw(win, y + 1, x + 2, "%c", symbol);
    } else {
        mvwprintw(win, y + 1, x + 2, "?");
    }

    if (isSelected) {
        wattroff(win, A_REVERSE);
    }
}

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

void flashFeedback(WINDOW* win,
                   int y,
                   int screenW,
                   const std::string& text,
                   bool positive,
                   bool hasColor) {
    const int colorPair = positive ? GOLDRUSH_BLACK_FOREST : GOLDRUSH_GOLD_TERRA;
    const int textX = (screenW - static_cast<int>(text.size())) / 2;

    for (int flash = 0; flash < 4; ++flash) {
        mvwprintw(win, y, 2, "%-*s", screenW - 4, "");
        if (flash % 2 == 0) {
            if (hasColor) {
                wattron(win, COLOR_PAIR(colorPair) | A_BOLD | A_BLINK);
            } else {
                wattron(win, A_REVERSE | A_BOLD);
            }
            mvwprintw(win, y, textX, "%s", text.c_str());
            if (hasColor) {
                wattroff(win, COLOR_PAIR(colorPair) | A_BOLD | A_BLINK);
            } else {
                wattroff(win, A_REVERSE | A_BOLD);
            }
        }
        wrefresh(win);
        napms(120);
    }
}

} // anonymous namespace

MemoryMatchResult playMemoryMatchMinigame(const std::string& playerName, bool hasColor) {
    MemoryMatchResult result;
    result.pairsMatched = 0;
    result.livesRemaining = STARTING_LIVES;
    result.abandoned = false;
    result.won = false;
    
    WINDOW* overlay = newwin(0, 0, 0, 0);
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
        const int arenaRight = arenaLeft + arenaWidth - 1;
        const int arenaBottom = arenaTop + arenaHeight - 1;
        
        drawAsciiTitle(overlay, screenW, hasColor);

        if (hasColor) {
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }
        mvwhline(overlay, arenaTop, arenaLeft, ACS_HLINE, arenaWidth);
        mvwhline(overlay, arenaBottom, arenaLeft, ACS_HLINE, arenaWidth);
        mvwvline(overlay, arenaTop, arenaLeft, ACS_VLINE, arenaHeight);
        mvwvline(overlay, arenaTop, arenaRight, ACS_VLINE, arenaHeight);
        mvwaddch(overlay, arenaTop, arenaLeft, ACS_ULCORNER);
        mvwaddch(overlay, arenaTop, arenaRight, ACS_URCORNER);
        mvwaddch(overlay, arenaBottom, arenaLeft, ACS_LLCORNER);
        mvwaddch(overlay, arenaBottom, arenaRight, ACS_LRCORNER);
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

        const char* instructions = "Arrow Keys: Move  |  ENTER/Space: Select  |  H: Help  |  Q: Quit";
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
            
            mvwprintw(overlay, arenaBottom - 4, (screenW - 30) / 2, 
                      "Memorize the positions! 5 seconds...");
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
        
        if (ch == 'q' || ch == 'Q') {
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
                napms(HELP_REVEAL_SECONDS * 1000);
                continue;
            }
        }
        
        if (ch == KEY_UP) {
            currentRow = (currentRow - 1 + GRID_SIZE) % GRID_SIZE;
        } else if (ch == KEY_DOWN) {
            currentRow = (currentRow + 1) % GRID_SIZE;
        } else if (ch == KEY_LEFT) {
            currentCol = (currentCol - 1 + GRID_SIZE) % GRID_SIZE;
        } else if (ch == KEY_RIGHT) {
            currentCol = (currentCol + 1) % GRID_SIZE;
        }
        else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == ' ') {
            int cellIdx = currentRow * GRID_SIZE + currentCol;
            
            if (!grid[cellIdx].matched && !grid[cellIdx].revealed) {
                if (!waitingForSecondMatch) {
                    firstMatchIdx = cellIdx;
                    grid[cellIdx].revealed = true;
                    waitingForSecondMatch = true;
                } else if (cellIdx != firstMatchIdx) {
                    grid[cellIdx].revealed = true;
                    wrefresh(overlay);
                    napms(500);
                    
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
    const int arenaRight = arenaLeft + arenaWidth - 1;
    const int arenaBottom = arenaTop + arenaHeight - 1;
    
    werase(overlay);
    if (hasColor) {
        wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
    }
    mvwhline(overlay, arenaTop, arenaLeft, ACS_HLINE, arenaWidth);
    mvwhline(overlay, arenaBottom, arenaLeft, ACS_HLINE, arenaWidth);
    mvwvline(overlay, arenaTop, arenaLeft, ACS_VLINE, arenaHeight);
    mvwvline(overlay, arenaTop, arenaRight, ACS_VLINE, arenaHeight);
    mvwaddch(overlay, arenaTop, arenaLeft, ACS_ULCORNER);
    mvwaddch(overlay, arenaTop, arenaRight, ACS_URCORNER);
    mvwaddch(overlay, arenaBottom, arenaLeft, ACS_LLCORNER);
    mvwaddch(overlay, arenaBottom, arenaRight, ACS_LRCORNER);
    if (hasColor) {
        wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
    }
    
    if (result.won) {
        if (hasColor) wattron(overlay, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
        wattron(overlay, A_BLINK);
        mvwprintw(overlay, screenH/2 - 2, (screenW - 30) / 2, "VICTORY!");
        wattroff(overlay, A_BLINK);
        mvwprintw(overlay, screenH/2 - 1, (screenW - 50) / 2, "You matched all %d pairs!", TOTAL_PAIRS);
        mvwprintw(overlay, screenH/2, (screenW - 46) / 2, "Lives remaining: %d  |  Earned $1000", result.livesRemaining);
        if (hasColor) wattroff(overlay, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
    } else if (result.abandoned) {
        mvwprintw(overlay, screenH/2 - 1, (screenW - 30) / 2, "Game abandoned.");
    } else {
        if (hasColor) wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
        wattron(overlay, A_BLINK);
        mvwprintw(overlay, screenH/2 - 2, (screenW - 30) / 2, "GAME OVER!");
        wattroff(overlay, A_BLINK);
        mvwprintw(overlay, screenH/2 - 1, (screenW - 45) / 2, "You ran out of lives!");
        mvwprintw(overlay, screenH/2, (screenW - 46) / 2, "Pairs matched: %d/8  |  Earned $%d", result.pairsMatched, result.pairsMatched * 100);
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
