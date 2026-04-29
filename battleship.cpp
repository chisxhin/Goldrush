#include "battleship.hpp"

#include <ncurses.h>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "minigame_tutorials.h"
#include "input_helpers.h"
#include "ui.h"
#include "ui_helpers.h"

namespace {
const std::vector<std::string> BATTLESHIP_TITLE = {
    " ____    ____  ______  ______  _        ___  _____ __ __  ____  ____  ",
    "|    \\  /    ||      ||      || |      /  _]/ ___/|  |  ||    ||    \\ ",
    "|  o  )|  o  ||      ||      || |     /  [_(   \\_ |  |  | |  | |  o  )",
    "|     ||     ||_|  |_||_|  |_|| |___ |    _]\\__  ||  _  | |  | |   _/ ",
    "|  O  ||  _  |  |  |    |  |  |     ||   [_ /  \\ ||  |  | |  | |  |   ",
    "|     ||  |  |  |  |    |  |  |     ||     |\\    ||  |  | |  | |  |   ",
    "|_____||__|__|  |__|    |__|  |_____||_____| \\___||__|__||____||__|   ",
    "                                                                      "
};

//Input: ncurses window, screen width, color flag
//Output: none (prints ASCII art title)
//Purpose: draws the Battleship ASCII banner centered on screen
//Relation: called at the start of each frame in the main loop to render the title.
void drawAsciiTitle(WINDOW* win, int screenW, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    for (std::size_t i = 0; i < BATTLESHIP_TITLE.size(); ++i) {
        mvwprintw(win, static_cast<int>(i), (screenW - static_cast<int>(BATTLESHIP_TITLE[i].size())) / 2,
                  "%s", BATTLESHIP_TITLE[i].c_str());
    }
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
}

//Fields: x, y, alive
//Purpose: represents each enemy ship’s position and state
//Relation: stored in vector, updated when hit by player shots.
struct EnemyShip {
    int x;
    int y;
    bool alive;
};

//Fields: x, y, dy (direction)
//Purpose: represents bullets fired by player or enemies
//Relation: updated each frame, checked for collisions.
struct Shot {
    int x;
    int y;
    int dy;
};

//Input: integer value, min, max
//Output: clamped integer
//Purpose: ensures player ship stays within arena bounds
//Relation: used after player movement to prevent leaving the play area.
int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

//Input: vector of shots, min/max Y bounds
//Output: modifies vector (removes shots outside bounds)
//Purpose: cleans up bullets that leave the arena
//Relation: called each frame for both player and enemy shots.
void removeInactiveShots(std::vector<Shot>& shots, int minY, int maxY) {
    shots.erase(
        std::remove_if(
            shots.begin(),
            shots.end(),
            [minY, maxY](const Shot& shot) { return shot.y < minY || shot.y > maxY; }),
        shots.end());
}

//Input: ncurses window, coordinates, color flag
//Output: none (prints enemy ship symbol <$>)
//Purpose: renders enemy ships
//Relation: used in main loop to draw all alive enemies.
void drawEnemyShip(WINDOW* win, int y, int x, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
    mvwprintw(win, y, x, "<$>");
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
}

//Input: ncurses window, coordinates, color flag
//Output: none (prints player ship /^\ and /_\)
//Purpose: renders player ship
//Relation: called once per frame to show player position.
void drawPlayerShip(WINDOW* win, int y, int x, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
    }
    mvwprintw(win, y, x, "/^\\");
    mvwprintw(win, y + 1, x, "/_\\");
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
    }
}

