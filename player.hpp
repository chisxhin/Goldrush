#pragma once

#include <string>

struct Player {
    std::string name;
    char token;
    int tile;
    int cash;
    std::string job;
    int salary;
    bool married;
    int kids;
    bool hasHouse;
    int houseValue;
    bool retired;
    int startChoice;
    int familyChoice;
};

char tokenForName(const std::string& name, int index);
int totalWorth(const Player& player);
