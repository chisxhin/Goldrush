#pragma once

#include <string>
#include <vector>

enum class PlayerType {
    Human,
    CPU
};

enum class CpuDifficulty {
    Easy,
    Normal,
    Hard
};

struct Player {
    std::string name;
    char token = 'A';
    int tile = 0;
    int cash = 0;
    std::string job;
    int salary = 0;
    bool married = false;
    int kids = 0;
    bool collegeGraduate = false;
    bool usedNightSchool = false;
    bool hasHouse = false;
    std::string houseName;
    int houseValue = 0;
    int loans = 0;
    int investedNumber = 0;
    int investPayout = 0;
    int spinToWinTokens = 0;
    int retirementPlace = 0;
    int retirementBonus = 0;
    int finalHouseSaleValue = 0;
    std::string retirementHome;
    std::vector<std::string> actionCards;
    std::vector<std::string> petCards;
    bool retired = false;
    int startChoice = -1;
    int familyChoice = -1;
    int riskChoice = -1;
    PlayerType type = PlayerType::Human;
    CpuDifficulty cpuDifficulty = CpuDifficulty::Normal;
    int turnsTaken = 0;
    int sabotageDebt = 0;
    int shieldCards = 0;
    int insuranceUses = 0;
    bool skipNextTurn = false;
    int movementPenaltyTurns = 0;
    int movementPenaltyPercent = 0;
    int salaryReductionTurns = 0;
    int salaryReductionPercent = 0;
    int sabotageCooldown = 0;
    int itemDisableTurns = 0;
};

char tokenForName(const std::string& name, int index);
int totalWorth(const Player& player);
std::string playerTypeLabel(PlayerType type);
std::string cpuDifficultyLabel(CpuDifficulty difficulty);
PlayerType playerTypeFromText(const std::string& text);
CpuDifficulty cpuDifficultyFromText(const std::string& text);
