#include "board.hpp"
#include "tile_display.h"
#include "ui.h"

#include <algorithm>
#include <set>
#include <utility>
#include <string>
#include <vector>

namespace {

struct UiPos {
    int x;
    int y;
};

const int PLAYER_PAIR_BASE = 9;
const int VIEW_COLS = 5;
const int VIEW_ROWS = 3;
const int TILE_W = 12;
const int TILE_H = 5;
const int TILE_GAP_X = 1;
const int TILE_GAP_Y = 1;

int centeredX(int areaLeft, int areaWidth, int textWidth) {
    return areaLeft + std::max(0, (areaWidth - textWidth) / 2);
}

std::string clipText(const std::string& text, int maxWidth) {
    if (maxWidth <= 0) {
        return "";
    }
    if (static_cast<int>(text.size()) <= maxWidth) {
        return text;
    }
    return text.substr(0, static_cast<std::size_t>(maxWidth));
}

std::string tileTitle(const Tile& tile) {
    switch (tile.kind) {
        case TILE_BLACK:
            if (tile.id >= 14 && tile.id <= 24) return "College";
            if (tile.id >= 26 && tile.id <= 36) return "Career";
            if (tile.id >= 59 && tile.id <= 68) return "Family";
            if (tile.id >= 69 && tile.id <= 78) return "Work";
            if (tile.id >= 80 && tile.id <= 82) return "Safe";
            if (tile.id >= 83 && tile.id <= 85) return "Risk";
            return "Action";
        case TILE_START: return "Start";
        case TILE_SPLIT_START: return "Choice";
        case TILE_COLLEGE: return "College";
        case TILE_CAREER: return "Career";
        case TILE_GRADUATION: return "Grad";
        case TILE_MARRIAGE: return "Married";
        case TILE_SPLIT_FAMILY: return "Family";
        case TILE_FAMILY: return "Family";
        case TILE_NIGHT_SCHOOL: return "Night";
        case TILE_SPLIT_RISK: return "Fork";
        case TILE_SAFE: return "Safe";
        case TILE_RISKY: return "Risk";
        case TILE_SPIN_AGAIN: return "Spin";
        case TILE_CAREER_2: return "Work";
        case TILE_PAYDAY: return "Payday";
        case TILE_BABY: return "Baby";
        case TILE_RETIREMENT: return "Retire";
        case TILE_HOUSE: return "House";
        case TILE_EMPTY:
        default:
            return "Road";
    }
}

std::string tileCaption(const Tile& tile) {
    return clipText(getTileAbbreviation(tile), TILE_W - 2);
}

int tileColorPair(const Tile& tile) {
    switch (tile.kind) {
        case TILE_BLACK:
        case TILE_RISKY:
        case TILE_MARRIAGE:
        case TILE_SPLIT_START:
        case TILE_SPLIT_FAMILY:
        case TILE_SPLIT_RISK:
        case TILE_HOUSE:
            return 5;
        case TILE_START:
        case TILE_RETIREMENT:
        case TILE_PAYDAY:
        case TILE_GRADUATION:
        case TILE_BABY:
        case TILE_NIGHT_SCHOOL:
        case TILE_SPIN_AGAIN:
            return 4;
        default:
            return 3;
    }
}

std::vector<int> playersAtTile(const std::vector<Player>& players, int tileIndex) {
    std::vector<int> indices;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (players[i].tile == tileIndex) {
            indices.push_back(static_cast<int>(i));
        }
    }
    return indices;
}

std::vector<std::pair<int, int> > boardConnections(const std::vector<Tile>& tiles) {
    std::vector<std::pair<int, int> > connections;
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        if (tiles[i].next >= 0) {
            connections.push_back(std::make_pair(static_cast<int>(i), tiles[i].next));
        }
        if (tiles[i].altNext >= 0) {
            connections.push_back(std::make_pair(static_cast<int>(i), tiles[i].altNext));
        }
    }
    return connections;
}

