#include "pong.hpp"

#include <ncurses.h>

#include <cmath>
#include <string>
#include <vector>

#include "ui.h"

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
    const int attrs = A_BOLD | A_BLINK;
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

    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);

    WINDOW* overlay = newwin(screenH, screenW, 0, 0);
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
        if (feedbackFrames > 0) {
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
        mvwhline(overlay, arenaTop, arenaLeft, ACS_HLINE, arenaWidth);
        mvwhline(overlay, arenaBottom, arenaLeft, ACS_HLINE, arenaWidth);
        mvwvline(overlay, arenaTop, arenaLeft, ACS_VLINE, arenaHeight);
        mvwvline(overlay, arenaTop, arenaRight, ACS_VLINE, arenaHeight);
        mvwaddch(overlay, arenaTop, arenaLeft, ACS_ULCORNER);
        mvwaddch(overlay, arenaTop, arenaRight, ACS_URCORNER);
        mvwaddch(overlay, arenaBottom, arenaLeft, ACS_LLCORNER);
        mvwaddch(overlay, arenaBottom, arenaRight, ACS_LRCORNER);
        if (hasColor) {
            wattroff(overlay, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        for (int y = topWall; y < bottomWall; y += 2) {
            mvwaddch(overlay, y, centerLineX, ACS_VLINE);
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

        mvwprintw(overlay, arenaBottom + 2, arenaLeft, "UP: A  DOWN: Z  START: X");
        mvwprintw(overlay, arenaBottom + 2, arenaRight - 20, "RIGHT SIDE: CPU");

        if (waitingForServe) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "Rules: one miss ends the game. Each successful return earns $100.");
            mvwprintw(overlay, arenaBottom + 5, arenaLeft,
                      "Press X to serve. Press Q to leave early.");
        } else if (gameOver) {
            mvwprintw(overlay, arenaBottom + 4, arenaLeft,
                      "Game over. Final score %d, payout $%d. Press ENTER.",
                      result.playerScore,
                      result.playerScore * 100);
        }

        wrefresh(overlay);

        int ch = wgetch(overlay);
        if (ch == 'q' || ch == 'Q') {
            result.abandoned = true;
            break;
        }

        if (waitingForServe) {
            if (ch == 'x' || ch == 'X') {
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

        if (ch == 'a' || ch == 'A') {
            playerPaddle.centerY -= 1.2f;
        } else if (ch == 'z' || ch == 'Z') {
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
            feedbackFrames = 26;
            feedbackPositive = true;
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
            feedbackFrames = 9999;
            feedbackPositive = false;
        } else if (ball.x > static_cast<float>(arenaRight - 1)) {
            feedbackText = "CPU MISSED - SERVE AGAIN";
            feedbackFrames = 34;
            feedbackPositive = true;
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
