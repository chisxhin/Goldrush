#include "cpu_player.hpp"

#include <algorithm>

CpuController::CpuController(RandomService& random)
    : rng(random) {
}

int CpuController::difficultyRank(const Player& player) const {
    switch (player.cpuDifficulty) {
        case CpuDifficulty::Easy:
            return 0;
        case CpuDifficulty::Hard:
            return 2;
        case CpuDifficulty::Normal:
        default:
            return 1;
    }
}

bool CpuController::chance(int percent) {
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }
    return rng.uniformInt(1, 100) <= percent;
}

int CpuController::chooseStartRoute(const Player& player, const RuleSet& rules) {
    (void)rules;
    const int rank = difficultyRank(player);
    if (rank == 0) {
        return chance(45) ? 0 : 1;
    }
    if (rank == 2) {
        return (player.cash >= 25000 || player.salary <= 0) ? 0 : 1;
    }
    return player.cash >= 50000 ? 0 : 1;
}

int CpuController::chooseFamilyRoute(const Player& player, const RuleSet& rules) {
    if (!rules.toggles.familyPathEnabled) {
        return 1;
    }
    const int rank = difficultyRank(player);
    if (rank == 0) {
        return chance(50) ? 0 : 1;
    }
    if (rank == 2) {
        return player.kids == 0 && player.cash >= 25000 ? 0 : 1;
    }
    return player.kids <= 1 ? 0 : 1;
}

int CpuController::chooseRiskRoute(const Player& player, const RuleSet& rules) {
    if (!rules.toggles.riskyRouteEnabled) {
        return 0;
    }
    const int rank = difficultyRank(player);
    if (rank == 0) {
        return chance(35) ? 1 : 0;
    }
    if (rank == 2) {
        return player.cash >= 80000 && player.loans <= 1 ? 1 : 0;
    }
    return player.cash >= 120000 ? 1 : 0;
}

int CpuController::chooseNightSchool(const Player& player, const RuleSet& rules) {
    if (!rules.toggles.nightSchoolEnabled || player.usedNightSchool) {
        return 1;
    }
    const int rank = difficultyRank(player);
    if (rank == 0) {
        return chance(25) ? 0 : 1;
    }
    if (rank == 2) {
        return player.salary < 70000 && player.cash >= 40000 ? 0 : 1;
    }
    return player.salary < 55000 && player.cash >= 70000 ? 0 : 1;
}

int CpuController::chooseRetirement(const Player& player) {
    const int rank = difficultyRank(player);
    if (rank == 0) {
        return chance(50) ? 0 : 1;
    }
    return totalWorth(player) >= 750000 ? 0 : 1;
}

int CpuController::chooseCareer(const Player& player, const std::vector<CareerCard>& choices) {
    if (choices.empty()) {
        return 0;
    }
    const int rank = difficultyRank(player);
    if (rank == 0 && choices.size() > 1) {
        return rng.uniformInt(0, static_cast<int>(choices.size()) - 1);
    }

    int bestIndex = 0;
    for (std::size_t i = 1; i < choices.size(); ++i) {
        if (choices[i].salary > choices[static_cast<std::size_t>(bestIndex)].salary) {
            bestIndex = static_cast<int>(i);
        }
    }

    if (rank == 1 && choices.size() > 1 && chance(20)) {
        return rng.uniformInt(0, static_cast<int>(choices.size()) - 1);
    }
    return bestIndex;
}

CpuMinigameResult CpuController::playBlackTileMinigame(const Player& player, int minigameChoice) {
    const int rank = difficultyRank(player);
    const int baseChance = rank == 0 ? 35 : (rank == 1 ? 55 : 75);
    const int scoreCap = rank == 0 ? 4 : (rank == 1 ? 7 : 10);
    CpuMinigameResult result;
    result.success = chance(baseChance);
    result.score = result.success
        ? rng.uniformInt(std::max(1, scoreCap / 2), scoreCap)
        : rng.uniformInt(0, std::max(1, scoreCap / 2));

    switch (minigameChoice) {
        case 0:
            result.summary = "Pong returns: " + std::to_string(result.score);
            break;
        case 1:
            result.summary = "Battleship ships destroyed: " + std::to_string(result.score);
            break;
        case 2:
            result.summary = "Hangman letters revealed: " + std::to_string(result.score);
            break;
        case 3:
            result.summary = "Memory pairs matched: " + std::to_string(std::min(result.score, 8));
            result.score = std::min(result.score, 8);
            break;
        case 4:
        default:
            result.summary = "Minesweeper safe tiles: " + std::to_string(std::min(result.score + 2, 15));
            result.score = std::min(result.score + 2, 15);
            break;
    }
    return result;
}
