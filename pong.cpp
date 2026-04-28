#include "pong.hpp"

#include <ncurses.h>

#include <cmath>
#include <string>
#include <vector>

#include "input_helpers.h"
#include "minigame_tutorials.h"
#include "ui.h"
#include "ui_helpers.h"

namespace {
const std::vector<std::string> PONG_TITLE = {
    ".______     ______   .__   __.   _______ ",
    "|   _  \\   /  __  \\  |  \\ |  |  /  _____|",
    "|  |_)  | |  |  |  | |   \\|  | |  |  __  ",
    "|   ___/  |  |  |  | |  . `  | |  | |_ | ",
    "|  |      |  `--'  | |  |\\   | |  |__| | ",
    "| _|       \\______/  |__| \\__|  \\______| ",
    "                                         "
};

void drawAsciiTitle(WINDOW* win, int screenW, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    for (std::size_t i = 0; i < PONG_TITLE.size(); ++i) {
        mvwprintw(win, static_cast<int>(i), (screenW - static_cast<int>(PONG_TITLE[i].size())) / 2,
                  "%s", PONG_TITLE[i].c_str());
    }
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
}

struct Paddle {
    float centerY;
};

struct Ball {
    float x;
    float y;
    float vx;
    float vy;
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

float clampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

void drawPaddle(WINDOW* win, int x, const Paddle& paddle, int paddleHalfHeight, int topBound, int bottomBound) {
    const int top = clampInt(static_cast<int>(std::round(paddle.centerY)) - paddleHalfHeight,
                             topBound,
                             bottomBound - (paddleHalfHeight * 2));
    for (int i = 0; i < paddleHalfHeight * 2 + 1; ++i) {
        mvwaddch(win, top + i, x, ' '|A_REVERSE);
    }
}

void resetBall(Ball& ball, float startX, float startY, int direction) {
    ball.x = startX;
    ball.y = startY;
    ball.vx = 0.55f * static_cast<float>(direction);
    ball.vy = 0.28f;
}

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

PongMinigameResult playPongMinigame(const std::string& playerName, bool hasColor) {
    PongMinigameResult result;
    result.playerScore = 0;
    result.cpuScore = 0;
    result.abandoned = false;

    showMinigameTutorial("Pong",
                         "Return the ball with your paddle for as long as possible.",
                         "W/S or arrows move, X serves, ESC exits.",
                         "Keep returning the ball. One miss ends the run.",
                         "Each successful return pays $100. Exiting early pays nothing.",
                         hasColor);

    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    if (!terminalIsAtLeast(35, 82)) {
        showTerminalSizeWarning(35, 82, hasColor);
        result.abandoned = true;
        return result;
    }

    WINDOW* overlay = newwin(screenH, screenW, 0, 0);
    if (!overlay) {
        showTerminalSizeWarning(35, 82, hasColor);
        result.abandoned = true;
        return result;
    }
    keypad(overlay, TRUE);
    nodelay(overlay, TRUE);
    wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

    const int arenaWidth = 78;
    const int arenaHeight = 24;
    const int arenaLeft = (screenW - arenaWidth) / 2;
    const int arenaTop = 8;
    const int arenaRight = arenaLeft + arenaWidth - 1;
    const int arenaBottom = arenaTop + arenaHeight - 1;
    const int paddleHalfHeight = 2;
    const int leftPaddleX = arenaLeft + 2;
    const int rightPaddleX = arenaRight - 2;
    const int topWall = arenaTop + 1;
    const int bottomWall = arenaBottom - 1;
    const int centerLineX = arenaLeft + arenaWidth / 2;

    Paddle playerPaddle;
    Paddle cpuPaddle;
    playerPaddle.centerY = static_cast<float>((arenaTop + arenaBottom) / 2);
    cpuPaddle.centerY = playerPaddle.centerY;

    Ball ball;
    resetBall(ball,
              static_cast<float>(arenaLeft + arenaWidth / 2),
              static_cast<float>((arenaTop + arenaBottom) / 2),
              1);

    bool waitingForServe = true;
    bool gameOver = false;
    std::string feedbackText;
    int feedbackFrames = 0;
    bool feedbackPositive = true;

    while (true) {
        werase(overlay);

        drawAsciiTitle(overlay, screenW, hasColor);

        const std::string statusLine =
            playerName + " vs CPU  |  One life  |  Score x $100 payout";
        mvwprintw(overlay, 7, (screenW - static_cast<int>(statusLine.size())) / 2,
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
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }
        drawBoxAtSafe(overlay, arenaTop, arenaLeft, arenaHeight, arenaWidth);
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        for (int y = topWall; y < bottomWall; y += 2) {
            drawBorderCharSafe(overlay, y, centerLineX, ACS_VLINE);
        }

        if (hasColor) {
            wattron(overlay, COLOR_PAIR(GOLDRUSH_BROWN_CREAM) | A_BOLD);
        }
        mvwprintw(overlay, arenaTop + 1, arenaLeft + 24, "%d", result.playerScore);
        mvwprintw(overlay, arenaTop + 1, arenaRight - 24, "%d", result.cpuScore);
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_BROWN_CREAM) | A_BOLD);
        }

