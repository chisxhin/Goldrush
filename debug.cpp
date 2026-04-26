#include "debug.h"

#include "bank.hpp"
#include "battleship.hpp"
#include "board.hpp"
#include "cards.hpp"
#include "game.hpp"
#include "hangman.hpp"
#include "memory.hpp"
#include "minesweeper.hpp"
#include "player.hpp"
#include "pong.hpp"
#include "random_service.hpp"
#include "rules.hpp"
#include "save_manager.hpp"
#include "spins.hpp"
#include "ui.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
Player makeDebugPlayer(const std::string& name, int index) {
    Player player = Player();
    player.name = name;
    player.token = tokenForName(name, index);
    player.tile = 10;
    player.cash = 50000;
    player.job = "Debug Tester";
    player.salary = 50000;
    player.married = true;
    player.kids = 2;
    player.collegeGraduate = false;
    player.usedNightSchool = false;
    player.hasHouse = false;
    player.houseName = "";
    player.houseValue = 0;
    player.loans = 0;
    player.investedNumber = 0;
    player.investPayout = 0;
    player.spinToWinTokens = 0;
    player.retirementPlace = 0;
    player.retirementBonus = 0;
    player.finalHouseSaleValue = 0;
    player.retirementHome = "";
    player.actionCards.clear();
    player.petCards.clear();
    player.retired = false;
    player.startChoice = 1;
    player.familyChoice = 1;
    player.riskChoice = 0;
    return player;
}

void pauseForEnter() {
    std::cout << "\nPress ENTER to continue...";
    std::string ignored;
    std::getline(std::cin, ignored);
}

int readInt(const std::string& prompt, int minValue, int maxValue, int defaultValue) {
    while (true) {
        std::cout << prompt << " [" << defaultValue << "]: ";
        std::string line;
        std::getline(std::cin, line);
        if (line.empty()) {
            return defaultValue;
        }

        std::istringstream in(line);
        int value = 0;
        if (in >> value && in.eof() && value >= minValue && value <= maxValue) {
            return value;
        }
        std::cout << "Enter a number from " << minValue << " to " << maxValue << ".\n";
    }
}

int readMenuChoice(int minValue, int maxValue) {
    return readInt("Choose an option", minValue, maxValue, minValue);
}

std::string describePayment(const PaymentResult& payment) {
    std::ostringstream out;
    out << "charged $" << payment.charged;
    if (payment.loansTaken > 0) {
        out << ", auto-loans taken: " << payment.loansTaken;
    }
    return out.str();
}

int debugNextTile(const Board& board, const Player& player) {
    const Tile& current = board.tileAt(player.tile);
    if (current.kind == TILE_SPLIT_START) {
        return player.startChoice == 0 ? current.next : current.altNext;
    }
    if (current.kind == TILE_SPLIT_FAMILY) {
        return player.familyChoice == 0 ? current.next : current.altNext;
    }
    if (current.kind == TILE_SPLIT_RISK) {
        return player.riskChoice == 0 ? current.next : current.altNext;
    }
    return current.next;
}

std::string debugMovePlayer(Player& player, const Board& board, int steps) {
    if (steps <= 0) {
        return "No forward movement requested.";
    }

    std::ostringstream out;
    out << player.name << " starts on tile " << player.tile
        << " [" << board.tileAt(player.tile).label << "]\n";

    for (int step = 0; step < steps; ++step) {
        const int next = debugNextTile(board, player);
        if (next < 0 || next == player.tile) {
            out << "  Step " << (step + 1) << ": end of path\n";
            break;
        }

        player.tile = next;
        const Tile& landed = board.tileAt(player.tile);
        out << "  Step " << (step + 1) << ": tile " << player.tile
            << " [" << landed.label << "]";
        if (board.isStopSpace(landed)) {
            out << " STOP";
        }
        out << "\n";

        if (board.isStopSpace(landed)) {
            break;
        }
    }

    return out.str();
}

