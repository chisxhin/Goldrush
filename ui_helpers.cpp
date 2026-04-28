#include "ui_helpers.h"

#include "ui.h"

#include <algorithm>
#include <sstream>

namespace {
const int BLINK_HALF_MS = 160;

void drawSolidText(WINDOW* win,
                   int y,
                   int x,
                   const std::string& text,
                   bool hasColor,
                   int colorPair,
                   int attrs) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(colorPair) | attrs);
    } else {
        wattron(win, A_REVERSE | A_BOLD);
    }
    mvwprintw(win, y, x, "%s", text.c_str());
    if (hasColor) {
        wattroff(win, COLOR_PAIR(colorPair) | attrs);
    } else {
        wattroff(win, A_REVERSE | A_BOLD);
    }
}

void clearTextArea(WINDOW* win, int y, int x, int width) {
    if (width <= 0) {
        return;
    }
    mvwprintw(win, y, x, "%-*s", width, "");
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}
}  // namespace

bool terminalIsAtLeast(int minHeight, int minWidth) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    return h >= minHeight && w >= minWidth;
}

void showTerminalSizeWarning(int minHeight, int minWidth, bool hasColor, bool waitForKey) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    if (hasColor) {
        bkgd(COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
    }
    clear();

    const std::string line1 = "Terminal too small - please resize";
    const std::string line2 = "Required: " + std::to_string(minWidth) + "x" + std::to_string(minHeight);
    const std::string line3 = "Current: " + std::to_string(w) + "x" + std::to_string(h);
    const std::string line4 = waitForKey ? "Press any key after resizing." : "Resize the terminal to continue.";
    const std::vector<std::string> lines{line1, line2, line3, line4};
    const int startY = std::max(0, (h - static_cast<int>(lines.size())) / 2);

    for (std::size_t i = 0; i < lines.size(); ++i) {
        const int x = std::max(0, (w - static_cast<int>(lines[i].size())) / 2);
        if (startY + static_cast<int>(i) >= 0 && startY + static_cast<int>(i) < h) {
            mvprintw(startY + static_cast<int>(i), x, "%s", lines[i].c_str());
        }
    }
    refresh();

    if (waitForKey) {
        nodelay(stdscr, FALSE);
        getch();
    }
}

bool calculateCenteredWindowBounds(int preferredHeight,
                                   int preferredWidth,
                                   int minHeight,
                                   int minWidth,
                                   UiWindowBounds& bounds) {
    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    minHeight = std::max(1, minHeight);
    minWidth = std::max(1, minWidth);
    if (screenH < minHeight || screenW < minWidth) {
        return false;
    }

    const int maxH = screenH >= minHeight + 2 ? screenH - 2 : screenH;
    const int maxW = screenW >= minWidth + 2 ? screenW - 2 : screenW;
    bounds.height = std::max(minHeight, std::min(preferredHeight, maxH));
    bounds.width = std::max(minWidth, std::min(preferredWidth, maxW));
    bounds.y = std::max(0, (screenH - bounds.height) / 2);
    bounds.x = std::max(0, (screenW - bounds.width) / 2);

    if (bounds.y + bounds.height > screenH) {
        bounds.y = std::max(0, screenH - bounds.height);
    }
    if (bounds.x + bounds.width > screenW) {
        bounds.x = std::max(0, screenW - bounds.width);
    }
    return bounds.height > 0 &&
           bounds.width > 0 &&
           bounds.y >= 0 &&
           bounds.x >= 0 &&
           bounds.y + bounds.height <= screenH &&
           bounds.x + bounds.width <= screenW;
}

WINDOW* createCenteredWindow(int preferredHeight, int preferredWidth, int minHeight, int minWidth) {
    UiWindowBounds bounds{0, 0, 0, 0};
    if (!calculateCenteredWindowBounds(preferredHeight, preferredWidth, minHeight, minWidth, bounds)) {
        return nullptr;
    }
    WINDOW* win = newwin(bounds.height, bounds.width, bounds.y, bounds.x);
    if (win) {
        apply_ui_background(win);
    }
    return win;
}

void clearWindowInterior(WINDOW* win) {
    if (!win) {
        return;
    }
    int h = 0;
    int w = 0;
    getmaxyx(win, h, w);
    for (int y = 1; y < h - 1; ++y) {
        mvwprintw(win, y, 1, "%*s", std::max(0, w - 2), "");
    }
}

void waitForEnterPrompt(WINDOW* win, int y, int x, const std::string& message) {
    if (!win) {
        return;
    }
    if (!message.empty()) {
        mvwprintw(win, y, x, "%s", message.c_str());
        wrefresh(win);
    }

    nodelay(win, FALSE);
    int ch = 0;
    do {
        ch = wgetch(win);
    } while (ch != '\n' && ch != '\r' && ch != KEY_ENTER);
}

