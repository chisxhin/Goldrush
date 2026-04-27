#include "sabotage.h"

#include <algorithm>

namespace {
int cpuDuelScore(const Player& player, RandomService& rng) {
    if (player.type != PlayerType::CPU) {
        return rng.uniformInt(40, 85);
    }

    switch (player.cpuDifficulty) {
        case CpuDifficulty::Easy:
            return rng.uniformInt(20, 55);
        case CpuDifficulty::Hard:
            return rng.uniformInt(65, 95);
        case CpuDifficulty::Normal:
        default:
            return rng.uniformInt(45, 75);
    }
}

std::string duelProfileText(const Player& player) {
    if (player.type != PlayerType::CPU) {
        return "Human controlled";
    }
    return "CPU difficulty: " + cpuDifficultyLabel(player.cpuDifficulty);
}
}  // namespace

SabotageManager::SabotageManager(Bank& bankRef, RandomService& random)
    : bank(bankRef),
      rng(random) {
}

bool SabotageManager::rollChance(int percent) {
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }
    return rng.uniformInt(1, 100) <= percent;
}

int SabotageManager::rollOutcome(const SabotageCard& card, bool& success, bool& critical) {
    critical = false;
    if (!card.requiresDiceRoll) {
        success = rollChance(card.successChance);
        return 0;
    }

    const int roll = rng.roll10();
    if (roll <= 3) {
        success = false;
        return roll;
    }
    success = true;
    critical = roll >= 8;
    return roll;
}

int SabotageManager::moneyAmount(int baseAmount, bool critical) const {
    return critical ? baseAmount * 2 : baseAmount;
}

bool SabotageManager::consumeShield(Player& target, std::string& message) {
    if (target.itemDisableTurns > 0) {
        return false;
    }
    if (target.shieldCards <= 0) {
        return false;
    }
    --target.shieldCards;
    message = target.name + "'s Shield Card blocked the sabotage.";
    return true;
}

int SabotageManager::applyInsurance(Player& target, int amount, std::string& message) {
    if (target.itemDisableTurns > 0 || target.insuranceUses <= 0 || amount <= 0) {
        return amount;
    }
    --target.insuranceUses;
    const int reduced = amount / 2;
    message = "Insurance reduced damage from $" + std::to_string(amount) +
              " to $" + std::to_string(reduced) + ".";
    return reduced;
}

SabotageResult SabotageManager::resolveLawsuit(Player& attacker, Player& target) {
    SabotageResult result;
    result.attempted = true;
    const int attackerRoll = rng.roll10();
    const int targetRoll = rng.roll10();
    result.roll = attackerRoll;

    if (attackerRoll > targetRoll) {
        int amount = 50000 + (attackerRoll - targetRoll) * 5000;
        std::string insuranceText;
        amount = applyInsurance(target, amount, insuranceText);
        PaymentResult payment = bank.charge(target, amount);
        bank.credit(attacker, amount);
        result.success = true;
        result.amount = amount;
        result.summary = attacker.name + " won the lawsuit roll " +
                         std::to_string(attackerRoll) + "-" + std::to_string(targetRoll) +
                         ". " + target.name + " paid $" + std::to_string(amount) + ".";
        if (!insuranceText.empty()) {
            result.summary += " " + insuranceText;
        }
        if (payment.loansTaken > 0) {
            result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
        }
        return result;
    }

    const int amount = 25000;
    PaymentResult payment = bank.charge(attacker, amount);
    bank.credit(target, amount);
    result.success = false;
    result.amount = amount;
    result.summary = attacker.name + " lost the lawsuit roll " +
                     std::to_string(attackerRoll) + "-" + std::to_string(targetRoll) +
                     " and paid $" + std::to_string(amount) + ".";
    if (payment.loansTaken > 0) {
        result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
    }
    return result;
}