        drawPaddle(overlay, leftPaddleX, playerPaddle, paddleHalfHeight, topWall + 1, bottomWall - 1);
        drawPaddle(overlay, rightPaddleX, cpuPaddle, paddleHalfHeight, topWall + 1, bottomWall - 1);

        if (hasColor) {
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }
        mvwaddch(overlay,
                 static_cast<int>(std::round(ball.y)),
                 static_cast<int>(std::round(ball.x)),
                 'O');
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        mvwprintw(overlay, arenaBottom + 2, arenaLeft, "UP/DOWN: W/S or arrows  START: X");
        mvwprintw(overlay, arenaBottom + 2, arenaRight - 20, "RIGHT SIDE: CPU");

        if (waitingForServe) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "Rules: one miss ends the game. Each successful return earns $100.");
            mvwprintw(overlay, arenaBottom + 5, arenaLeft,
                      "Press X to serve. Press ESC to leave early.");
        } else if (gameOver) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "Game over. Final score %d, payout $%d. Press ENTER.",
                      result.playerScore,
                      result.playerScore * 100);
        }

        wrefresh(overlay);

        int ch = wgetch(overlay);
        const InputAction action = getInputAction(ch, ControlScheme::SinglePlayer);
        if (action == InputAction::Cancel) {
            result.abandoned = true;
            break;
        }

        if (waitingForServe) {
            if (action == InputAction::Start) {
                waitingForServe = false;
            } else {
                napms(16);
                continue;
            }
        }

        if (gameOver) {
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                break;
            }
            napms(16);
            continue;
        }

        if (action == InputAction::Up) {
            playerPaddle.centerY -= 1.2f;
        } else if (action == InputAction::Down) {
            playerPaddle.centerY += 1.2f;
        }

        playerPaddle.centerY = clampFloat(playerPaddle.centerY,
                                          static_cast<float>(topWall + paddleHalfHeight + 1),
                                          static_cast<float>(bottomWall - paddleHalfHeight - 1));

        if (cpuPaddle.centerY + 0.5f < ball.y) {
            cpuPaddle.centerY += 0.55f;
        } else if (cpuPaddle.centerY - 0.5f > ball.y) {
            cpuPaddle.centerY -= 0.55f;
        }
        cpuPaddle.centerY = clampFloat(cpuPaddle.centerY,
                                       static_cast<float>(topWall + paddleHalfHeight + 1),
                                       static_cast<float>(bottomWall - paddleHalfHeight - 1));

        ball.x += ball.vx;
        ball.y += ball.vy;

        if (ball.y <= static_cast<float>(topWall + 1) || ball.y >= static_cast<float>(bottomWall - 1)) {
            ball.vy = -ball.vy;
            ball.y += ball.vy;
        }

        const float playerTop = playerPaddle.centerY - static_cast<float>(paddleHalfHeight);
        const float playerBottom = playerPaddle.centerY + static_cast<float>(paddleHalfHeight);
        if (ball.vx < 0.0f &&
            ball.x <= static_cast<float>(leftPaddleX + 1) &&
            ball.y >= playerTop - 0.25f &&
            ball.y <= playerBottom + 0.25f) {
            ++result.playerScore;
            ball.vx = std::fabs(ball.vx) + 0.03f;
            ball.vy += (ball.y - playerPaddle.centerY) * 0.08f;
            feedbackText = "RETURN! +$100";
            feedbackPositive = true;
            feedbackFrames = 18;
        }

        const float cpuTop = cpuPaddle.centerY - static_cast<float>(paddleHalfHeight);
        const float cpuBottom = cpuPaddle.centerY + static_cast<float>(paddleHalfHeight);
        if (ball.vx > 0.0f &&
            ball.x >= static_cast<float>(rightPaddleX - 1) &&
            ball.y >= cpuTop - 0.25f &&
            ball.y <= cpuBottom + 0.25f) {
            ball.vx = -(std::fabs(ball.vx) + 0.03f);
            ball.vy += (ball.y - cpuPaddle.centerY) * 0.05f;
        }

        if (ball.x < static_cast<float>(arenaLeft + 1)) {
            ++result.cpuScore;
            gameOver = true;
            feedbackText = "MISS! RUN ENDS";
            feedbackPositive = false;
            feedbackFrames = 18;
        } else if (ball.x > static_cast<float>(arenaRight - 1)) {
            feedbackText = "CPU MISSED - SERVE AGAIN";
            feedbackPositive = true;
            feedbackFrames = 18;
            resetBall(ball,
                      static_cast<float>(arenaLeft + arenaWidth / 2),
                      static_cast<float>((arenaTop + arenaBottom) / 2),
                      (result.playerScore % 2 == 0) ? 1 : -1);
            waitingForServe = true;
        }

        napms(16);
    }

    nodelay(overlay, FALSE);
    delwin(overlay);
    touchwin(stdscr);
    refresh();
    return result;
}

