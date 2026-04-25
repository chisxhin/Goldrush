#include "minesweeper.hpp"

#include <ncurses.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "ui.h"

namespace {

const int GRID_ROWS = 5;
const int GRID_COLS = 5;
const int BOMB_COUNT = 10;
const int TOTAL_SAFE_TILES = (GRID_ROWS * GRID_COLS) - BOMB_COUNT;
const int ROUND_SECONDS = 60;
const int CELL_WIDTH = 5;
const int CELL_HEIGHT = 3;
const int GRID_WIDTH = GRID_COLS * CELL_WIDTH;
const std::vector<std::string> MINESWEEPER_TITLE = {
    " __  __ ___ _   _ _____ ____ __        _______ _____ ____  _____ ____  ",
    "|  \\/  |_ _| \\ | | ____/ ___|\\ \\      / / ____| ____|  _ \\| ____|  _ \\ ",
    "| |\\/| || ||  \\| |  _| \\___ \\ \\ \\ /\\ / /|  _| |  _| | |_) |  _| | |_) |",
    "| |  | || || |\\  | |___ ___) | \\ V  V / | |___| |___|  __/| |___|  _ < ",
    "|_|  |_|___|_| \\_|_____|____/   \\_/\\_/  |_____|_____|_|   |_____|_| \\_\\",
    "                                                                        ",
    "                                                                        "
};

struct Cell {
    bool hasBomb;
    bool revealed;
    int adjacentBombs;
};

int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void drawAsciiTitle(WINDOW* win, int screenW, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    for (std::size_t i = 0; i < MINESWEEPER_TITLE.size(); ++i) {
        mvwprintw(win, static_cast<int>(i),
                  (screenW - static_cast<int>(MINESWEEPER_TITLE[i].size())) / 2,
                  "%s", MINESWEEPER_TITLE[i].c_str());
    }
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
}

bool inBounds(int row, int col) {
    return row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS;
}

int indexFor(int row, int col) {
    return row * GRID_COLS + col;
}

void computeAdjacencies(std::vector<Cell>& grid) {
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            Cell& cell = grid[static_cast<std::size_t>(indexFor(row, col))];
            cell.adjacentBombs = 0;
            if (cell.hasBomb) {
                continue;
            }
            for (int dRow = -1; dRow <= 1; ++dRow) {
                for (int dCol = -1; dCol <= 1; ++dCol) {
                    if (dRow == 0 && dCol == 0) {
                        continue;
                    }
                    const int nRow = row + dRow;
                    const int nCol = col + dCol;
                    if (inBounds(nRow, nCol) &&
                        grid[static_cast<std::size_t>(indexFor(nRow, nCol))].hasBomb) {
                        ++cell.adjacentBombs;
                    }
                }
            }
        }
    }
}

std::vector<Cell> createGrid() {
    std::vector<Cell> grid(static_cast<std::size_t>(GRID_ROWS * GRID_COLS));
    for (std::size_t i = 0; i < grid.size(); ++i) {
        grid[i].hasBomb = false;
        grid[i].revealed = false;
        grid[i].adjacentBombs = 0;
    }

    std::vector<int> positions;
    positions.reserve(static_cast<std::size_t>(GRID_ROWS * GRID_COLS));
    for (int i = 0; i < GRID_ROWS * GRID_COLS; ++i) {
        positions.push_back(i);
    }
    std::shuffle(positions.begin(), positions.end(), std::mt19937(std::random_device()()));
    for (int i = 0; i < BOMB_COUNT; ++i) {
        grid[static_cast<std::size_t>(positions[static_cast<std::size_t>(i)])].hasBomb = true;
    }
    computeAdjacencies(grid);
    return grid;
}