SabotageResult SabotageManager::resolveForcedDuel(Player& attacker, Player& target) {
    SabotageResult result;
    result.attempted = true;
    const int attackerScore = cpuDuelScore(attacker, rng);
    const int targetScore = cpuDuelScore(target, rng);
    result.roll = attackerScore;
    const int pot = 40000;

    if (attackerScore > targetScore) {
        PaymentResult payment = bank.charge(target, pot);
        bank.credit(attacker, pot);
        result.success = true;
        result.amount = pot;
        result.summary = attacker.name + " won the duel minigame " +
                         std::to_string(attackerScore) + "-" + std::to_string(targetScore) +
                         " and took $" + std::to_string(pot) + ". " +
                         duelProfileText(attacker) + " vs " + duelProfileText(target) + ".";
        if (payment.loansTaken > 0) {
            result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
        }
    } else {
        PaymentResult payment = bank.charge(attacker, pot / 2);
        bank.credit(target, pot / 2);
        result.success = false;
        result.amount = pot / 2;
        result.summary = target.name + " defended the duel minigame " +
                         std::to_string(targetScore) + "-" + std::to_string(attackerScore) +
                         " and collected $" + std::to_string(pot / 2) + ". " +
                         duelProfileText(attacker) + " vs " + duelProfileText(target) + ".";
        if (payment.loansTaken > 0) {
            result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
        }
    }
    return result;
}

SabotageResult SabotageManager::applyDirectSabotage(const SabotageCard& card,
                                                    Player& attacker,
                                                    Player& target,
                                                    std::vector<Player>& players,
                                                    int attackerIndex,
                                                    int targetIndex) {
    SabotageResult result;
    result.attempted = true;

    if (targetIndex < 0 || targetIndex >= static_cast<int>(players.size()) || attackerIndex == targetIndex) {
        result.summary = "Invalid sabotage target.";
        return result;
    }

    std::string shieldText;
    if (consumeShield(target, shieldText)) {
        result.blocked = true;
        result.summary = shieldText;
        return result;
    }

    if (card.type == SabotageType::MoneyLoss && card.name == "Lawsuit") {
        return resolveLawsuit(attacker, target);
    }

    bool success = false;
    bool critical = false;
    result.roll = rollOutcome(card, success, critical);
    result.critical = critical;
    if (!success) {
        result.summary = card.name + " failed";
        if (result.roll > 0) {
            result.summary += " on roll " + std::to_string(result.roll);
        }
        result.summary += ".";
        return result;
    }

    switch (card.type) {
        case SabotageType::MoneyLoss: {
            int amount = moneyAmount(30000, critical);
            std::string insuranceText;
            amount = applyInsurance(target, amount, insuranceText);
            PaymentResult payment = bank.charge(target, amount);
            result.success = true;
            result.amount = amount;
            result.summary = target.name + " lost $" + std::to_string(amount) + ".";
            if (!insuranceText.empty()) {
                result.summary += " " + insuranceText;
            }
            if (payment.loansTaken > 0) {
                result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
            }
            break;
        }
        case SabotageType::MovementPenalty:
            target.movementPenaltyTurns = 1;
            target.movementPenaltyPercent = critical ? 100 : 50;
            result.success = true;
            result.summary = target.name + "'s next movement is " +
                             std::string(critical ? "cancelled." : "cut in half.");
            break;
        case SabotageType::SkipTurn:
            target.skipNextTurn = true;
            result.success = true;
            result.summary = target.name + " will skip their next turn.";
            break;
        case SabotageType::StealCard:
            if (target.actionCards.empty()) {
                result.summary = target.name + " has no action cards to steal.";
                return result;
            } else {
                const int cardIndex = rng.uniformInt(0, static_cast<int>(target.actionCards.size()) - 1);
                const std::string stolen = target.actionCards[static_cast<std::size_t>(cardIndex)];
                target.actionCards.erase(target.actionCards.begin() + cardIndex);
                attacker.actionCards.push_back(stolen);
                result.success = true;
                result.summary = attacker.name + " stole action card: " + stolen + ".";
            }
            break;
        case SabotageType::ForceMinigame:
            return resolveForcedDuel(attacker, target);
        case SabotageType::CareerPenalty:
            target.salaryReductionTurns = critical ? 4 : 3;
            target.salaryReductionPercent = critical ? 35 : 25;
            result.success = true;
            result.summary = target.name + "'s salary is reduced by " +
                             std::to_string(target.salaryReductionPercent) + "% for " +
                             std::to_string(target.salaryReductionTurns) + " turns.";
            break;
        case SabotageType::PropertyDamage: {
            int amount = moneyAmount(25000, critical);
            std::string insuranceText;
            amount = applyInsurance(target, amount, insuranceText);
            PaymentResult payment = bank.charge(target, amount);
            result.success = true;
            result.amount = amount;
            result.summary = target.name + " paid $" + std::to_string(amount) + " for property damage.";
            if (!insuranceText.empty()) {
                result.summary += " " + insuranceText;
            }
            if (payment.loansTaken > 0) {
                result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
            }
            break;
        }
        case SabotageType::DebtIncrease: {
            const int debtAmount = moneyAmount(60000, critical);
            const int loans = bank.issueLoan(target, debtAmount);
            target.sabotageDebt += loans * 60000;
            result.success = true;
            result.amount = loans * 60000;
            result.summary = target.name + " was forced into " +
                             std::to_string(loans) + " loan(s).";
            break;
        }
        case SabotageType::PositionSwap:
            std::swap(attacker.tile, target.tile);
            attacker.sabotageCooldown = critical ? 3 : 4;
            result.success = true;
            result.summary = attacker.name + " swapped positions with " + target.name +
                             " and is on sabotage cooldown.";
            break;
        case SabotageType::ItemDisable:
            target.itemDisableTurns = critical ? 3 : 2;
            result.success = true;
            result.summary = target.name + "'s shields and insurance are disabled for " +
                             std::to_string(target.itemDisableTurns) + " turns.";
            break;
        default:
            result.summary = "No effect.";
            break;
    }

    return result;
}