void blinkIndicator(WINDOW* win,
                    int y,
                    int x,
                    const std::string& text,
                    bool hasColor,
                    int colorPair,
                    int blinkCount,
                    int finalHoldMs,
                    int clearWidth) {
    if (!win || text.empty()) {
        return;
    }

    const int width = clearWidth > 0 ? clearWidth : static_cast<int>(text.size());
    drawSolidText(win, y, x, text, hasColor, colorPair, A_BOLD);
    wrefresh(win);
    napms(BLINK_HALF_MS);

    for (int blink = 0; blink < blinkCount; ++blink) {
        clearTextArea(win, y, x, width);
        wrefresh(win);
        napms(BLINK_HALF_MS);

        drawSolidText(win, y, x, text, hasColor, colorPair, A_BOLD | A_BLINK);
        wrefresh(win);
        napms(BLINK_HALF_MS);
    }

    clearTextArea(win, y, x, width);
    drawSolidText(win, y, x, text, hasColor, colorPair, A_BOLD);
    wrefresh(win);
    if (finalHoldMs > 0) {
        napms(finalHoldMs);
    }
}

void blinkIndicator(const std::string& text, int blinkCount, int finalHoldMs) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);
    const int x = std::max(0, (w - static_cast<int>(text.size())) / 2);
    const int y = std::max(0, h / 2);
    blinkIndicator(stdscr,
                   y,
                   x,
                   text,
                   has_colors(),
                   GOLDRUSH_GOLD_SAND,
                   blinkCount,
                   finalHoldMs,
                   static_cast<int>(text.size()));
}

std::vector<std::string> wrapUiText(const std::string& text, std::size_t width) {
    if (width == 0) {
        return splitLines(text);
    }

    std::vector<std::string> out;
    const std::vector<std::string> rawLines = splitLines(text);
    for (std::size_t raw = 0; raw < rawLines.size(); ++raw) {
        std::istringstream words(rawLines[raw]);
        std::string word;
        std::string line;
        while (words >> word) {
            if (word.size() > width) {
                if (!line.empty()) {
                    out.push_back(line);
                    line.clear();
                }
                out.push_back(word.substr(0, width));
                continue;
            }

            if (line.empty()) {
                line = word;
            } else if (line.size() + 1 + word.size() <= width) {
                line += " " + word;
            } else {
                out.push_back(line);
                line = word;
            }
        }
        if (!line.empty()) {
            out.push_back(line);
        }
        if (rawLines[raw].empty()) {
            out.push_back("");
        }
    }
    if (out.empty()) {
        out.push_back("");
    }
    return out;
}

std::string clipUiText(const std::string& text, std::size_t width) {
    if (text.size() <= width) {
        return text;
    }
    if (width <= 3) {
        return text.substr(0, width);
    }
    return text.substr(0, width - 3) + "...";
}

void showPopupMessage(const std::string& title,
                      const std::vector<std::string>& lines,
                      bool hasColor,
                      bool autoAdvance) {
    int h = 0;
    int w = 0;
    getmaxyx(stdscr, h, w);

    const int popupW = std::min(std::max(58, w - 8), 86);
    const int contentW = popupW - 4;
    std::vector<std::string> wrapped;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::vector<std::string> part = wrapUiText(lines[i], static_cast<std::size_t>(contentW));
        wrapped.insert(wrapped.end(), part.begin(), part.end());
    }

    int popupH = std::min(std::max(10, static_cast<int>(wrapped.size()) + 6), h - 2);
    WINDOW* popup = createCenteredWindow(popupH, popupW, 8, 32);
    if (!popup) {
        showTerminalSizeWarning(8, 32, hasColor, !autoAdvance);
        return;
    }
    int actualW = 0;
    getmaxyx(popup, popupH, actualW);
    const int actualContentW = std::max(1, actualW - 4);
    if (actualContentW != contentW) {
        wrapped.clear();
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const std::vector<std::string> part = wrapUiText(lines[i], static_cast<std::size_t>(actualContentW));
            wrapped.insert(wrapped.end(), part.begin(), part.end());
        }
    }
    keypad(popup, TRUE);
    werase(popup);
    drawBoxSafe(popup);

    blinkIndicator(popup,
                   1,
                   2,
                   clipUiText(title, static_cast<std::size_t>(actualContentW)),
                   hasColor,
                   GOLDRUSH_GOLD_SAND,
                   2,
                   0,
                   actualContentW);

    const int maxLines = popupH - 5;
    for (int i = 0; i < maxLines && i < static_cast<int>(wrapped.size()); ++i) {
        mvwprintw(popup, 3 + i, 2, "%s", clipUiText(wrapped[static_cast<std::size_t>(i)],
                                                     static_cast<std::size_t>(actualContentW)).c_str());
    }

    if (autoAdvance) {
        mvwprintw(popup, popupH - 2, 2, "%s",
                  clipUiText("Continuing...", static_cast<std::size_t>(actualContentW)).c_str());
        wrefresh(popup);
        napms(2000);
    } else {
        waitForEnterPrompt(popup,
                           popupH - 2,
                           2,
                           clipUiText("Press ENTER to continue...",
                                      static_cast<std::size_t>(actualContentW)));
    }

    delwin(popup);
    touchwin(stdscr);
    refresh();
}

void showPopupMessage(const std::string& title,
                      const std::string& message,
                      bool hasColor,
                      bool autoAdvance) {
    showPopupMessage(title, splitLines(message), hasColor, autoAdvance);
}

void showDecisionPopup(const std::string& playerName,
                       const std::string& decision,
                       const std::string& explanation,
                       bool hasColor,
                       bool autoAdvance) {
    std::vector<std::string> lines;
    lines.push_back(playerName + " chooses " + decision + ".");
    lines.push_back("");
    lines.push_back(explanation);
    showPopupMessage("Decision Confirmed", lines, hasColor, autoAdvance);
}