std::set<int> reachableTiles(const std::vector<Tile>& tiles) {
    std::set<int> reachable;
    std::vector<int> pending;
    pending.push_back(0);

    while (!pending.empty()) {
        const int current = pending.back();
        pending.pop_back();
        if (current < 0 || current >= static_cast<int>(tiles.size()) || reachable.count(current) != 0) {
            continue;
        }

        reachable.insert(current);
        if (tiles[static_cast<std::size_t>(current)].next >= 0) {
            pending.push_back(tiles[static_cast<std::size_t>(current)].next);
        }
        if (tiles[static_cast<std::size_t>(current)].altNext >= 0) {
            pending.push_back(tiles[static_cast<std::size_t>(current)].altNext);
        }
    }

    return reachable;
}

int nextTileForView(const std::vector<Tile>& tiles, const Player& player, int tileId) {
    const Tile& tile = tiles[static_cast<std::size_t>(tileId)];
    if (tile.kind == TILE_SPLIT_START || tile.kind == TILE_START) {
        if (player.startChoice == 0) {
            return tile.next;
        }
        if (player.startChoice == 1) {
            return tile.altNext;
        }
        return tile.next;
    }
    if (tile.kind == TILE_SPLIT_FAMILY) {
        if (player.familyChoice == 0) {
            return tile.next;
        }
        if (player.familyChoice == 1) {
            return tile.altNext;
        }
        return tile.altNext;
    }
    if (tile.kind == TILE_SPLIT_RISK) {
        if (player.riskChoice == 0) {
            return tile.next;
        }
        if (player.riskChoice == 1) {
            return tile.altNext;
        }
        return tile.next;
    }
    return tile.next;
}

int previousTileForView(const std::vector<Tile>& tiles, const Player& player, int tileId) {
    std::vector<int> candidates;
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        if (tiles[i].next == tileId || tiles[i].altNext == tileId) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) {
        return -1;
    }
    if (candidates.size() == 1) {
        return candidates[0];
    }
    if (tileId == 1) {
        return player.startChoice == 0 ? 37 : 36;
    }
    if (tileId == 79) {
        return player.familyChoice == 0 ? 68 : 78;
    }
    if (tileId == 87) {
        return 86;
    }
    if (tileId == 86) {
        return player.riskChoice == 0 ? 82 : 85;
    }
    return candidates[0];
}

std::set<int> buildVisibleTrail(const std::vector<Tile>& tiles, const Player& player, int focusTile) {
    std::set<int> visible;
    visible.insert(focusTile);

    int cursor = focusTile;
    for (int i = 0; i < 3; ++i) {
        cursor = previousTileForView(tiles, player, cursor);
        if (cursor < 0) {
            break;
        }
        visible.insert(cursor);
        if (tiles[static_cast<std::size_t>(cursor)].stop) {
            break;
        }
    }

    cursor = focusTile;
    for (int i = 0; i < 6; ++i) {
        cursor = nextTileForView(tiles, player, cursor);
        if (cursor < 0) {
            break;
        }
        visible.insert(cursor);
        if (tiles[static_cast<std::size_t>(cursor)].stop) {
            break;
        }
    }

    const Tile& focus = tiles[static_cast<std::size_t>(focusTile)];
    if (focus.kind == TILE_START || focus.kind == TILE_SPLIT_START) {
        if (focus.next >= 0) {
            visible.insert(focus.next);
        }
        if (focus.altNext >= 0) {
            visible.insert(focus.altNext);
        }
    }
    if (focus.kind == TILE_SPLIT_FAMILY || focus.kind == TILE_SPLIT_RISK) {
        if (focus.next >= 0) {
            visible.insert(focus.next);
        }
        if (focus.altNext >= 0) {
            visible.insert(focus.altNext);
        }
    }

    return visible;
}

void drawLineSegment(WINDOW* win, int y1, int x1, int y2, int x2, bool hasColor, int colorPair, int maxY, int maxX) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(colorPair) | A_DIM);
    } else {
        wattron(win, A_DIM);
    }

    int x = x1;
    int y = y1;
    while (x != x2) {
        x += (x2 > x) ? 1 : -1;
        if (y > 0 && y < maxY - 1 && x > 0 && x < maxX - 1) {
            mvwaddch(win, y, x, ACS_HLINE);
        }
    }
    while (y != y2) {
        y += (y2 > y) ? 1 : -1;
        if (y > 0 && y < maxY - 1 && x > 0 && x < maxX - 1) {
            mvwaddch(win, y, x, ACS_VLINE);
        }
    }

    if (hasColor) {
        wattroff(win, COLOR_PAIR(colorPair) | A_DIM);
    } else {
        wattroff(win, A_DIM);
    }
}

