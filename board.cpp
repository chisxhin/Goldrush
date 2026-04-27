#include "board.hpp"
#include "tile_display.h"

namespace {
struct UiPos {
    int x;
    int y;
};

static const UiPos board_ui_positions[TILE_COUNT] = {
    {17, 1}, {21, 1}, {25, 1}, {29, 1}, {33, 1}, {37, 1}, {41, 1}, {45, 1}, {49, 1}, {53, 1}, {57, 1}, {61, 1}, {39, 4},
    {3, 7}, {7, 7}, {11, 7}, {15, 7}, {19, 7}, {23, 7}, {27, 7}, {31, 7}, {35, 7}, {39, 7}, {43, 7}, {47, 7},
    {25, 10}, {29, 10}, {33, 10}, {37, 10}, {41, 10}, {45, 10}, {49, 10}, {53, 10}, {57, 10}, {61, 10}, {65, 10}, {69, 10}, {73, 10},
    {19, 13}, {23, 13}, {27, 13}, {31, 13}, {35, 13}, {39, 13}, {43, 13}, {47, 13}, {51, 13}, {55, 13}, {59, 13},
    {21, 16}, {25, 16}, {29, 16}, {33, 16}, {37, 16}, {41, 16}, {45, 16}, {49, 16}, {53, 16}, {57, 16},
    {3, 19}, {7, 19}, {11, 19}, {15, 19}, {19, 19}, {23, 19}, {27, 19}, {31, 19}, {35, 19}, {39, 19}, {43, 19}, {47, 19}, {51, 19}, {55, 19},
    {23, 22}, {27, 22}, {31, 22}, {35, 22}, {39, 22}, {43, 22}, {47, 22}, {51, 22}, {55, 22}, {59, 22}, {63, 22}, {67, 22}, {71, 22}, {75, 22},
    {37, 25}, {41, 25}
};

struct RowSegment {
    int y;
    int startIndex;
    int count;
};

static const RowSegment row_segments[] = {
    {1, 0, 12},
    {4, 12, 1},
    {7, 13, 12},
    {10, 25, 13},
    {13, 38, 11},
    {16, 49, 10},
    {19, 59, 14},
    {22, 73, 14},
    {25, 87, 2}
};

const int PLAYER_PAIR_BASE = 9;
}

Board::Board() {
    initTiles();
}

const Tile& Board::tileAt(int id) const {
    return tiles[id];
}

