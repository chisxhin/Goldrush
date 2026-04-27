#pragma once

#include <string>
#include <vector>

#include "cards.hpp"
#include "player.hpp"
#include "random_service.hpp"
#include "rules.hpp"
#include "sabotage_card.h"

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
    bool shouldUseSabotage(const Player& player, int turnCounter);
    int chooseSabotageTarget(const Player& player,
                             const std::vector<Player>& players,
                             int selfIndex);
    SabotageType chooseSabotageType(const Player& player,
                                    const Player& target,
                                    int turnCounter);

private:
    RandomService& rng;

    int difficultyRank(const Player& player) const;
    bool chance(int percent);
};
