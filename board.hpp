#pragma once

#include <ncurses.h>
#include <string>
#include <vector>

#include "player.hpp"

static const int TILE_COUNT = 89;

enum TileKind {
    TILE_EMPTY,
    TILE_BLACK,
    TILE_START,
    TILE_SPLIT_START,
    TILE_COLLEGE,
    TILE_CAREER,
    TILE_GRADUATION,
    TILE_MARRIAGE,
    TILE_SPLIT_FAMILY,
    TILE_FAMILY,
    TILE_NIGHT_SCHOOL,
    TILE_SPLIT_RISK,
    TILE_SAFE,
    TILE_RISKY,
    TILE_SPIN_AGAIN,
    TILE_CAREER_2,
    TILE_PAYDAY,
    TILE_BABY,
    TILE_RETIREMENT,
    TILE_HOUSE
};

struct Tile {
    int id;
    int y;
    int x;
    std::string label;
    TileKind kind;
    int next;
    int altNext;
    int value;
    bool stop;
};

class Board {
public:
    Board();

    const Tile& tileAt(int id) const;
    bool isStopSpace(const Tile& tile) const;
    std::vector<std::string> tutorialLegend() const;
    void render(WINDOW* boardWin,
                const std::vector<Player>& players,
                int focusPlayerIndex,
                int highlightedTile,
                bool hasColor) const;

private:
    std::vector<Tile> tiles;

    void initTiles();
    int colorForTile(const Tile& tile) const;
};