void ensureFirstRevealSafeZone(std::vector<Cell>& grid, int centerRow, int centerCol) {
    std::vector<int> protectedIndices;
    for (int dRow = -1; dRow <= 1; ++dRow) {
        for (int dCol = -1; dCol <= 1; ++dCol) {
            const int row = centerRow + dRow;
            const int col = centerCol + dCol;
            if (inBounds(row, col)) {
                protectedIndices.push_back(indexFor(row, col));
            }
        }
    }

    for (std::size_t i = 0; i < protectedIndices.size(); ++i) {
        const int protectedIndex = protectedIndices[i];
        if (!grid[static_cast<std::size_t>(protectedIndex)].hasBomb) {
            continue;
        }

        for (int target = 0; target < GRID_ROWS * GRID_COLS; ++target) {
            const bool inProtectedZone =
                std::find(protectedIndices.begin(), protectedIndices.end(), target) != protectedIndices.end();
            if (inProtectedZone || grid[static_cast<std::size_t>(target)].hasBomb) {
                continue;
            }

            grid[static_cast<std::size_t>(protectedIndex)].hasBomb = false;
            grid[static_cast<std::size_t>(target)].hasBomb = true;
            break;
        }
    }

    computeAdjacencies(grid);
}

int revealSafeTiles(std::vector<Cell>& grid, int startRow, int startCol) {
    const int startIndex = indexFor(startRow, startCol);
    if (grid[static_cast<std::size_t>(startIndex)].revealed ||
        grid[static_cast<std::size_t>(startIndex)].hasBomb) {
        return 0;
    }

    int revealedCount = 0;
    std::queue<int> pending;
    pending.push(startIndex);

    while (!pending.empty()) {
        const int current = pending.front();
        pending.pop();

        Cell& cell = grid[static_cast<std::size_t>(current)];
        if (cell.revealed || cell.hasBomb) {
            continue;
        }

        cell.revealed = true;
        ++revealedCount;

        if (cell.adjacentBombs != 0) {
            continue;
        }

        const int row = current / GRID_COLS;
        const int col = current % GRID_COLS;
        for (int dRow = -1; dRow <= 1; ++dRow) {
            for (int dCol = -1; dCol <= 1; ++dCol) {
                if (dRow == 0 && dCol == 0) {
                    continue;
                }
                const int nRow = row + dRow;
                const int nCol = col + dCol;
                if (inBounds(nRow, nCol)) {
                    pending.push(indexFor(nRow, nCol));
                }
            }
        }
    }

    return revealedCount;
}

void drawCell(WINDOW* win,
              int y,
              int x,
              const Cell& cell,
              bool selected,
              bool revealBombs,
              bool hasColor) {
    if (selected) {
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

    char content = '.';
    if (revealBombs && cell.hasBomb) {
        content = '*';
    } else if (cell.revealed) {
        content = (cell.adjacentBombs == 0) ? ' ' : static_cast<char>('0' + cell.adjacentBombs);
    }

    if (hasColor && cell.revealed && !cell.hasBomb && cell.adjacentBombs > 0) {
        wattron(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
    } else if (hasColor && revealBombs && cell.hasBomb) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
    mvwaddch(win, y + 1, x + 2, content);
    if (hasColor && cell.revealed && !cell.hasBomb && cell.adjacentBombs > 0) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
    } else if (hasColor && revealBombs && cell.hasBomb) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }

    if (selected) {
        wattroff(win, A_REVERSE);
    }
}

}  // namespace