void drawTokenString(WINDOW* boardWin,
                     int y,
                     int x,
                     const std::vector<Player>& players,
                     const std::vector<int>& tokenIndices,
                     bool hasColor) {
    const int maxTokens = tokenIndices.size() > 3 ? 2 : static_cast<int>(tokenIndices.size());
    std::string tokenText = "[";
    for (int i = 0; i < maxTokens; ++i) {
        tokenText.push_back(players[static_cast<std::size_t>(tokenIndices[static_cast<std::size_t>(i)])].token);
    }
    if (tokenIndices.size() > 3) {
        tokenText.push_back('+');
    } else if (tokenIndices.size() == 3) {
        tokenText.push_back(players[static_cast<std::size_t>(tokenIndices[2])].token);
    } else if (tokenIndices.empty()) {
        tokenText.push_back(' ');
    }
    tokenText.push_back(']');

    const int drawX = x + std::max(0, ((TILE_W - 2) - static_cast<int>(tokenText.size())) / 2);
    mvwprintw(boardWin, y, drawX, "%s", tokenText.c_str());

    for (std::size_t i = 0; i < tokenIndices.size(); ++i) {
        if (i >= 3) {
            break;
        }
        const int tokenOffset = drawX + 1 + static_cast<int>(i);
        if (hasColor) {
            wattron(boardWin, COLOR_PAIR(PLAYER_PAIR_BASE + (tokenIndices[i] % 4)) | A_BOLD);
        } else {
            wattron(boardWin, A_BOLD);
        }
        mvwaddch(boardWin, y, tokenOffset, players[static_cast<std::size_t>(tokenIndices[i])].token);
        if (hasColor) {
            wattroff(boardWin, COLOR_PAIR(PLAYER_PAIR_BASE + (tokenIndices[i] % 4)) | A_BOLD);
        } else {
            wattroff(boardWin, A_BOLD);
        }
    }
}

void drawTileBox(WINDOW* boardWin,
                 const Tile& tile,
                 const std::vector<Player>& players,
                 int tileLeft,
                 int tileTop,
                 bool isFocusTile,
                 bool hasColor) {
    const int borderPair = isFocusTile ? 8 : 2;
    const int textPair = isFocusTile ? 8 : (tile.kind == TILE_EMPTY ? 6 : tileColorPair(tile));
    const int attrs = isFocusTile ? A_BOLD : A_DIM;

    if (hasColor) {
        wattron(boardWin, COLOR_PAIR(borderPair) | attrs);
    } else {
        wattron(boardWin, attrs);
    }

    mvwaddch(boardWin, tileTop, tileLeft, ACS_ULCORNER);
    mvwhline(boardWin, tileTop, tileLeft + 1, ACS_HLINE, TILE_W - 2);
    mvwaddch(boardWin, tileTop, tileLeft + TILE_W - 1, ACS_URCORNER);
    mvwvline(boardWin, tileTop + 1, tileLeft, ACS_VLINE, TILE_H - 2);
    mvwvline(boardWin, tileTop + 1, tileLeft + TILE_W - 1, ACS_VLINE, TILE_H - 2);
    mvwaddch(boardWin, tileTop + TILE_H - 1, tileLeft, ACS_LLCORNER);
    mvwhline(boardWin, tileTop + TILE_H - 1, tileLeft + 1, ACS_HLINE, TILE_W - 2);
    mvwaddch(boardWin, tileTop + TILE_H - 1, tileLeft + TILE_W - 1, ACS_LRCORNER);

    if (hasColor) {
        wattroff(boardWin, COLOR_PAIR(borderPair) | attrs);
    } else {
        wattroff(boardWin, attrs);
    }

    const std::string title = clipText(tileTitle(tile), TILE_W - 2);
    const std::string caption = tileCaption(tile);
    const std::vector<int> tilePlayers = playersAtTile(players, tile.id);

    if (hasColor) {
        wattron(boardWin, COLOR_PAIR(textPair) | attrs);
    } else {
        wattron(boardWin, attrs);
    }
    mvwprintw(boardWin, tileTop + 1, centeredX(tileLeft + 1, TILE_W - 2, static_cast<int>(title.size())), "%s", title.c_str());
    if (hasColor) {
        wattroff(boardWin, COLOR_PAIR(textPair) | attrs);
    } else {
        wattroff(boardWin, attrs);
    }

    drawTokenString(boardWin, tileTop + 3, tileLeft + 1, players, tilePlayers, hasColor);

    if (hasColor) {
        wattron(boardWin, COLOR_PAIR(textPair) | attrs);
    } else {
        wattron(boardWin, attrs);
    }
    mvwprintw(boardWin, tileTop + TILE_H - 2,
              centeredX(tileLeft + 1, TILE_W - 2, static_cast<int>(caption.size())),
              "%s",
              caption.c_str());
    if (hasColor) {
        wattroff(boardWin, COLOR_PAIR(textPair) | attrs);
    } else {
        wattroff(boardWin, attrs);
    }
}

}  // namespace

