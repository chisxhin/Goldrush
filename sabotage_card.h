#pragma once

#include <string>
#include <vector>

enum class SabotageType {
    MoneyLoss,
    MovementPenalty,
    SkipTurn,
    StealCard,
    ForceMinigame,
    CareerPenalty,
    PropertyDamage,
    DebtIncrease,
    PositionSwap,
    ItemDisable
};

struct SabotageCard {
    std::string name;
    std::string description;
    SabotageType type;
    int costToUse;
    int successChance;
    int strengthLevel;
    bool requiresDiceRoll;
    bool triggersMinigame;
};

std::string sabotageTypeName(SabotageType type);
SabotageType sabotageTypeFromText(const std::string& text);
std::vector<SabotageCard> makeSabotageCards();
const SabotageCard& sabotageCardForType(SabotageType type);
const SabotageCard& sabotageCardByName(const std::string& name);