MinesweeperResult playMinesweeperMinigame(const std::string& playerName, bool hasColor) {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }

    MinesweeperResult result;
    result.safeTilesRevealed = 0;
    result.hitBomb = false;
    result.timedOut = false;
    result.abandoned = false;

    WINDOW* overlay = newwin(0, 0, 0, 0);
    keypad(overlay, TRUE);
    nodelay(overlay, TRUE);
    wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

    std::vector<Cell> grid = createGrid();
    int currentRow = 0;
    int currentCol = 0;
    bool firstReveal = true;
    bool gameOver = false;
    const std::time_t startTime = std::time(nullptr);

    while (true) {
        int screenH = 0;
        int screenW = 0;
        getmaxyx(stdscr, screenH, screenW);
        wresize(overlay, screenH, screenW);
        mvwin(overlay, 0, 0);
        werase(overlay);

        if (hasColor) {
            wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        }

        const int arenaWidth = 72;
        const int arenaHeight = 26;
        const int arenaLeft = (screenW - arenaWidth) / 2;
        const int arenaTop = 8;
        const int arenaRight = arenaLeft + arenaWidth - 1;
        const int arenaBottom = arenaTop + arenaHeight - 1;

        drawAsciiTitle(overlay, screenW, hasColor);

        const int secondsElapsed = static_cast<int>(std::time(nullptr) - startTime);
        int secondsLeft = ROUND_SECONDS - secondsElapsed;
        if (secondsLeft < 0) {
            secondsLeft = 0;
        }

        const std::string statusLine =
            "Player: " + playerName +
            "  |  Safe tiles: " + std::to_string(result.safeTilesRevealed) + "/" +
            std::to_string(TOTAL_SAFE_TILES) +
            "  |  Timer: " + std::to_string(secondsLeft) + "s";
        mvwprintw(overlay, 7, (screenW - static_cast<int>(statusLine.size())) / 2,
                  "%s", statusLine.c_str());

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

        mvwprintw(overlay, arenaTop + 1, arenaLeft + 3,
                  "Reveal as many safe tiles as you can in 60 seconds.");
        mvwprintw(overlay, arenaTop + 2, arenaLeft + 3,
                  "One bomb ends the game. Each safe tile is worth $100.");
        mvwprintw(overlay, arenaTop + 3, arenaLeft + 3,
                  "First reveal opens with a safe 3x3 zone.");

        const int gridStartY = arenaTop + 6;
        const int gridStartX = arenaLeft + (arenaWidth - GRID_WIDTH) / 2;

        for (int row = 0; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                const int idx = indexFor(row, col);
                drawCell(overlay,
                         gridStartY + row * CELL_HEIGHT,
                         gridStartX + col * CELL_WIDTH,
                         grid[static_cast<std::size_t>(idx)],
                         row == currentRow && col == currentCol,
                         gameOver && result.hitBomb,
                         hasColor);
            }
        }

        mvwprintw(overlay, arenaBottom - 2, arenaLeft + 3,
                  "Arrow Keys: Move  |  ENTER/Space: Reveal  |  Q: Quit");

        if (gameOver) {
            std::string endLine;
            if (result.hitBomb) {
                endLine = "Bomb hit. Earned $" + std::to_string(result.safeTilesRevealed * 100) +
                          ". Press ENTER.";
            } else {
                endLine = "Time up. Earned $" + std::to_string(result.safeTilesRevealed * 100) +
                          ". Press ENTER.";
            }
            mvwprintw(overlay, arenaBottom - 4,
                      arenaLeft + (arenaWidth - static_cast<int>(endLine.size())) / 2,
                      "%s", endLine.c_str());
        }

        wrefresh(overlay);

        if (!gameOver && secondsLeft == 0) {
            result.timedOut = true;
            gameOver = true;
            continue;
        }

        const int ch = wgetch(overlay);
        if (ch == ERR) {
            napms(40);
            continue;
        }

        if (ch == 'q' || ch == 'Q') {
            result.abandoned = true;
            break;
        }

        if (gameOver) {
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == ' ') {
                break;
            }
            continue;
        }

        if (ch == KEY_UP) {
            currentRow = clampInt(currentRow - 1, 0, GRID_ROWS - 1);
        } else if (ch == KEY_DOWN) {
            currentRow = clampInt(currentRow + 1, 0, GRID_ROWS - 1);
        } else if (ch == KEY_LEFT) {
            currentCol = clampInt(currentCol - 1, 0, GRID_COLS - 1);
        } else if (ch == KEY_RIGHT) {
            currentCol = clampInt(currentCol + 1, 0, GRID_COLS - 1);
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == ' ') {
            const int idx = indexFor(currentRow, currentCol);
            if (grid[static_cast<std::size_t>(idx)].revealed) {
                continue;
            }

            if (firstReveal) {
                ensureFirstRevealSafeZone(grid, currentRow, currentCol);
                firstReveal = false;
            }

            if (grid[static_cast<std::size_t>(idx)].hasBomb) {
                result.hitBomb = true;
                gameOver = true;
            } else {
                result.safeTilesRevealed += revealSafeTiles(grid, currentRow, currentCol);
                if (result.safeTilesRevealed >= TOTAL_SAFE_TILES) {
                    result.timedOut = true;
                    gameOver = true;
                }
            }
        }
    }

    delwin(overlay);
    touchwin(stdscr);
    refresh();
    return result;
}