Board::Board() {
    initTiles();
    initRegions();
}

const Tile& Board::tileAt(int id) const {
    return tiles[id];
}

void Board::initTiles() {
    tiles.resize(TILE_COUNT);
    for (int i = 0; i < TILE_COUNT; ++i) {
        tiles[i].id = i;
        tiles[i].y = 0;
        tiles[i].x = 0;
        tiles[i].label = "  ";
        tiles[i].kind = TILE_EMPTY;
        tiles[i].next = (i < TILE_COUNT - 1) ? i + 1 : -1;
        tiles[i].altNext = -1;
        tiles[i].value = 0;
        tiles[i].stop = false;
    }

    const auto place = [&](int id, int col, int row) {
        tiles[id].x = col;
        tiles[id].y = row;
    };

    place(0, 1, 0);

    place(1, 9, 5);
    place(2, 8, 5);
    place(3, 7, 5);
    place(4, 6, 5);
    place(5, 5, 5);
    place(6, 5, 4);
    place(7, 5, 3);
    place(8, 5, 2);
    place(9, 6, 2);
    place(10, 7, 2);
    place(11, 8, 2);
    place(12, 9, 2);

    place(13, 0, 1);
    place(14, 0, 2);
    place(15, 0, 3);
    place(16, 0, 4);
    place(17, 1, 4);
    place(18, 2, 4);
    place(19, 3, 4);
    place(20, 4, 4);
    place(21, 5, 4);
    place(22, 6, 4);
    place(23, 7, 4);
    place(24, 8, 4);

    place(25, 2, 1);
    place(26, 3, 1);
    place(27, 4, 1);
    place(28, 5, 1);
    place(29, 6, 1);
    place(30, 7, 1);
    place(31, 8, 1);
    place(32, 9, 1);
    place(33, 10, 1);
    place(34, 10, 2);
    place(35, 10, 3);
    place(36, 10, 4);
    place(37, 9, 4);

    place(38, 4, 8);
    place(39, 3, 6);
    place(40, 4, 6);
    place(41, 6, 9);
    place(42, 7, 9);
    place(43, 8, 9);
    place(44, 11, 4);
    place(45, 11, 5);
    place(46, 11, 6);
    place(47, 11, 7);
    place(48, 11, 8);
    place(49, 11, 9);
    place(50, 11, 10);
    place(51, 11, 11);
    place(52, 11, 12);
    place(53, 10, 12);
    place(54, 9, 12);
    place(55, 8, 12);
    place(56, 7, 12);
    place(57, 6, 12);
    place(58, 5, 12);

    place(59, 10, 3);
    place(60, 9, 3);
    place(61, 8, 3);
    place(62, 7, 3);
    place(63, 7, 4);
    place(64, 7, 5);
    place(65, 7, 6);
    place(66, 7, 7);
    place(67, 7, 8);
    place(68, 7, 9);

    place(69, 10, 2);
    place(70, 9, 2);
    place(71, 8, 2);
    place(72, 7, 2);
    place(73, 6, 2);
    place(74, 5, 2);
    place(75, 5, 3);
    place(76, 5, 4);
    place(77, 5, 5);
    place(78, 5, 6);

    place(79, 6, 9);
    place(80, 0, 10);
    place(81, 0, 11);
    place(82, 0, 12);
    place(83, 1, 11);
    place(84, 1, 12);
    place(85, 1, 13);
    place(86, 3, 10);
    place(87, 4, 10);
    place(88, 4, 10);

    for (int i = 1; i <= 12; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }

    tiles[0].label = "ST";
    tiles[0].kind = TILE_START;
    tiles[0].next = 13;
    tiles[0].altNext = 25;

    tiles[13].label = "O";
    tiles[13].kind = TILE_COLLEGE;
    tiles[13].stop = true;
    for (int i = 14; i <= 24; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 1;
    }
    tiles[22].label = "PD";
    tiles[22].kind = TILE_PAYDAY;
    tiles[22].value = 5000;
    tiles[24].next = 37;

    tiles[25].label = "A";
    tiles[25].kind = TILE_CAREER;
    tiles[25].stop = true;
    for (int i = 26; i <= 37; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 1;
    }
    tiles[31].label = "PD";
    tiles[31].kind = TILE_PAYDAY;
    tiles[31].value = 7000;
    tiles[37].label = "GR";
    tiles[37].kind = TILE_GRADUATION;
    tiles[37].stop = true;
    tiles[37].next = 1;
    tiles[36].next = 1;

    tiles[38].label = "BK";
    tiles[38].kind = TILE_BLACK;
    tiles[38].value = 2;
    tiles[38].stop = false;
    tiles[38].next = 39;

    for (int i = 39; i <= 58; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[12].next = 58;
    tiles[41].label = "PD";
    tiles[41].kind = TILE_PAYDAY;
    tiles[41].value = 12000;
    tiles[9].label = "M";
    tiles[9].kind = TILE_MARRIAGE;
    tiles[9].stop = true;
    tiles[47].label = "PD";
    tiles[47].kind = TILE_PAYDAY;
    tiles[47].value = 15000;
    tiles[55].label = "PD";
    tiles[55].kind = TILE_PAYDAY;
    tiles[55].value = 18000;
    tiles[12].label = "FW";
    tiles[12].kind = TILE_SPLIT_FAMILY;
    tiles[12].stop = true;
    tiles[12].next = 59;
    tiles[12].altNext = 69;

    for (int i = 59; i <= 68; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[59].label = "F";
    tiles[59].kind = TILE_FAMILY;
    tiles[59].stop = true;
    tiles[64].label = "PD";
    tiles[64].kind = TILE_PAYDAY;
    tiles[64].value = 20000;
    tiles[68].next = 79;

    for (int i = 69; i <= 78; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[69].label = "W";
    tiles[69].kind = TILE_CAREER_2;
    tiles[69].value = 10000;
    tiles[75].label = "PD";
    tiles[75].kind = TILE_PAYDAY;
    tiles[75].value = 22000;
    tiles[78].next = 79;

    tiles[79].label = "SR";
    tiles[79].kind = TILE_SPLIT_RISK;
    tiles[79].stop = true;
    tiles[79].next = 80;
    tiles[79].altNext = 83;

    tiles[80].label = "S";
    tiles[80].kind = TILE_SAFE;
    tiles[80].stop = true;
    tiles[81].label = "BK";
    tiles[81].kind = TILE_BLACK;
    tiles[81].value = 2;
    tiles[82].label = "BK";
    tiles[82].kind = TILE_BLACK;
    tiles[82].value = 2;
    tiles[82].next = 86;

    tiles[83].label = "R";
    tiles[83].kind = TILE_RISKY;
    tiles[83].stop = true;
    tiles[84].label = "BK";
    tiles[84].kind = TILE_BLACK;
    tiles[84].value = 3;
    tiles[85].label = "BK";
    tiles[85].kind = TILE_BLACK;
    tiles[85].value = 3;
    tiles[85].next = 86;

    tiles[86].label = "BK";
    tiles[86].kind = TILE_BLACK;
    tiles[86].value = 3;
    tiles[86].next = 87;

    tiles[87].label = "RET";
    tiles[87].kind = TILE_RETIREMENT;
    tiles[87].stop = true;
    tiles[87].next = -1;

    tiles[88].label = "RET";
    tiles[88].kind = TILE_RETIREMENT;
    tiles[88].stop = true;
    tiles[88].next = -1;
}

bool Board::isStopSpace(const Tile& tile) const {
    return tile.stop;
}

std::string Board::regionNameForTile(int tileIndex) const {
    for (std::size_t i = 0; i < regions.size(); ++i) {
        if (tileIndex >= regions[i].startTileIndex && tileIndex <= regions[i].endTileIndex) {
            return regions[i].name;
        }
    }
    return "Open Road";
}

std::vector<std::string> Board::tutorialLegend() const {
    std::vector<std::string> lines;
    lines.push_back("[" + getTileBoardSymbol(tileAt(10)) + "] " + getTileDisplayName(tileAt(10)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(12)) + "] " + getTileDisplayName(tileAt(12)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(13)) + "] " + getTileDisplayName(tileAt(13)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(25)) + "] " + getTileDisplayName(tileAt(25)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(20)) + "] " + getTileDisplayName(tileAt(20)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(39)) + "] " + getTileDisplayName(tileAt(39)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(84)) + "] " + getTileDisplayName(tileAt(84)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(68)) + "] " + getTileDisplayName(tileAt(68)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(83)) + "] " + getTileDisplayName(tileAt(83)));
    lines.push_back("[" + getTileBoardSymbol(tileAt(0)) + "] " + getTileDisplayName(tileAt(0)));
    lines.push_back("[2P] Multiple players on one tile");
    return lines;
}

void Board::initRegions() {
    regions.clear();
    regions.push_back({"Startup Street", 0, 12});
    regions.push_back({"Career City", 13, 38});
    regions.push_back({"Goldrush Valley", 39, 58});
    regions.push_back({"Family Avenue", 59, 72});
    regions.push_back({"Risky Road", 73, 86});
    regions.push_back({"Retirement Ridge", 87, 88});
}
void Board::render(WINDOW* boardWin,
                   const std::vector<Player>& players,
                   int focusPlayerIndex,
                   int highlightedTile,
                   bool hasColor) const {
    werase(boardWin);
    box(boardWin, 0, 0);

    if (players.empty()) {
        mvwprintw(boardWin, 1, 2, "Board view unavailable.");
        wrefresh(boardWin);
        return;
    }

    const int focusIndex = std::max(0, std::min(focusPlayerIndex, static_cast<int>(players.size()) - 1));
    const int centerTile = players[static_cast<std::size_t>(focusIndex)].tile;
    const int focusTile = highlightedTile >= 0 ? highlightedTile : centerTile;
    const Tile& center = tileAt(centerTile);
    const int maxY = getmaxy(boardWin);
    const int maxX = getmaxx(boardWin);

    const std::string title =
        " " + players[static_cast<std::size_t>(focusIndex)].name + "'s view ";
    mvwprintw(boardWin, 0, 3, "%s", clipText(title, std::max(0, maxX - 6)).c_str());

    const std::string statusLine =
        players[static_cast<std::size_t>(focusIndex)].name + " at Space " +
        std::to_string(centerTile) + " - " + getTileDisplayName(tileAt(centerTile));
    mvwprintw(boardWin, 1, centeredX(1, maxX - 2, static_cast<int>(clipText(statusLine, maxX - 4).size())),
              "%s", clipText(statusLine, maxX - 4).c_str());

    const int viewportWidth = (VIEW_COLS * TILE_W) + ((VIEW_COLS - 1) * TILE_GAP_X) + 2;
    const int viewportHeight = (VIEW_ROWS * TILE_H) + ((VIEW_ROWS - 1) * TILE_GAP_Y) + 2;
    const int viewportLeft = std::max(1, (maxX - viewportWidth) / 2);
    const int viewportTop = 3;
    const int viewportRight = viewportLeft + viewportWidth - 1;
    const int viewportBottom = std::min(maxY - 2, viewportTop + viewportHeight - 1);

    if (hasColor) {
        wattron(boardWin, COLOR_PAIR(8) | A_BOLD);
    }
    mvwaddch(boardWin, viewportTop, viewportLeft, ACS_ULCORNER);
    mvwhline(boardWin, viewportTop, viewportLeft + 1, ACS_HLINE, viewportWidth - 2);
    mvwaddch(boardWin, viewportTop, viewportRight, ACS_URCORNER);
    mvwvline(boardWin, viewportTop + 1, viewportLeft, ACS_VLINE, viewportHeight - 2);
    mvwvline(boardWin, viewportTop + 1, viewportRight, ACS_VLINE, viewportHeight - 2);
    mvwaddch(boardWin, viewportBottom, viewportLeft, ACS_LLCORNER);
    mvwhline(boardWin, viewportBottom, viewportLeft + 1, ACS_HLINE, viewportWidth - 2);
    mvwaddch(boardWin, viewportBottom, viewportRight, ACS_LRCORNER);
    if (hasColor) {
        wattroff(boardWin, COLOR_PAIR(8) | A_BOLD);
    }

    const int tileStartLeft = viewportLeft + 1;
    const int tileStartTop = viewportTop + 1;
    const int centerCol = center.x;
    const int centerRow = center.y;
    const std::vector<std::pair<int, int> > connections = boardConnections(tiles);
    const std::set<int> reachable = reachableTiles(tiles);
    const std::set<int> visibleTrail =
        buildVisibleTrail(tiles, players[static_cast<std::size_t>(focusIndex)], focusTile);

    for (std::size_t i = 0; i < connections.size(); ++i) {
        if (reachable.count(connections[i].first) == 0 || reachable.count(connections[i].second) == 0) {
            continue;
        }
        if (visibleTrail.count(connections[i].first) == 0 || visibleTrail.count(connections[i].second) == 0) {
            continue;
        }
        const Tile& from = tiles[static_cast<std::size_t>(connections[i].first)];
        const Tile& to = tiles[static_cast<std::size_t>(connections[i].second)];
        const int fromDeltaCol = from.x - centerCol;
        const int fromDeltaRow = from.y - centerRow;
        const int toDeltaCol = to.x - centerCol;
        const int toDeltaRow = to.y - centerRow;
        if (fromDeltaCol < -2 || fromDeltaCol > 2 || fromDeltaRow < -1 || fromDeltaRow > 1 ||
            toDeltaCol < -2 || toDeltaCol > 2 || toDeltaRow < -1 || toDeltaRow > 1) {
            continue;
        }

        const int fromLeft = tileStartLeft + (fromDeltaCol + 2) * (TILE_W + TILE_GAP_X);
        const int fromTop = tileStartTop + (fromDeltaRow + 1) * (TILE_H + TILE_GAP_Y);
        const int toLeft = tileStartLeft + (toDeltaCol + 2) * (TILE_W + TILE_GAP_X);
        const int toTop = tileStartTop + (toDeltaRow + 1) * (TILE_H + TILE_GAP_Y);
        int startY = fromTop + (TILE_H / 2);
        int startX = fromLeft + (TILE_W / 2);
        int endY = toTop + (TILE_H / 2);
        int endX = toLeft + (TILE_W / 2);

        if (to.x > from.x) {
            startX = fromLeft + TILE_W - 1;
            endX = toLeft;
        } else if (to.x < from.x) {
            startX = fromLeft;
            endX = toLeft + TILE_W - 1;
        }

        if (to.y > from.y) {
            startY = fromTop + TILE_H - 1;
            endY = toTop;
        } else if (to.y < from.y) {
            startY = fromTop;
            endY = toTop + TILE_H - 1;
        }

        drawLineSegment(boardWin,
                        startY,
                        startX,
                        endY,
                        endX,
                        hasColor,
                        2,
                        maxY,
                        maxX);
    }

    for (int i = 0; i < TILE_COUNT; ++i) {
        if (reachable.count(i) == 0) {
            continue;
        }
        if (visibleTrail.count(i) == 0) {
            continue;
        }
        const Tile& tile = tiles[static_cast<std::size_t>(i)];
        const int deltaCol = tile.x - centerCol;
        const int deltaRow = tile.y - centerRow;
        if (deltaCol < -2 || deltaCol > 2 || deltaRow < -1 || deltaRow > 1) {
            continue;
        }

        const int col = deltaCol + 2;
        const int row = deltaRow + 1;
        const int tileLeft = tileStartLeft + col * (TILE_W + TILE_GAP_X);
        const int tileTop = tileStartTop + row * (TILE_H + TILE_GAP_Y);
        drawTileBox(boardWin,
                    tile,
                    players,
                    tileLeft,
                    tileTop,
                    i == focusTile,
                    hasColor);
    }

    wrefresh(boardWin);
}