//Input: ncurses window, position, arena dimensions, text, positive/negative flag, color flag
//Output: none (prints banner message)
//Purpose: shows feedback messages (reload, hit, etc.)
//Relation: triggered by events (out of ammo, hit enemy, reload complete).
void drawFeedbackBanner(WINDOW* win,
                        int y,
                        int arenaLeft,
                        int arenaWidth,
                        const std::string& text,
                        bool positive,
                        bool hasColor) {
    if (text.empty()) {
        return;
    }

    const int colorPair = positive ? GOLDRUSH_BLACK_FOREST : GOLDRUSH_GOLD_TERRA;
    const int attrs = A_BOLD;
    if (hasColor) {
        wattron(win, COLOR_PAIR(colorPair) | attrs);
    } else {
        wattron(win, A_REVERSE | A_BOLD);
    }
    mvwprintw(win,
              y,
              arenaLeft + (arenaWidth - static_cast<int>(text.size())) / 2,
              "%s",
              text.c_str());
    if (hasColor) {
        wattroff(win, COLOR_PAIR(colorPair) | attrs);
    } else {
        wattroff(win, A_REVERSE | A_BOLD);
    }
}
}

//Input: player name, color flag
//Output: BattleshipMinigameResult (shipsDestroyed, clearedWave, abandoned)
//Purpose: runs the entire Battleship minigame loop
//Relation:
//Calls tutorials (showMinigameTutorial) and warnings (showTerminalSizeWarning)
//Uses drawing functions (drawAsciiTitle, drawEnemyShip, drawPlayerShip, drawFeedbackBanner)
//Uses utility functions (clampInt, removeInactiveShots)
//Updates EnemyShip and Shot vectors each frame
//Handles input actions (move, fire, reload, cancel)
//Tracks game state (waitingForStart, gameOver, feedback messages)
//Ends when player quits, is hit, or clears all ships.
BattleshipMinigameResult playBattleshipMinigame(const std::string& playerName, bool hasColor) {
    BattleshipMinigameResult result;
    result.shipsDestroyed = 0;
    result.clearedWave = false;
    result.abandoned = false;

    showMinigameTutorial("Battleship",
                         "Destroy the money ships while dodging enemy fire.",
                         "Press X to start. A/D or arrows move. Space/Enter fires. R reloads.",
                         "Clear the wave or survive until the run ends.",
                         "Each ship destroyed pays $100. Ammo is limited.",
                         hasColor);

    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    if (!terminalIsAtLeast(37, 90)) {
        showTerminalSizeWarning(37, 90, hasColor);
        result.abandoned = true;
        return result;
    }

    WINDOW* overlay = newwin(screenH, screenW, 0, 0);
    if (!overlay) {
        showTerminalSizeWarning(37, 90, hasColor);
        result.abandoned = true;
        return result;
    }
    keypad(overlay, TRUE);
    nodelay(overlay, TRUE);
    wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

    const int arenaWidth = 86;
    const int arenaHeight = 28;
    const int arenaLeft = (screenW - arenaWidth) / 2;
    const int arenaTop = 9;
    const int arenaRight = arenaLeft + arenaWidth - 1;
    const int arenaBottom = arenaTop + arenaHeight - 1;
    const int playerY = arenaBottom - 3;
    const int enemyTopY = arenaTop + 4;
    const int enemySpacingX = 7;
    const int enemySpacingY = 3;
    const int enemyRows = 2;
    const int enemyCols = 5;
    const int maxShips = enemyRows * enemyCols;

    std::vector<EnemyShip> enemies;
    enemies.reserve(static_cast<std::size_t>(maxShips));
    const int formationStartX = arenaLeft + 12;
    for (int row = 0; row < enemyRows; ++row) {
        for (int col = 0; col < enemyCols; ++col) {
            EnemyShip ship;
            ship.x = formationStartX + (col * enemySpacingX);
            ship.y = enemyTopY + (row * enemySpacingY);
            ship.alive = true;
            enemies.push_back(ship);
        }
    }

    int playerX = arenaLeft + arenaWidth / 2;
    int formationOffsetX = 0;
    int formationDirection = 1;
    int frameCounter = 0;
    bool waitingForStart = true;
    bool gameOver = false;
    std::string feedbackText;
    int feedbackFrames = 0;
    bool feedbackPositive = true;
    const int maxAmmo = 3;
    int ammo = maxAmmo;

    std::vector<Shot> playerShots;
    std::vector<Shot> enemyShots;

    while (true) {
        werase(overlay);

        drawAsciiTitle(overlay, screenW, hasColor);

        const std::string statusLine =
            playerName + " vs Enemy Fleet  |  Score: " + std::to_string(result.shipsDestroyed) +
            "  |  Wave: 1  |  Ammo: " + std::to_string(ammo) + " / " + std::to_string(maxAmmo);
        mvwprintw(overlay, 8, (screenW - static_cast<int>(statusLine.size())) / 2,
                  "%s", statusLine.c_str());
        if (feedbackFrames > 0 && ((feedbackFrames / 2) % 2 == 0)) {
            drawFeedbackBanner(overlay,
                               arenaTop + 2,
                               arenaLeft,
                               arenaWidth,
                               feedbackText,
                               feedbackPositive,
                               hasColor);
            if (!gameOver) {
                --feedbackFrames;
            }
        }

        if (hasColor) {
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }
        drawBoxAtSafe(overlay, arenaTop, arenaLeft, arenaHeight, arenaWidth);
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }

        for (std::size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i].alive) {
                continue;
            }
            drawEnemyShip(overlay,
                          enemies[i].y,
                          enemies[i].x + formationOffsetX,
                          hasColor);
        }

        drawPlayerShip(overlay, playerY, playerX, hasColor);

        for (std::size_t i = 0; i < playerShots.size(); ++i) {
            if (hasColor) {
                wattron(overlay, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
            }
            mvwaddch(overlay, playerShots[i].y, playerShots[i].x + 1, '|');
            if (hasColor) {
                wattroff(overlay, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
            }
        }

        for (std::size_t i = 0; i < enemyShots.size(); ++i) {
            if (hasColor) {
                wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
            }
            mvwaddch(overlay, enemyShots[i].y, enemyShots[i].x + 1, '!');
            if (hasColor) {
                wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
            }
        }

        mvwprintw(overlay, arenaBottom + 2, arenaLeft,
                  "MOVE: A/D or LEFT/RIGHT  FIRE: SPACE/ENTER  RELOAD: R");

        if (waitingForStart) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "Rules: destroy the $ ships. One enemy hit ends the game.");
            mvwprintw(overlay, arenaBottom + 5, arenaLeft,
                      "Each ship destroyed earns $100. Max payout: $1000. Press X to start.");
        } else if (gameOver) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "Wave over. Destroyed %d/%d ships, payout $%d. Press ENTER.",
                      result.shipsDestroyed,
                      maxShips,
                      result.shipsDestroyed * 100);
        } else {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "One wave only. Dodge enemy fire or the run ends immediately.");
        }

        wrefresh(overlay);

        int ch = wgetch(overlay);
        const InputAction action = getInputAction(ch, ControlScheme::SinglePlayer);
        if (action == InputAction::Cancel) {
            result.abandoned = true;
            break;
        }

        if (waitingForStart) {
            if (action == InputAction::Start) {
                waitingForStart = false;
            } else {
                napms(20);
                continue;
            }
        }

        if (gameOver) {
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                break;
            }
            napms(20);
            continue;
        }

        if (action == InputAction::Left) {
            playerX -= 2;
        } else if (action == InputAction::Right) {
            playerX += 2;
        } else if (action == InputAction::Fire || action == InputAction::Confirm) {
            if (ammo <= 0) {
                feedbackText = "Out of ammo! Press R to reload.";
                feedbackFrames = 24;
                feedbackPositive = false;
            } else if (playerShots.size() < 2) {
                Shot shot;
                shot.x = playerX;
                shot.y = playerY - 1;
                shot.dy = -1;
                playerShots.push_back(shot);
                --ammo;
            }
        } else if (action == InputAction::Reload) {
            if (ammo < maxAmmo) {
                feedbackText = "Reloading...";
                feedbackFrames = 16;
                feedbackPositive = true;
                wrefresh(overlay);
                napms(450);
                ammo = maxAmmo;
                feedbackText = "Reload complete.";
                feedbackFrames = 18;
            }
        }

        playerX = clampInt(playerX, arenaLeft + 2, arenaRight - 4);

        ++frameCounter;
        if (frameCounter % 7 == 0) {
            int leftEdge = arenaRight;
            int rightEdge = arenaLeft;
            for (std::size_t i = 0; i < enemies.size(); ++i) {
                if (!enemies[i].alive) {
                    continue;
                }
                leftEdge = std::min(leftEdge, enemies[i].x + formationOffsetX);
                rightEdge = std::max(rightEdge, enemies[i].x + formationOffsetX + 2);
            }

            if (leftEdge <= arenaLeft + 2) {
                formationDirection = 1;
            } else if (rightEdge >= arenaRight - 2) {
                formationDirection = -1;
            }
            formationOffsetX += formationDirection;
        }

        if (frameCounter % 24 == 0 && (std::rand() % 100) < 65) {
            std::vector<int> livingShips;
            for (std::size_t i = 0; i < enemies.size(); ++i) {
                if (!enemies[i].alive) {
                    continue;
                }
                livingShips.push_back(static_cast<int>(i));
            }

            if (!livingShips.empty()) {
                const int firingIndex = livingShips[static_cast<std::size_t>(std::rand() % livingShips.size())];
                Shot shot;
                shot.x = enemies[static_cast<std::size_t>(firingIndex)].x + formationOffsetX;
                shot.y = enemies[static_cast<std::size_t>(firingIndex)].y + 1;
                shot.dy = 1;
                enemyShots.push_back(shot);
            }
        }

        for (std::size_t i = 0; i < playerShots.size(); ++i) {
            playerShots[i].y += playerShots[i].dy;
        }
        if (frameCounter % 2 == 0) {
            for (std::size_t i = 0; i < enemyShots.size(); ++i) {
                enemyShots[i].y += enemyShots[i].dy;
            }
        }

        for (std::size_t shotIndex = 0; shotIndex < playerShots.size(); ++shotIndex) {
            for (std::size_t enemyIndex = 0; enemyIndex < enemies.size(); ++enemyIndex) {
                if (!enemies[enemyIndex].alive) {
                    continue;
                }
                const int enemyX = enemies[enemyIndex].x + formationOffsetX;
                const int enemyY = enemies[enemyIndex].y;
                if (playerShots[shotIndex].y == enemyY &&
                    playerShots[shotIndex].x >= enemyX &&
                    playerShots[shotIndex].x <= enemyX + 2) {
                    enemies[enemyIndex].alive = false;
                    playerShots[shotIndex].y = arenaTop - 1;
                    ++result.shipsDestroyed;
                    feedbackText = "DIRECT HIT! +$100";
                    feedbackPositive = true;
                    feedbackFrames = 18;
                    break;
                }
            }
        }

        for (std::size_t i = 0; i < enemyShots.size(); ++i) {
            if ((enemyShots[i].y == playerY || enemyShots[i].y == playerY + 1) &&
                enemyShots[i].x >= playerX &&
                enemyShots[i].x <= playerX + 2) {
                gameOver = true;
                feedbackText = "HIT TAKEN! RUN ENDS";
                feedbackPositive = false;
                feedbackFrames = 18;
                break;
            }
        }

        removeInactiveShots(playerShots, arenaTop + 1, arenaBottom - 1);
        removeInactiveShots(enemyShots, arenaTop + 1, arenaBottom - 1);

        bool anyAlive = false;
        for (std::size_t i = 0; i < enemies.size(); ++i) {
            if (enemies[i].alive) {
                anyAlive = true;
                break;
            }
        }

        if (!anyAlive) {
            result.clearedWave = true;
            gameOver = true;
            feedbackText = "WAVE CLEARED!";
            feedbackPositive = true;
            feedbackFrames = 18;
        }

        napms(20);
    }

    nodelay(overlay, FALSE);
    delwin(overlay);
    touchwin(stdscr);
    refresh();
    return result;
}
