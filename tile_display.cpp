#include "tile_display.h"

std::string getTileFullName(TileKind kind) {
    switch (kind) {
        case TILE_BLACK: return "Action / Minigame";
        case TILE_START: return "Start";
        case TILE_SPLIT_START: return "College or Career Split";
        case TILE_COLLEGE: return "College Route";
        case TILE_CAREER: return "Career Search";
        case TILE_GRADUATION: return "Graduation";
        case TILE_MARRIAGE: return "Get Married";
        case TILE_SPLIT_FAMILY: return "Family or Life Split";
        case TILE_FAMILY: return "Family Path";
        case TILE_NIGHT_SCHOOL: return "Night School";
        case TILE_SPLIT_RISK: return "Safe or Risky Split";
        case TILE_SAFE: return "Safe Route";
        case TILE_RISKY: return "Risky Route";
        case TILE_SPIN_AGAIN: return "Spin Again";
        case TILE_CAREER_2: return "Promotion";
        case TILE_PAYDAY: return "Salary Payday";
        case TILE_BABY: return "Baby Event";
        case TILE_RETIREMENT: return "Retirement";
        case TILE_HOUSE: return "House Purchase";
        case TILE_EMPTY:
        default:
            return "Open Road";
    }
}

std::string getTileAbbreviation(TileKind kind) {
    switch (kind) {
        case TILE_BLACK: return "BK";
        case TILE_START: return "ST";
        case TILE_SPLIT_START: return "SP";
        case TILE_COLLEGE: return "CO";
        case TILE_CAREER: return "CR";
        case TILE_GRADUATION: return "GR";
        case TILE_MARRIAGE: return "GM";
        case TILE_SPLIT_FAMILY: return "FM";
        case TILE_FAMILY: return "FP";
        case TILE_NIGHT_SCHOOL: return "NS";
        case TILE_SPLIT_RISK: return "RS";
        case TILE_SAFE: return "SF";
        case TILE_RISKY: return "RK";
        case TILE_SPIN_AGAIN: return "SG";
        case TILE_CAREER_2: return "PR";
        case TILE_PAYDAY: return "PD";
        case TILE_BABY: return "BB";
        case TILE_RETIREMENT: return "RT";
        case TILE_HOUSE: return "HS";
        case TILE_EMPTY:
        default:
            return "--";
    }
}

std::string getTileAbbreviation(const Tile& tile) {
    if (!tile.label.empty()) {
        return tile.label;
    }
    return getTileAbbreviation(tile.kind);
}

std::string getTileDisplayName(TileKind kind) {
    return getTileFullName(kind) + " (" + getTileAbbreviation(kind) + ")";
}

std::string getTileDisplayName(const Tile& tile) {
    return getTileFullName(tile.kind) + " (" + getTileAbbreviation(tile) + ")";
}
