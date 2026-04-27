#pragma once

#include <ncurses.h>

#include <string>

std::string countdownTimerText(int remainingSeconds);
std::string countdownTimerText(double remainingSeconds);
void drawCountdownTimer(WINDOW* win,
                        int y,
                        int x,
                        int remainingSeconds,
                        bool hasColor);
void drawCountdownTimer(WINDOW* win,
                        int y,
                        int x,
                        double remainingSeconds,
                        bool hasColor);
void displayCountdownTimer(int seconds, bool hasColor);