std::string debugApplyActionEffect(Player& player,
                                   const Board& board,
                                   const Bank& bank,
                                   const Tile& tile,
                                   const ActionEffect& effect) {
    int amount = effect.amount;
    if (effect.useTileValue) {
        amount += tile.value * 2000;
    }

    switch (effect.kind) {
        case ACTION_NO_EFFECT:
            return "No effect.";
        case ACTION_GAIN_CASH:
            bank.credit(player, amount);
            return "Collected $" + std::to_string(amount) + ".";
        case ACTION_PAY_CASH: {
            const PaymentResult payment = bank.charge(player, amount);
            return "Paid $" + std::to_string(amount) + " (" + describePayment(payment) + ").";
        }
        case ACTION_GAIN_PER_KID:
            amount = player.kids * effect.amount;
            bank.credit(player, amount);
            return "Collected $" + std::to_string(amount) + " for kids.";
        case ACTION_PAY_PER_KID: {
            amount = player.kids * effect.amount;
            const PaymentResult payment = bank.charge(player, amount);
            return "Paid $" + std::to_string(amount) + " for kids (" + describePayment(payment) + ").";
        }
        case ACTION_GAIN_SALARY_BONUS:
            player.salary += effect.amount;
            bank.credit(player, effect.amount);
            return "Salary and cash increased by $" + std::to_string(effect.amount) + ".";
        case ACTION_BONUS_IF_MARRIED:
            if (!player.married) {
                return "No bonus because the debug player is not married.";
            }
            bank.credit(player, effect.amount);
            return "Marriage bonus paid $" + std::to_string(effect.amount) + ".";
        case ACTION_MOVE_SPACES:
            return debugMovePlayer(player, board, effect.amount);
        default:
            return "Unknown action effect.";
    }
}

void printPlayerSummary(const Player& player, const Bank& bank) {
    std::cout << player.name << " | tile " << player.tile
              << " | cash $" << player.cash
              << " | loans " << player.loans
              << " | salary $" << player.salary
              << " | loan debt $" << bank.totalLoanDebt(player)
              << "\n";
}

void runCursesPong() {
    initialize_game_ui();
    const PongMinigameResult result = playPongMinigame("DEBUG", has_colors());
    destroy_game_ui();
    std::cout << "Pong result: player score " << result.playerScore
              << ", CPU score " << result.cpuScore
              << ", abandoned " << (result.abandoned ? "yes" : "no") << "\n";
}

void runCursesBattleship() {
    initialize_game_ui();
    const BattleshipMinigameResult result = playBattleshipMinigame("DEBUG", has_colors());
    destroy_game_ui();
    std::cout << "Battleship result: ships destroyed " << result.shipsDestroyed
              << ", cleared wave " << (result.clearedWave ? "yes" : "no")
              << ", abandoned " << (result.abandoned ? "yes" : "no") << "\n";
}

void runCursesHangman() {
    initialize_game_ui();
    const HangmanResult result = playHangmanMinigame("DEBUG", has_colors());
    destroy_game_ui();
    std::cout << "Hangman result: won " << (result.won ? "yes" : "no")
              << ", attempts left " << result.attemptsLeft
              << ", letters guessed " << result.lettersGuessed
              << ", abandoned " << (result.abandoned ? "yes" : "no") << "\n";
}

void runCursesMemory() {
    initialize_game_ui();
    const MemoryMatchResult result = playMemoryMatchMinigame("DEBUG", has_colors());
    destroy_game_ui();
    std::cout << "Memory result: won " << (result.won ? "yes" : "no")
              << ", pairs " << result.pairsMatched
              << ", lives " << result.livesRemaining
              << ", abandoned " << (result.abandoned ? "yes" : "no") << "\n";
}

