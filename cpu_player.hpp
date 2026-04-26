#pragma once

#include <string>
#include <vector>

#include "cards.hpp"
#include "player.hpp"
#include "random_service.hpp"
#include "rules.hpp"

struct CpuMinigameResult {
    int score;
    bool success;
    std::string summary;
};

class CpuController {
public:
    explicit CpuController(RandomService& random);

    int chooseStartRoute(const Player& player, const RuleSet& rules);
    int chooseFamilyRoute(const Player& player, const RuleSet& rules);
    int chooseRiskRoute(const Player& player, const RuleSet& rules);
    int chooseNightSchool(const Player& player, const RuleSet& rules);
    int chooseRetirement(const Player& player);
    int chooseCareer(const Player& player, const std::vector<CareerCard>& choices);
    CpuMinigameResult playBlackTileMinigame(const Player& player, int minigameChoice);

private:
    RandomService& rng;

    int difficultyRank(const Player& player) const;
    bool chance(int percent);
};
