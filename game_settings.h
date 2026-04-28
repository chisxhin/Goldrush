#pragma once

#include <string>

#include "rules.hpp"

struct GameSettings {
    std::string modeName = "Life Mode";
    bool customMode = false;

    int minJobSalary = 20000;
    int maxJobSalary = 120000;

    int collegeCost = 100000;

    int startingCash = 10000;
    int loanAmount = 50000;
    int loanPenalty = 60000;
    int maxLoans = 99;

    int paydayMultiplierPercent = 100;
    int taxMultiplierPercent = 100;
    int eventRewardMultiplierPercent = 100;
    int eventPenaltyMultiplierPercent = 100;

    int investmentMinReturnPercent = 100;
    int investmentMaxReturnPercent = 100;

    int babyCost = 0;
    int petCost = 0;
    int houseMinCost = 0;
    int houseMaxCost = 1000000;

    int minigameReward = 100;
    int minigamePenalty = 0;
    int minigameDifficultyModifier = 0;

    int sabotageUnlockTurn = 3;
    int sabotagePenaltyMultiplierPercent = 100;

    bool allowAutomaticLoans = true;
    bool allowSabotage = true;
    bool allowInvestments = true;
    bool allowPets = true;
    bool allowRandomEvents = true;
};

GameSettings createRelaxModeSettings();
GameSettings createLifeModeSettings();
GameSettings createHellModeSettings();
GameSettings createCustomSettingsFromMenu(bool hasColor);

void validateGameSettings(GameSettings& settings);
void applyGameSettingsToRules(const GameSettings& settings, RuleSet& rules);
//deleted
bool showCustomSettingsMenu(GameSettings& settings, bool hasColor);
std::string gameSettingsSummary(const GameSettings& settings);