void Board::initTiles() {
    tiles.resize(TILE_COUNT);
    for (int i = 0; i < TILE_COUNT; ++i) {
        tiles[i].id = i;
        tiles[i].y = board_ui_positions[i].y;
        tiles[i].x = board_ui_positions[i].x - 1;
        tiles[i].label = "  ";
        tiles[i].kind = TILE_EMPTY;
        tiles[i].next = (i < TILE_COUNT - 1) ? i + 1 : -1;
        tiles[i].altNext = -1;
        tiles[i].value = 0;
        tiles[i].stop = false;
    }

    for (int i = 0; i <= 9; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 1;
    }

    tiles[10].label = "ST";
    tiles[10].kind = TILE_START;

    tiles[11].label = "BK";
    tiles[11].kind = TILE_BLACK;
    tiles[11].value = 1;

    tiles[12].label = "SP";
    tiles[12].kind = TILE_SPLIT_START;
    tiles[12].next = 13;
    tiles[12].altNext = 25;

    tiles[13].label = "CO";
    tiles[13].kind = TILE_COLLEGE;
    for (int i = 14; i <= 24; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = (i >= 20) ? 2 : 1;
    }
    tiles[20].label = "PD";
    tiles[20].kind = TILE_PAYDAY;
    tiles[20].value = 5000;
    tiles[24].next = 38;

    tiles[25].label = "CR";
    tiles[25].kind = TILE_CAREER;
    tiles[25].stop = true;
    for (int i = 26; i <= 37; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = (i >= 33) ? 2 : 1;
    }
    tiles[31].label = "PD";
    tiles[31].kind = TILE_PAYDAY;
    tiles[31].value = 7000;
    tiles[37].next = 38;

    tiles[38].label = "GR";
    tiles[38].kind = TILE_GRADUATION;
    tiles[38].stop = true;
    for (int i = 39; i <= 48; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[41].label = "PD";
    tiles[41].kind = TILE_PAYDAY;
    tiles[41].value = 10000;
    tiles[44].label = "GM";
    tiles[44].kind = TILE_MARRIAGE;
    tiles[44].stop = true;
    tiles[47].label = "PD";
    tiles[47].kind = TILE_PAYDAY;
    tiles[47].value = 15000;

    for (int i = 49; i <= 57; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[50].label = "FM";
    tiles[50].kind = TILE_FAMILY;
    tiles[50].stop = true;
    tiles[52].label = "NS";
    tiles[52].kind = TILE_NIGHT_SCHOOL;
    tiles[52].stop = true;
    tiles[55].label = "PD";
    tiles[55].kind = TILE_PAYDAY;
    tiles[55].value = 12000;
    tiles[58].label = "SP";
    tiles[58].kind = TILE_SPLIT_FAMILY;
    tiles[58].next = 59;
    tiles[58].altNext = 73;

    for (int i = 59; i <= 72; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[60].label = "3B";
    tiles[60].kind = TILE_BABY;
    tiles[60].stop = true;
    tiles[64].label = "2B";
    tiles[64].kind = TILE_BABY;
    tiles[64].stop = true;
    tiles[68].label = "HS";
    tiles[68].kind = TILE_HOUSE;
    tiles[68].stop = true;
    tiles[70].label = "PD";
    tiles[70].kind = TILE_PAYDAY;
    tiles[70].value = 18000;
    tiles[72].label = "1B";
    tiles[72].kind = TILE_BABY;
    tiles[72].stop = true;
    tiles[72].next = 87;

    for (int i = 73; i <= 86; ++i) {
        tiles[i].label = "BK";
        tiles[i].kind = TILE_BLACK;
        tiles[i].value = 2;
    }
    tiles[75].label = "PD";
    tiles[75].kind = TILE_PAYDAY;
    tiles[75].value = 20000;
    tiles[76].label = "PR";
    tiles[76].kind = TILE_CAREER_2;
    tiles[76].value = 10000;
    tiles[79].label = "RS";
    tiles[79].kind = TILE_SPLIT_RISK;
    tiles[79].stop = true;
    tiles[79].next = 80;
    tiles[79].altNext = 83;
    tiles[80].label = "SF";
    tiles[80].kind = TILE_SAFE;
    tiles[80].stop = true;
    tiles[81].label = "PD";
    tiles[81].kind = TILE_PAYDAY;
    tiles[81].value = 25000;
    tiles[82].label = "SG";
    tiles[82].kind = TILE_SPIN_AGAIN;
    tiles[82].stop = true;
    tiles[82].next = 86;
    tiles[83].label = "RK";
    tiles[83].kind = TILE_RISKY;
    tiles[83].stop = true;
    tiles[84].label = "BK";
    tiles[84].kind = TILE_BLACK;
    tiles[84].value = 3;
    tiles[85].label = "RK";
    tiles[85].kind = TILE_RISKY;
    tiles[85].stop = true;
    tiles[85].next = 86;
    tiles[86].label = "PD";
    tiles[86].kind = TILE_PAYDAY;
    tiles[86].value = 30000;
    tiles[86].next = 87;

    tiles[87].label = "MM";
    tiles[87].kind = TILE_RETIREMENT;
    tiles[87].stop = true;
    tiles[87].next = -1;

    tiles[88].label = "CA";
    tiles[88].kind = TILE_RETIREMENT;
    tiles[88].stop = true;
    tiles[88].next = -1;
}

bool Board::isStopSpace(const Tile& tile) const {
    return tile.stop;
}

std::vector<std::string> Board::tutorialLegend() const {
    std::vector<std::string> lines;
    lines.push_back(getTileDisplayName(TILE_START));
    lines.push_back(getTileDisplayName(TILE_SPLIT_START));
    lines.push_back(getTileDisplayName(TILE_COLLEGE));
    lines.push_back(getTileDisplayName(TILE_CAREER));
    lines.push_back(getTileDisplayName(TILE_GRADUATION));
    lines.push_back(getTileDisplayName(TILE_MARRIAGE));
    lines.push_back(getTileDisplayName(TILE_SPLIT_FAMILY));
    lines.push_back(getTileDisplayName(TILE_NIGHT_SCHOOL));
    lines.push_back(getTileDisplayName(TILE_PAYDAY));
    lines.push_back(getTileDisplayName(TILE_CAREER_2));
    lines.push_back(getTileDisplayName(TILE_HOUSE));
    lines.push_back("Baby Event (3B/2B/1B)");
    lines.push_back(getTileDisplayName(TILE_SPLIT_RISK));
    lines.push_back(getTileDisplayName(TILE_SAFE));
    lines.push_back(getTileDisplayName(TILE_RISKY));
    lines.push_back(getTileDisplayName(TILE_SPIN_AGAIN));
    lines.push_back("Millionaire Mansion (MM)");
    lines.push_back("Countryside Acres (CA)");
    lines.push_back(getTileDisplayName(TILE_BLACK));
    return lines;
}

int Board::colorForTile(const Tile& tile) const {
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

void Board::drawBoardGrid(WINDOW* boardWin) const {
    wattron(boardWin, COLOR_PAIR(2));
    for (size_t i = 0; i < sizeof(row_segments) / sizeof(row_segments[0]); ++i) {
        const RowSegment& row = row_segments[i];
        const int left = board_ui_positions[row.startIndex].x - 1;
        const int width = row.count * 4 + 1;
        mvwhline(boardWin, row.y - 1, left, ACS_HLINE, width);
        mvwhline(boardWin, row.y + 1, left, ACS_HLINE, width);
        for (int col = 0; col <= row.count; ++col) {
            mvwaddch(boardWin, row.y, left + (col * 4), ACS_VLINE);
        }
        mvwaddch(boardWin, row.y - 1, left, ACS_ULCORNER);
        mvwaddch(boardWin, row.y + 1, left, ACS_LLCORNER);
        for (int col = 1; col < row.count; ++col) {
            mvwaddch(boardWin, row.y - 1, left + (col * 4), ACS_TTEE);
            mvwaddch(boardWin, row.y + 1, left + (col * 4), ACS_BTEE);
        }
        mvwaddch(boardWin, row.y - 1, left + width - 1, ACS_URCORNER);
        mvwaddch(boardWin, row.y + 1, left + width - 1, ACS_LRCORNER);
    }
    wattroff(boardWin, COLOR_PAIR(2));
}

void Board::drawTreeGuides(WINDOW* boardWin) const {
    wattron(boardWin, COLOR_PAIR(2) | A_BOLD);
    mvwvline(boardWin, 3, 40, ACS_VLINE, 1);
    mvwvline(boardWin, 5, 16, ACS_VLINE, 2);
    mvwhline(boardWin, 6, 17, ACS_HLINE, 22);
    mvwvline(boardWin, 5, 40, ACS_VLINE, 2);
    mvwvline(boardWin, 8, 40, ACS_VLINE, 1);
    mvwvline(boardWin, 11, 40, ACS_VLINE, 1);
    mvwvline(boardWin, 14, 40, ACS_VLINE, 1);
    mvwhline(boardWin, 15, 40, ACS_HLINE, 2);
    mvwvline(boardWin, 15, 22, ACS_VLINE, 2);
    mvwhline(boardWin, 18, 23, ACS_HLINE, 17);
    mvwvline(boardWin, 18, 58, ACS_VLINE, 4);
    mvwvline(boardWin, 23, 40, ACS_VLINE, 1);
    wattroff(boardWin, COLOR_PAIR(2) | A_BOLD);
}

void Board::drawTile(WINDOW* boardWin, const Tile& tile, bool hasColor) const {
    std::string label = tile.label;
    if (tile.kind == TILE_EMPTY) {
        label = "  ";
    }
    mvwaddch(boardWin, tile.y, tile.x, '[');
    if (hasColor) wattron(boardWin, COLOR_PAIR(colorForTile(tile)) | A_BOLD);
    mvwprintw(boardWin, tile.y, tile.x + 1, "%-2s", label.c_str());
    if (hasColor) wattroff(boardWin, COLOR_PAIR(colorForTile(tile)) | A_BOLD);
    mvwaddch(boardWin, tile.y, tile.x + 3, ']');
}

void Board::drawTokens(WINDOW* boardWin, const std::vector<Player>& players, int tileIndex, bool hasColor) const {
    int slot = 0;
    for (size_t p = 0; p < players.size(); ++p) {
        if (players[p].tile != tileIndex || slot >= 2) continue;
        if (hasColor) wattron(boardWin, COLOR_PAIR(PLAYER_PAIR_BASE + static_cast<int>(p % 4)) | A_BOLD);
        mvwaddch(boardWin, tiles[tileIndex].y, tiles[tileIndex].x + 1 + slot, players[p].token);
        if (hasColor) wattroff(boardWin, COLOR_PAIR(PLAYER_PAIR_BASE + static_cast<int>(p % 4)) | A_BOLD);
        ++slot;
    }
}

void Board::render(WINDOW* boardWin,
                   const std::vector<Player>& players,
                   bool hasColor) const {
    werase(boardWin);
    box(boardWin, 0, 0);
    drawBoardGrid(boardWin);
    drawTreeGuides(boardWin);

    for (int i = 0; i < TILE_COUNT; ++i) {
        drawTile(boardWin, tiles[i], hasColor);
    }
    for (int i = 0; i < TILE_COUNT; ++i) {
        drawTokens(boardWin, players, i, hasColor);
    }
    wrefresh(boardWin);
}
