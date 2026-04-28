#pragma once

#include <ncurses.h>

#include <string>
#include <vector>

struct UiWindowBounds {
    int height;
    int width;
    int y;
    int x;
};

bool terminalIsAtLeast(int minHeight, int minWidth);
void showTerminalSizeWarning(int minHeight, int minWidth, bool hasColor, bool waitForKey = true);
bool calculateCenteredWindowBounds(int preferredHeight,
                                   int preferredWidth,
                                   int minHeight,
                                   int minWidth,
                                   UiWindowBounds& bounds);
WINDOW* createCenteredWindow(int preferredHeight,
                             int preferredWidth,
                             int minHeight = 3,
                             int minWidth = 12);
void clearWindowInterior(WINDOW* win);

void waitForEnterPrompt(WINDOW* win,
                        int y,
                        int x,
                        const std::string& message = "Press ENTER to continue...");

void blinkIndicator(WINDOW* win,
                    int y,
                    int x,
                    const std::string& text,
                    bool hasColor,
                    int colorPair,
                    int blinkCount = 2,
                    int finalHoldMs = 2000,
                    int clearWidth = 0);

void blinkIndicator(const std::string& text,
                    int blinkCount = 2,
                    int finalHoldMs = 2000);

void showPopupMessage(const std::string& title,
                      const std::vector<std::string>& lines,
                      bool hasColor,
                      bool autoAdvance = false);

void showPopupMessage(const std::string& title,
                      const std::string& message,
                      bool hasColor,
                      bool autoAdvance = false);

void showDecisionPopup(const std::string& playerName,
                       const std::string& decision,
                       const std::string& explanation,
                       bool hasColor,
                       bool autoAdvance = false);

std::vector<std::string> wrapUiText(const std::string& text, std::size_t width);
std::string clipUiText(const std::string& text, std::size_t width);