void runCursesMinesweeper() {
    initialize_game_ui();
    const MinesweeperResult result = playMinesweeperMinigame("DEBUG", has_colors());
    destroy_game_ui();
    std::cout << "Minesweeper result: safe tiles " << result.safeTilesRevealed
              << ", hit bomb " << (result.hitBomb ? "yes" : "no")
              << ", timed out " << (result.timedOut ? "yes" : "no")
              << ", abandoned " << (result.abandoned ? "yes" : "no") << "\n";
}

void debugMathMinigameStub() {
    std::cout << "\nNo production math minigame exists in this codebase yet.\n";
    std::cout << "Running a debug-only arithmetic smoke test with RandomService.\n";

    RandomService rng(42024);
    int correct = 0;
    for (int i = 0; i < 3; ++i) {
        const int left = rng.uniformInt(2, 12);
        const int right = rng.uniformInt(2, 12);
        const int answer = left * right;
        const int guess = readInt("  " + std::to_string(left) + " x " + std::to_string(right), 0, 200, answer);
        if (guess == answer) {
            ++correct;
            std::cout << "  Correct.\n";
        } else {
            std::cout << "  Expected " << answer << ".\n";
        }
    }
    std::cout << "Math stub score: " << correct << "/3\n";
}
}

void debugDiceRoll() {
    std::cout << "\n===== DICE / SPINNER DEBUG =====\n";
    const int rolls = readInt("How many spinner rolls?", 1, 1000, 20);
    RandomService rng(20260427);
    std::vector<int> counts(11, 0);

    for (int i = 0; i < rolls; ++i) {
        const int value = rng.roll10();
        ++counts[static_cast<std::size_t>(value)];
        if (i < 50) {
            std::cout << value << ((i + 1) % 20 == 0 ? "\n" : " ");
        }
    }
    if (rolls > 50) {
        std::cout << "\n(first 50 rolls shown)";
    }
    std::cout << "\n\nDistribution:\n";
    for (int value = 1; value <= 10; ++value) {
        std::cout << "  " << value << ": " << counts[static_cast<std::size_t>(value)] << "\n";
    }

    std::cout << "\nRule helper samples:\n";
    for (int value = 1; value <= 10; ++value) {
        std::cout << "  spin " << value
                  << " | marriage gift $" << marriageGiftFromSpin(value)
                  << " | safe $" << safeRoutePayout(value)
                  << " | risky $" << riskyRoutePayout(value)
                  << "\n";
    }
    pauseForEnter();
}

void debugActionCards() {
    std::cout << "\n===== ACTION CARD DEBUG =====\n";
    RuleSet rules = makeNormalRules();
    RandomService rng(12345);
    DeckManager decks(rules, rng);
    Bank bank(rules);
    Board board;
    Player player = makeDebugPlayer("Debug Player", 0);
    player.tile = 39;

    const int count = readInt("How many action cards should be drawn?", 1, 20, 3);
    for (int i = 0; i < count; ++i) {
        ActionCard card;
        if (!decks.drawActionCard(card)) {
            std::cout << "No more action cards available.\n";
            break;
        }

        std::cout << "\nCard " << (i + 1) << ": " << card.title << "\n";
        std::cout << "  " << card.description << "\n";

        ActionEffect effect = card.effect;
        if (actionCardUsesRoll(card)) {
            const int roll = rng.roll10();
            std::cout << "  Roll: " << roll << "\n";
            const ActionRollOutcome* outcome = findMatchingRollOutcome(card, roll);
            if (outcome != 0) {
                std::cout << "  Branch: "
                          << (outcome->text.empty() ? describeRollCondition(outcome->condition) : outcome->text)
                          << "\n";
                effect = outcome->effect;
            } else {
                std::cout << "  No matching branch.\n";
                effect.kind = ACTION_NO_EFFECT;
            }
        }

        const std::string result = debugApplyActionEffect(player, board, bank, board.tileAt(player.tile), effect);
        std::cout << "  Result: " << result << "\n";
        decks.resolveActionCard(card, card.keepAfterUse);
        printPlayerSummary(player, bank);
    }
    pauseForEnter();
}

