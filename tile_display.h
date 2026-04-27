#pragma once

#include <string>

#include "board.hpp"

std::string getTileFullName(TileKind kind);
std::string getTileFullName(const Tile& tile);
std::string getTileAbbreviation(TileKind kind);
std::string getTileAbbreviation(const Tile& tile);
std::string getTileBoardSymbol(TileKind kind);
std::string getTileBoardSymbol(const Tile& tile);
std::string getTileDisplayName(TileKind kind);
std::string getTileDisplayName(const Tile& tile);
