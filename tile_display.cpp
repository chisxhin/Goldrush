#include "tile_display.h"

std::string getTileFullName(TileKind kind) {
    switch (kind) {
        case TILE_BLACK: return "Action Tile";
        case TILE_START: return "Start";
        case TILE_SPLIT_START: return "Route Split";
        case TILE_COLLEGE: return "College";
        case TILE_CAREER: return "Job Search";
        case TILE_GRADUATION: return "Graduation Stop";
        case TILE_MARRIAGE: return "Marriage Stop";
        case TILE_SPLIT_FAMILY: return "Family Split";
        case TILE_FAMILY: return "Family Stop";
        case TILE_NIGHT_SCHOOL: return "Night School";
        case TILE_SPLIT_RISK: return "Risk Split";
        case TILE_SAFE: return "Safe Route";
        case TILE_RISKY: return "Risk Event";
        case TILE_SPIN_AGAIN: return "Spin Again";
        case TILE_CAREER_2: return "Work";
        case TILE_PAYDAY: return "Salary Payday";
        case TILE_BABY: return "Family Event";
        case TILE_RETIREMENT: return "Retirement";
        case TILE_HOUSE: return "House";
        case TILE_EMPTY:
        default:
            return "Open Road";
    }
}

std::string getTileFullName(const Tile& tile) {
    if (tile.kind == TILE_BLACK) {
        if (tile.value >= 3) {
            return "Minigame";
        }
        if (tile.value >= 2) {
            return "Action Tile";
        }
        return "Adventure Space";
    }
    if (tile.kind == TILE_BABY) {
        return "Family Event";
    }
    if (tile.kind == TILE_RETIREMENT && tile.label == "MM") {
        return "Millionaire Mansion";
    }
    if (tile.kind == TILE_RETIREMENT && tile.label == "CA") {
        return "Countryside Acres";
    }
    return getTileFullName(tile.kind);
}

std::string getTileAbbreviation(TileKind kind) {
    switch (kind) {
        case TILE_BLACK: return "A";
        case TILE_START: return "GO";
        case TILE_SPLIT_START: return "SP";
        case TILE_COLLEGE: return "CO";
        case TILE_CAREER: return "JB";
        case TILE_GRADUATION: return "GR";
        case TILE_MARRIAGE: return "GM";
        case TILE_SPLIT_FAMILY: return "FM";
        case TILE_FAMILY: return "FA";
        case TILE_NIGHT_SCHOOL: return "NS";
        case TILE_SPLIT_RISK: return "RS";
        case TILE_SAFE: return "SF";
        case TILE_RISKY: return "RK";
        case TILE_SPIN_AGAIN: return "SG";
        case TILE_CAREER_2: return "PR";
        case TILE_PAYDAY: return "PD";
        case TILE_BABY: return "FE";
        case TILE_RETIREMENT: return "RT";
        case TILE_HOUSE: return "HS";
        case TILE_EMPTY:
        default:
            return "--";
    }
}

std::string getTileAbbreviation(const Tile& tile) {
    if (tile.kind == TILE_BLACK) {
        return tile.value >= 3 ? "MG" : "A";
    }
    if (tile.kind != TILE_EMPTY && tile.kind != TILE_BLACK && !tile.label.empty()) {
        return tile.label;
    }
    if (tile.kind == TILE_BABY && !tile.label.empty()) {
        return tile.label;
    }
    if (tile.kind == TILE_RETIREMENT && !tile.label.empty()) {
        return tile.label;
    }
    return getTileAbbreviation(tile.kind);
}

std::string getTileBoardSymbol(TileKind kind) {
    switch (kind) {
        case TILE_BLACK: return "..";
        case TILE_START: return "GO";
        case TILE_SPLIT_START: return "<>";
        case TILE_COLLEGE: return "CO";
        case TILE_CAREER: return "JB";
        case TILE_GRADUATION: return "GR";
        case TILE_MARRIAGE: return "MR";
        case TILE_SPLIT_FAMILY: return "Y ";
        case TILE_FAMILY: return "FA";
        case TILE_NIGHT_SCHOOL: return "NS";
        case TILE_SPLIT_RISK: return "??";
        case TILE_SAFE: return "$ ";
        case TILE_RISKY: return "!!";
        case TILE_SPIN_AGAIN: return ">>";
        case TILE_CAREER_2: return "CR";
        case TILE_PAYDAY: return "$ ";
        case TILE_BABY: return "B ";
        case TILE_RETIREMENT: return "RT";
        case TILE_HOUSE: return "HS";
        case TILE_EMPTY:
        default:
            return "  ";
    }
}

std::string getTileBoardSymbol(const Tile& tile) {
    if (tile.kind == TILE_BLACK) {
        if (tile.value >= 3) {
            return "MG";
        }
        if (tile.value >= 2) {
            return "? ";
        }
        return (tile.id % 2 == 0) ? ".." : "--";
    }
    if (tile.kind == TILE_BABY && tile.label.size() >= 2) {
        return tile.label.substr(0, 2);
    }
    if (tile.kind == TILE_RETIREMENT && !tile.label.empty()) {
        return tile.label;
    }
    return getTileBoardSymbol(tile.kind);
}

std::string getTileDisplayName(TileKind kind) {
    return getTileFullName(kind) + " (" + getTileAbbreviation(kind) + ")";
}

std::string getTileDisplayName(const Tile& tile) {
    return getTileFullName(tile) + " (" + getTileAbbreviation(tile) + ")";
}