void debugPlayerMovement() {
    std::cout << "\n===== PLAYER MOVEMENT DEBUG =====\n";
    Board board;
    Player player = makeDebugPlayer("Mover", 0);
    player.tile = readInt("Start tile id", 0, TILE_COUNT - 1, 10);
    player.startChoice = readInt("Start route: 0 college, 1 career", 0, 1, 1);
    player.familyChoice = readInt("Family split: 0 family, 1 life", 0, 1, 1);
    player.riskChoice = readInt("Risk split: 0 safe, 1 risky", 0, 1, 0);
    const int steps = readInt("Move how many spaces?", 1, 30, 6);

    std::cout << debugMovePlayer(player, board, steps);
    pauseForEnter();
}

void debugSaveSystem() {
    std::cout << "\n===== SAVE SYSTEM DEBUG =====\n";
    SaveManager manager;
    std::string error;
    if (!manager.ensureSaveDirectory(error)) {
        std::cout << "Save directory check failed: " << error << "\n";
        pauseForEnter();
        return;
    }

    std::cout << "Default filename: " << manager.defaultSaveFilename() << "\n";
    std::cout << "Normalized '../debug_empty_save': "
              << manager.normalizeFilename("../debug_empty_save") << "\n";

    initialize_game_ui();
    Game game(20260427);
    destroy_game_ui();

    const std::string filename = "debug_empty_save.sav";
    if (manager.saveGame(game, filename, error)) {
        std::cout << "Wrote smoke-test save: " << manager.resolvePath(filename) << "\n";
        std::cout << "saveExists: " << (manager.saveExists(filename) ? "yes" : "no") << "\n";
        std::cout << "Note: this smoke save has no players; use Load System with a real save to test full restore.\n";
    } else {
        std::cout << "Save failed: " << error << "\n";
    }
    pauseForEnter();
}

void debugLoadSystem() {
    std::cout << "\n===== LOAD SYSTEM DEBUG =====\n";
    SaveManager manager;
    std::string error;
    const std::vector<SaveFileInfo> saves = manager.listSaveFiles(error);
    if (!error.empty()) {
        std::cout << "List failed: " << error << "\n";
        pauseForEnter();
        return;
    }
    if (saves.empty()) {
        std::cout << "No .sav files found in saves/.\n";
        pauseForEnter();
        return;
    }

    for (std::size_t i = 0; i < saves.size(); ++i) {
        std::cout << (i + 1) << ". " << saves[i].filename
                  << " | metadata " << (saves[i].metadataValid ? "ok" : "invalid")
                  << " | modified " << saves[i].modifiedText << "\n";
    }
    const int choice = readInt("Select a save to load, or 0 to cancel",
                               0,
                               static_cast<int>(saves.size()),
                               0);
    if (choice == 0) {
        return;
    }

    initialize_game_ui();
    Game game(20260427);
    destroy_game_ui();

    if (manager.loadGame(game, saves[static_cast<std::size_t>(choice - 1)].filename, error)) {
        std::cout << "Load succeeded for " << saves[static_cast<std::size_t>(choice - 1)].filename << ".\n";
    } else {
        std::cout << "Load failed: " << error << "\n";
    }
    pauseForEnter();
}