SabotageResult SabotageManager::triggerTrap(const ActiveTrap& trap, Player& target) {
    SabotageResult result;
    result.attempted = true;

    std::string shieldText;
    if (consumeShield(target, shieldText)) {
        result.blocked = true;
        result.summary = shieldText;
        return result;
    }

    const int roll = rng.roll10();
    result.roll = roll;
    result.critical = roll >= 8;
    if (roll <= 3) {
        result.summary = target.name + " dodged the trap on roll " + std::to_string(roll) + ".";
        return result;
    }

    result.success = true;
    switch (trap.effectType) {
        case SabotageType::MoneyLoss: {
            int amount = moneyAmount(15000 * trap.strengthLevel, result.critical);
            std::string insuranceText;
            amount = applyInsurance(target, amount, insuranceText);
            PaymentResult payment = bank.charge(target, amount);
            result.amount = amount;
            result.summary = target.name + " triggered a money trap and lost $" +
                             std::to_string(amount) + ".";
            if (!insuranceText.empty()) {
                result.summary += " " + insuranceText;
            }
            if (payment.loansTaken > 0) {
                result.summary += " Auto-loans: " + std::to_string(payment.loansTaken) + ".";
            }
            break;
        }
        case SabotageType::MovementPenalty:
            result.summary = target.name + " triggered a movement trap.";
            break;
        case SabotageType::SkipTurn:
            target.skipNextTurn = true;
            result.summary = target.name + " triggered a skip-turn trap.";
            break;
        case SabotageType::StealCard:
            if (!target.actionCards.empty()) {
                const int cardIndex = rng.uniformInt(0, static_cast<int>(target.actionCards.size()) - 1);
                const std::string lost = target.actionCards[static_cast<std::size_t>(cardIndex)];
                target.actionCards.erase(target.actionCards.begin() + cardIndex);
                result.summary = target.name + " lost action card: " + lost + ".";
            } else {
                result.summary = target.name + " triggered a card trap but had no cards.";
            }
            break;
        case SabotageType::ForceMinigame:
            result.summary = target.name + " triggered a forced minigame trap.";
            break;
        default:
            result.summary = target.name + " triggered a trap.";
            break;
    }

    return result;
}
