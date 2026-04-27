#include "sabotage_card.h"

#include <stdexcept>

std::string sabotageTypeName(SabotageType type) {
    switch (type) {
        case SabotageType::MoneyLoss:
            return "Money Loss";
        case SabotageType::MovementPenalty:
            return "Movement Penalty";
        case SabotageType::SkipTurn:
            return "Skip Turn";
        case SabotageType::StealCard:
            return "Steal Action Card";
        case SabotageType::ForceMinigame:
            return "Forced Duel Minigame";
        case SabotageType::CareerPenalty:
            return "Career Sabotage";
        case SabotageType::PropertyDamage:
            return "Property Damage";
        case SabotageType::DebtIncrease:
            return "Debt Trap";
        case SabotageType::PositionSwap:
            return "Position Swap";
        case SabotageType::ItemDisable:
            return "Item Disable";
        default:
            return "Unknown Sabotage";
    }
}

SabotageType sabotageTypeFromText(const std::string& text) {
    if (text == "Money Loss" || text == "MoneyLoss") return SabotageType::MoneyLoss;
    if (text == "Movement Penalty" || text == "MovementPenalty") return SabotageType::MovementPenalty;
    if (text == "Skip Turn" || text == "SkipTurn") return SabotageType::SkipTurn;
    if (text == "Steal Action Card" || text == "StealCard") return SabotageType::StealCard;
    if (text == "Forced Duel Minigame" || text == "ForceMinigame") return SabotageType::ForceMinigame;
    if (text == "Career Sabotage" || text == "CareerPenalty") return SabotageType::CareerPenalty;
    if (text == "Property Damage" || text == "PropertyDamage") return SabotageType::PropertyDamage;
    if (text == "Debt Trap" || text == "DebtIncrease") return SabotageType::DebtIncrease;
    if (text == "Position Swap" || text == "PositionSwap") return SabotageType::PositionSwap;
    if (text == "Item Disable" || text == "ItemDisable") return SabotageType::ItemDisable;
    return SabotageType::MoneyLoss;
}

std::vector<SabotageCard> makeSabotageCards() {
    std::vector<SabotageCard> cards;
    cards.push_back({"Trap Tile",
                     "Place a trap on a board tile. The next rival to land there triggers a random penalty.",
                     SabotageType::MoneyLoss,
                     12000,
                     100,
                     2,
                     false,
                     false});
    cards.push_back({"Lawsuit",
                     "Both players roll. Higher attacker roll transfers money; otherwise the attacker pays.",
                     SabotageType::MoneyLoss,
                     15000,
                     70,
                     3,
                     true,
                     false});
    cards.push_back({"Traffic Jam",
                     "Target's next movement is reduced by half.",
                     SabotageType::MovementPenalty,
                     10000,
                     80,
                     2,
                     true,
                     false});
    cards.push_back({"Steal Action Card",
                     "Steal one random action card from the target.",
                     SabotageType::StealCard,
                     18000,
                     75,
                     2,
                     true,
                     false});
    cards.push_back({"Forced Duel Minigame",
                     "Start a duel challenge. Winner takes money from the loser.",
                     SabotageType::ForceMinigame,
                     22000,
                     70,
                     3,
                     true,
                     true});
    cards.push_back({"Career Sabotage",
                     "Reduce the target's salary by 25% for 3 turns.",
                     SabotageType::CareerPenalty,
                     24000,
                     70,
                     3,
                     true,
                     false});
    cards.push_back({"Property Damage",
                     "Force the target to pay repair costs. Insurance can reduce this.",
                     SabotageType::PropertyDamage,
                     26000,
                     70,
                     3,
                     true,
                     false});
    cards.push_back({"Position Swap",
                     "Swap board position with a target. Expensive and puts the attacker on cooldown.",
                     SabotageType::PositionSwap,
                     90000,
                     65,
                     5,
                     true,
                     false});
    cards.push_back({"Debt Trap",
                     "Force the target to take loan debt.",
                     SabotageType::DebtIncrease,
                     20000,
                     75,
                     3,
                     true,
                     false});
    cards.push_back({"Item Disable",
                     "Disable shields and insurance for a short time.",
                     SabotageType::ItemDisable,
                     16000,
                     75,
                     2,
                     true,
                     false});
    return cards;
}

const SabotageCard& sabotageCardForType(SabotageType type) {
    static const std::vector<SabotageCard> cards = makeSabotageCards();
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (cards[i].type == type) {
            return cards[i];
        }
    }
    throw std::runtime_error("Unknown sabotage card type");
}

const SabotageCard& sabotageCardByName(const std::string& name) {
    static const std::vector<SabotageCard> cards = makeSabotageCards();
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (cards[i].name == name) {
            return cards[i];
        }
    }
    throw std::runtime_error("Unknown sabotage card name");
}