void debugCPUDecision() {
    std::cout << "\n===== CPU DECISION DEBUG =====\n";
    std::cout << "No production CPU player strategy API exists yet, so this uses mock CPU inputs.\n";

    RuleSet rules = makeNormalRules();
    RandomService rng(9001);
    DeckManager decks(rules, rng);
    Player cpu = makeDebugPlayer("CPU Test", 1);
    cpu.cash = readInt("CPU cash", 0, 500000, 75000);
    cpu.collegeGraduate = readInt("CPU has degree? 0 no, 1 yes", 0, 1, 0) == 1;

    const bool chooseCollege = cpu.cash >= 100000 && !cpu.collegeGraduate;
    std::cout << "Mock start-route decision: " << (chooseCollege ? "College" : "Career") << "\n";

    const bool degreeCareer = chooseCollege || cpu.collegeGraduate;
    const std::vector<CareerCard> choices = decks.drawCareerChoices(degreeCareer, 2);
    if (choices.empty()) {
        std::cout << "Career deck returned no choices.\n";
    } else {
        int bestIndex = 0;
        for (std::size_t i = 1; i < choices.size(); ++i) {
            if (choices[i].salary > choices[static_cast<std::size_t>(bestIndex)].salary) {
                bestIndex = static_cast<int>(i);
            }
        }
        for (std::size_t i = 0; i < choices.size(); ++i) {
            std::cout << "  Choice " << (i + 1) << ": " << choices[i].title
                      << " salary $" << choices[i].salary
                      << (static_cast<int>(i) == bestIndex ? " <- mock CPU picks" : "")
                      << "\n";
        }
    }
    pauseForEnter();
}

void debugMinigames() {
    while (true) {
        std::cout << "\n===== MINIGAME DEBUG MENU =====\n"
                  << "1. Test memory minigame\n"
                  << "2. Test math minigame\n"
                  << "3. Test reaction minigame\n"
                  << "4. Test luck-based minigame\n"
                  << "5. Test hangman minigame\n"
                  << "6. Test battleship minigame\n"
                  << "7. Return to main debug menu\n";

        const int choice = readMenuChoice(1, 7);
        if (choice == 1) {
            runCursesMemory();
            pauseForEnter();
        } else if (choice == 2) {
            debugMathMinigameStub();
            pauseForEnter();
        } else if (choice == 3) {
            runCursesPong();
            pauseForEnter();
        } else if (choice == 4) {
            runCursesMinesweeper();
            pauseForEnter();
        } else if (choice == 5) {
            runCursesHangman();
            pauseForEnter();
        } else if (choice == 6) {
            runCursesBattleship();
            pauseForEnter();
        } else if (choice == 7) {
            return;
        }
    }
}

void debugSabotage() {
    std::cout << "\n===== SABOTAGE DEBUG =====\n";
    std::cout << "No production sabotage feature API exists yet; using mock cash penalties.\n";

    RuleSet rules = makeNormalRules();
    Bank bank(rules);
    Player attacker = makeDebugPlayer("Attacker", 0);
    Player target = makeDebugPlayer("Target", 1);
    target.cash = readInt("Target starting cash", 0, 200000, 25000);

    std::cout << "Before sabotage-like penalty:\n";
    printPlayerSummary(attacker, bank);
    printPlayerSummary(target, bank);

    const int penalty = readInt("Penalty amount", 0, 250000, 75000);
    const PaymentResult payment = bank.charge(target, penalty);
    std::cout << "Applied mock penalty: " << describePayment(payment) << "\n";
    std::cout << "After penalty:\n";
    printPlayerSummary(target, bank);
    pauseForEnter();
}

void runDebugMenu() {
    while (true) {
        std::cout << "\n===== DEBUG MENU =====\n"
                  << "1. Test dice rolling\n"
                  << "2. Test action card effects\n"
                  << "3. Test player movement\n"
                  << "4. Test save system\n"
                  << "5. Test load system\n"
                  << "6. Test CPU player decision-making\n"
                  << "7. Test minigames\n"
                  << "8. Test sabotage features\n"
                  << "9. Exit\n";

        const int choice = readMenuChoice(1, 9);
        switch (choice) {
            case 1:
                debugDiceRoll();
                break;
            case 2:
                debugActionCards();
                break;
            case 3:
                debugPlayerMovement();
                break;
            case 4:
                debugSaveSystem();
                break;
            case 5:
                debugLoadSystem();
                break;
            case 6:
                debugCPUDecision();
                break;
            case 7:
                debugMinigames();
                break;
            case 8:
                debugSabotage();
                break;
            case 9:
                std::cout << "Exiting debug menu.\n";
                return;
            default:
                break;
        }
    }
}

int main() {
    runDebugMenu();
    return 0;
}
