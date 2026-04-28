#pragma once

#include <string>
#include <vector>

struct TurnSummary {
    std::string playerName;
    int turnNumber = 0;

    int moneyChange = 0;
    int loanChange = 0;
    int babyChange = 0;
    int petChange = 0;
    int investmentChange = 0;
    int shieldChange = 0;
    int insuranceChange = 0;

    bool gotMarried = false;
    bool jobChanged = false;
    bool houseChanged = false;

    std::string oldJob;
    std::string newJob;
    std::string oldHouse;
    std::string newHouse;

    std::vector<std::string> cardsGained;
    std::vector<std::string> cardsUsed;
    std::vector<std::string> minigameResults;
    std::vector<std::string> sabotageEvents;
    std::vector<std::string> importantEvents;
};

std::string formatMoney(int amount);
std::string formatSignedChange(int amount, const std::string& unit);
void showTurnSummaryReport(const TurnSummary& summary, bool hasColor);