PongDuelResult playPongDuelMinigame(const std::string& leftPlayerName,
                                    const std::string& rightPlayerName,
                                    bool hasColor,
                                    bool rightSideCpu) {
    PongDuelResult result;
    result.winnerSide = -1;
    result.abandoned = false;

    showMinigameTutorial("Pong Duel",
                         "Outlast your rival in a one-life Pong duel.",
                         rightSideCpu
                             ? "Left: W/S, Start: X, ESC exits. Right side is CPU."
                             : "Left: W/S, Right: Up/Down, Start: X, ESC exits.",
                         "The first side to miss the ball loses immediately.",
                         "Winner steals the duel prize. There is no timeout.",
                         hasColor);

    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    if (!terminalIsAtLeast(35, 82)) {
        showTerminalSizeWarning(35, 82, hasColor);
        result.abandoned = true;
        return result;
    }

    WINDOW* overlay = newwin(screenH, screenW, 0, 0);
    if (!overlay) {
        showTerminalSizeWarning(35, 82, hasColor);
        result.abandoned = true;
        return result;
    }
    keypad(overlay, TRUE);
    nodelay(overlay, TRUE);
    wbkgd(overlay, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));

    const int arenaWidth = 78;
    const int arenaHeight = 24;
    const int arenaLeft = (screenW - arenaWidth) / 2;
    const int arenaTop = 8;
    const int arenaRight = arenaLeft + arenaWidth - 1;
    const int arenaBottom = arenaTop + arenaHeight - 1;
    const int paddleHalfHeight = 2;
    const int leftPaddleX = arenaLeft + 2;
    const int rightPaddleX = arenaRight - 2;
    const int topWall = arenaTop + 1;
    const int bottomWall = arenaBottom - 1;
    const int centerLineX = arenaLeft + arenaWidth / 2;

    Paddle leftPaddle;
    Paddle rightPaddle;
    leftPaddle.centerY = static_cast<float>((arenaTop + arenaBottom) / 2);
    rightPaddle.centerY = leftPaddle.centerY;

    Ball ball;
    resetBall(ball,
              static_cast<float>(arenaLeft + arenaWidth / 2),
              static_cast<float>((arenaTop + arenaBottom) / 2),
              1);

    bool waitingForServe = true;
    bool gameOver = false;
    std::string feedbackText;
    int feedbackFrames = 0;
    bool feedbackPositive = true;

    while (true) {
        werase(overlay);
        drawAsciiTitle(overlay, screenW, hasColor);

        const std::string statusLine = leftPlayerName + " vs " + rightPlayerName + "  |  First miss loses";
        mvwprintw(overlay, 7, (screenW - static_cast<int>(statusLine.size())) / 2, "%s", statusLine.c_str());
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
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }
        drawBoxAtSafe(overlay, arenaTop, arenaLeft, arenaHeight, arenaWidth);
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        for (int y = topWall; y < bottomWall; y += 2) {
            drawBorderCharSafe(overlay, y, centerLineX, ACS_VLINE);
        }

        drawPaddle(overlay, leftPaddleX, leftPaddle, paddleHalfHeight, topWall + 1, bottomWall - 1);
        drawPaddle(overlay, rightPaddleX, rightPaddle, paddleHalfHeight, topWall + 1, bottomWall - 1);

        if (hasColor) {
            wattron(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }
        mvwaddch(overlay,
                 static_cast<int>(std::round(ball.y)),
                 static_cast<int>(std::round(ball.x)),
                 'O');
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        mvwprintw(overlay, arenaBottom + 2, arenaLeft, "LEFT: W/S  START: X");
        mvwprintw(overlay, arenaBottom + 2, arenaRight - 27,
                  rightSideCpu ? "RIGHT SIDE: CPU" : "RIGHT: UP/DOWN");

        if (waitingForServe) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "The first side to miss loses the duel.");
            mvwprintw(overlay, arenaBottom + 5, arenaLeft,
                      "Press X to serve. Press ESC to leave early.");
        } else if (gameOver) {
            const std::string endLine = std::string("Winner: ") +
                (result.winnerSide == 0 ? leftPlayerName : rightPlayerName) +
                ". Press ENTER.";
            mvwprintw(overlay, arenaBottom + 4, arenaLeft, "%s", endLine.c_str());
        }

        wrefresh(overlay);

        int ch = wgetch(overlay);
        const InputAction leftAction = getInputAction(ch, ControlScheme::DuelLeftPlayer);
        const InputAction rightAction = getInputAction(ch, ControlScheme::DuelRightPlayer);
        if (leftAction == InputAction::Cancel) {
            result.abandoned = true;
            break;
        }

        if (waitingForServe) {
            if (leftAction == InputAction::Start) {
                waitingForServe = false;
            } else {
                napms(16);
                continue;
            }
        }

        if (gameOver) {
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                break;
            }
            napms(16);
            continue;
        }

        if (leftAction == InputAction::Up) {
            leftPaddle.centerY -= 1.2f;
        } else if (leftAction == InputAction::Down) {
            leftPaddle.centerY += 1.2f;
        }
        if (!rightSideCpu) {
            if (rightAction == InputAction::Up) {
                rightPaddle.centerY -= 1.2f;
            } else if (rightAction == InputAction::Down) {
                rightPaddle.centerY += 1.2f;
            }
        }

        leftPaddle.centerY = clampFloat(leftPaddle.centerY,
                                        static_cast<float>(topWall + paddleHalfHeight + 1),
                                        static_cast<float>(bottomWall - paddleHalfHeight - 1));
        if (rightSideCpu) {
            if (rightPaddle.centerY + 0.5f < ball.y) {
                rightPaddle.centerY += 0.55f;
            } else if (rightPaddle.centerY - 0.5f > ball.y) {
                rightPaddle.centerY -= 0.55f;
            }
        }
        rightPaddle.centerY = clampFloat(rightPaddle.centerY,
                                         static_cast<float>(topWall + paddleHalfHeight + 1),
                                         static_cast<float>(bottomWall - paddleHalfHeight - 1));

        ball.x += ball.vx;
        ball.y += ball.vy;
        if (ball.y <= static_cast<float>(topWall + 1) || ball.y >= static_cast<float>(bottomWall - 1)) {
            ball.vy = -ball.vy;
            ball.y += ball.vy;
        }

        const float leftTop = leftPaddle.centerY - static_cast<float>(paddleHalfHeight);
        const float leftBottom = leftPaddle.centerY + static_cast<float>(paddleHalfHeight);
        if (ball.vx < 0.0f &&
            ball.x <= static_cast<float>(leftPaddleX + 1) &&
            ball.y >= leftTop - 0.25f &&
            ball.y <= leftBottom + 0.25f) {
            ball.vx = std::fabs(ball.vx) + 0.03f;
            ball.vy += (ball.y - leftPaddle.centerY) * 0.08f;
            feedbackText = leftPlayerName + " keeps it alive!";
            feedbackPositive = true;
            feedbackFrames = 12;
        }

        const float rightTop = rightPaddle.centerY - static_cast<float>(paddleHalfHeight);
        const float rightBottom = rightPaddle.centerY + static_cast<float>(paddleHalfHeight);
        if (ball.vx > 0.0f &&
            ball.x >= static_cast<float>(rightPaddleX - 1) &&
            ball.y >= rightTop - 0.25f &&
            ball.y <= rightBottom + 0.25f) {
            ball.vx = -(std::fabs(ball.vx) + 0.03f);
            ball.vy += (ball.y - rightPaddle.centerY) * 0.08f;
            feedbackText = rightPlayerName + " answers back!";
            feedbackPositive = true;
            feedbackFrames = 12;
        }

        if (ball.x < static_cast<float>(arenaLeft + 1)) {
            result.winnerSide = 1;
            gameOver = true;
            feedbackText = leftPlayerName + " falls first!";
            feedbackPositive = false;
            feedbackFrames = 18;
        } else if (ball.x > static_cast<float>(arenaRight - 1)) {
            result.winnerSide = 0;
            gameOver = true;
            feedbackText = rightPlayerName + " falls first!";
            feedbackPositive = true;
            feedbackFrames = 18;
        }

        napms(16);
    }

    nodelay(overlay, FALSE);
    delwin(overlay);
    touchwin(stdscr);
    refresh();
    return result;
}
