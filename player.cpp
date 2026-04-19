#include "Player.hpp"

char tokenForName(const std::string& name, int index) {
    if (!name.empty()) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
        return c;
    }
    return static_cast<char>('A' + index);
}

int totalWorth(const Player& player) {
    int total = player.cash + player.kids * 20000;
    if (player.hasHouse) {
        total += player.houseValue;
    }
    return total;
}
