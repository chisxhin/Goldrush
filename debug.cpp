#include "debug.h"

#include "bank.hpp"
#include "battleship.hpp"
#include "board.hpp"
#include "cards.hpp"
#include "completed_history.h"
#include "cpu_player.hpp"
#include "dice_art.h"
#include "game.hpp"
#include "hangman.hpp"
#include "input_helpers.h"
#include "memory.hpp"
#include "minesweeper.hpp"
#include "minigame_tutorials.h"
#include "player.hpp"
#include "pong.hpp"
#include "random_service.hpp"
#include "rules.hpp"
#include "save_manager.hpp"
#include "sabotage.h"
#include "spins.hpp"
#include "tile_display.h"
#include "timer_display.h"
#include "turn_summary.h"
#include "tutorials.h"
#include "ui.h"
#include "ui_helpers.h"

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <fstream>
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
    player.type = PlayerType::Human;
    player.cpuDifficulty = CpuDifficulty::Normal;
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
        << " - " << getTileDisplayName(board.tileAt(player.tile)) << "\n";

    for (int step = 0; step < steps; ++step) {
        const int next = debugNextTile(board, player);
        if (next < 0 || next == player.tile) {
            out << "  Step " << (step + 1) << ": end of path\n";
            break;
        }

        player.tile = next;
        const Tile& landed = board.tileAt(player.tile);
        out << "  Step " << (step + 1) << ": tile " << player.tile
            << " - " << getTileDisplayName(landed);
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
        case ACTION_DUEL_MINIGAME:
            return "Would draw a random opponent and launch a production duel minigame in Game.";
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

std::vector<Player> makeBoardPreviewPlayers() {
    std::vector<Player> players;
    players.push_back(makeDebugPlayer("Alex", 0));
    players.push_back(makeDebugPlayer("CPU Bronze", 1));
    players.push_back(makeDebugPlayer("CPU Gold", 2));
    players.push_back(makeDebugPlayer("Jamie", 3));

    players[0].tile = 20;
    players[0].cash = 92000;
    players[0].job = "Architect";
    players[0].houseName = "Starter Loft";
    players[0].hasHouse = true;
    players[0].actionCards.push_back("Bonus Payday");

    players[1].type = PlayerType::CPU;
    players[1].cpuDifficulty = CpuDifficulty::Easy;
    players[1].tile = 44;
    players[1].cash = 38000;
    players[1].loans = 2;
    players[1].job = "Chef";

    players[2].type = PlayerType::CPU;
    players[2].cpuDifficulty = CpuDifficulty::Hard;
    players[2].tile = 68;
    players[2].cash = 174000;
    players[2].job = "Engineer";
    players[2].investedNumber = 7;
    players[2].investPayout = 45000;

    players[3].tile = 83;
    players[3].cash = 61000;
    players[3].job = "Teacher";
    players[3].married = false;
    players[3].kids = 0;

    return players;
}

void showBoardPreview(const std::string& eventTitle,
                      const std::string& eventMessage,
                      const std::vector<Player>& previewPlayers,
                      int currentPlayerIndex,
                      const std::vector<std::string>& historyLines,
                      const RuleSet& rules) {
    if (previewPlayers.empty()) {
        return;
    }

    initialize_game_ui();
    const UILayout layout = calculateUILayout();
    Board board;

    WINDOW* titleWin = newwin(layout.headerHeight, layout.totalWidth, layout.originY, layout.originX);
    WINDOW* boardWin = newwin(layout.boardHeight, layout.boardWidth, layout.originY + layout.headerHeight, layout.originX);
    WINDOW* infoWin = newwin(layout.sidePanelHeight,
                             layout.sidePanelWidth,
                             layout.originY + layout.headerHeight,
                             layout.originX + layout.boardWidth);
    WINDOW* msgWin = newwin(layout.messageHeight,
                            layout.totalWidth,
                            layout.originY + layout.headerHeight + layout.boardHeight,
                            layout.originX);

    apply_ui_background(titleWin);
    apply_ui_background(boardWin);
    apply_ui_background(infoWin);
    apply_ui_background(msgWin);
    keypad(msgWin, TRUE);

    const int safeCurrent = std::max(0, std::min(currentPlayerIndex, static_cast<int>(previewPlayers.size()) - 1));
    draw_title_banner_ui(titleWin);
    draw_board_ui(boardWin, board, previewPlayers, previewPlayers[static_cast<std::size_t>(safeCurrent)].tile, safeCurrent);
    draw_sidebar_ui(infoWin, board, previewPlayers, safeCurrent, historyLines, rules);
    drawEventMessage(msgWin, eventTitle, eventMessage);
    waitForEnterPrompt(msgWin, layout.messageHeight - 2, 2, "Press ENTER to continue...");

    delwin(msgWin);
    delwin(infoWin);
    delwin(boardWin);
    delwin(titleWin);
    destroy_game_ui();
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
    std::cout << "Uses the production CpuController with mock player inputs.\n";

    RuleSet rules = makeNormalRules();
    RandomService rng(9001);
    DeckManager decks(rules, rng);
    CpuController cpuController(rng);
    Player cpu = makeDebugPlayer("CPU Test", 1);
    cpu.type = PlayerType::CPU;
    cpu.cash = readInt("CPU cash", 0, 500000, 75000);
    cpu.collegeGraduate = readInt("CPU has degree? 0 no, 1 yes", 0, 1, 0) == 1;
    cpu.cpuDifficulty = static_cast<CpuDifficulty>(
        readInt("Difficulty: 0 easy, 1 normal, 2 hard", 0, 2, 1));

    const bool chooseCollege = cpuController.chooseStartRoute(cpu, rules) == 0;
    std::cout << "CPU start-route decision: " << (chooseCollege ? "College" : "Career") << "\n";
    std::cout << "CPU family decision: "
              << (cpuController.chooseFamilyRoute(cpu, rules) == 0 ? "Family" : "Life") << "\n";
    std::cout << "CPU risk decision: "
              << (cpuController.chooseRiskRoute(cpu, rules) == 0 ? "Safe" : "Risky") << "\n";

    const bool degreeCareer = chooseCollege || cpu.collegeGraduate;
    const std::vector<CareerCard> choices = decks.drawCareerChoices(degreeCareer, 2);
    if (choices.empty()) {
        std::cout << "Career deck returned no choices.\n";
    } else {
        const int pickedIndex = cpuController.chooseCareer(cpu, choices);
        for (std::size_t i = 0; i < choices.size(); ++i) {
            std::cout << "  Choice " << (i + 1) << ": " << choices[i].title
                      << " salary $" << choices[i].salary
                      << (static_cast<int>(i) == pickedIndex ? " <- CPU picks" : "")
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

int simulateDebugCPUMinigamePerformance(Player& cpuPlayer, RandomService& rng) {
    if (cpuPlayer.type != PlayerType::CPU) {
        return rng.uniformInt(40, 85);
    }

    switch (cpuPlayer.cpuDifficulty) {
        case CpuDifficulty::Easy:
            return rng.uniformInt(20, 55);
        case CpuDifficulty::Hard:
            return rng.uniformInt(65, 95);
        case CpuDifficulty::Normal:
        default:
            return rng.uniformInt(45, 75);
    }
}

int chooseRandomOpponentIndex(int currentPlayer, const std::vector<Player>& players, RandomService& rng) {
    std::vector<int> candidates;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (static_cast<int>(i) != currentPlayer && !players[i].retired) {
            candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) {
        return -1;
    }
    return candidates[static_cast<std::size_t>(rng.uniformInt(0, static_cast<int>(candidates.size()) - 1))];
}

void debugBlinkingIndicator() {
    initialize_game_ui();
    blinkIndicator("DEBUG INDICATOR: blinks twice, then holds solid", 2, 2000);
    destroy_game_ui();
    std::cout << "Blinking indicator test completed.\n";
    pauseForEnter();
}

void debugMinigameTutorialPopup() {
    initialize_game_ui();
    showMinigameTutorial("Debug Tutorial",
                         "Confirm the one-page tutorial explains the game before play starts.",
                         "Press Enter to start after reading.",
                         "The minigame does not begin until Enter is pressed.",
                         "Rewards, penalties, sabotage, and investment notes are shown here.",
                         has_colors());
    destroy_game_ui();
    std::cout << "Minigame tutorial popup test completed.\n";
    pauseForEnter();
}

void debugCountdownTimer() {
    initialize_game_ui();
    displayCountdownTimer(5, has_colors());
    destroy_game_ui();
    std::cout << "Countdown timer test completed.\n";
    pauseForEnter();
}

void debugDecisionPopup() {
    initialize_game_ui();
    showDecisionPopup("CPU 2",
                      "Career Path",
                      "CPU Difficulty: Normal. Reason: Career Path gives faster money growth early in the game.",
                      has_colors(),
                      false);
    destroy_game_ui();
    std::cout << "Decision popup test completed.\n";
    pauseForEnter();
}

void debugTileDisplayNames() {
    std::cout << "\n===== TILE DISPLAY NAME DEBUG =====\n";
    Board board;
    const std::vector<std::string> legend = board.tutorialLegend();
    for (std::size_t i = 0; i < legend.size(); ++i) {
        std::cout << "  " << legend[i] << "\n";
    }
    std::cout << "\nSample board spaces:\n";
    for (int id = 0; id < TILE_COUNT; id += 8) {
        const Tile& tile = board.tileAt(id);
        std::cout << "  Space " << id << ": " << getTileDisplayName(tile) << "\n";
    }
    pauseForEnter();
}

void debugActionCardMinigameOpponentDraw() {
    std::cout << "\n===== 2-PLAYER MINIGAME OPPONENT DRAW DEBUG =====\n";
    RandomService rng(24680);
    std::vector<Player> players;
    players.push_back(makeDebugPlayer("Player 1", 0));
    players.push_back(makeDebugPlayer("CPU Easy", 1));
    players.push_back(makeDebugPlayer("CPU Hard", 2));
    players.push_back(makeDebugPlayer("Player 4", 3));
    players[1].type = PlayerType::CPU;
    players[1].cpuDifficulty = CpuDifficulty::Easy;
    players[2].type = PlayerType::CPU;
    players[2].cpuDifficulty = CpuDifficulty::Hard;

    const int currentPlayer = 0;
    const int opponent = chooseRandomOpponentIndex(currentPlayer, players, rng);
    if (opponent < 0) {
        std::cout << "No valid opponent was available.\n";
        pauseForEnter();
        return;
    }

    std::cout << "Player 1 used a Duel Minigame Card!\n";
    std::cout << "Random opponent draw begins...\n\n";
    std::cout << "Opponent selected: " << players[static_cast<std::size_t>(opponent)].name << "\n";
    std::cout << "Player 1 will play against " << players[static_cast<std::size_t>(opponent)].name << ".\n";
    if (players[static_cast<std::size_t>(opponent)].type == PlayerType::CPU) {
        const int score = simulateDebugCPUMinigamePerformance(players[static_cast<std::size_t>(opponent)], rng);
        std::cout << "CPU difficulty: "
                  << cpuDifficultyLabel(players[static_cast<std::size_t>(opponent)].cpuDifficulty)
                  << "\n";
        std::cout << "Simulated CPU performance score: " << score << "/100\n";
    } else {
        std::cout << "Opponent is human: the normal minigame controls would be used.\n";
    }
    pauseForEnter();
}

void debugCPUMinigameDifficulty() {
    std::cout << "\n===== CPU MINIGAME DIFFICULTY DEBUG =====\n";
    RandomService rng(13579);
    std::vector<Player> cpus;
    cpus.push_back(makeDebugPlayer("Easy CPU", 0));
    cpus.push_back(makeDebugPlayer("Normal CPU", 1));
    cpus.push_back(makeDebugPlayer("Hard CPU", 2));
    cpus[0].type = PlayerType::CPU;
    cpus[0].cpuDifficulty = CpuDifficulty::Easy;
    cpus[1].type = PlayerType::CPU;
    cpus[1].cpuDifficulty = CpuDifficulty::Normal;
    cpus[2].type = PlayerType::CPU;
    cpus[2].cpuDifficulty = CpuDifficulty::Hard;

    for (std::size_t i = 0; i < cpus.size(); ++i) {
        std::cout << cpus[i].name << " (" << cpuDifficultyLabel(cpus[i].cpuDifficulty) << "): ";
        for (int run = 0; run < 5; ++run) {
            std::cout << simulateDebugCPUMinigamePerformance(cpus[i], rng)
                      << (run == 4 ? "" : ", ");
        }
        std::cout << "\n";
    }
    pauseForEnter();
}

void debugDetailedTurnOutput() {
    Board board;
    Player player = makeDebugPlayer("Player 1", 0);
    player.tile = 12;
    player.cash = 45000;
    const int roll = 5;
    const int startTile = player.tile;
    const int endTile = 17;
    const Tile& start = board.tileAt(startTile);
    const Tile& end = board.tileAt(endTile);

    std::vector<std::string> lines;
    lines.push_back(player.name + "'s Turn");
    lines.push_back("Player Type: " + playerTypeLabel(player.type));
    lines.push_back("Current Money: $" + std::to_string(player.cash));
    lines.push_back("Current Position: Space " + std::to_string(startTile) + " - " + getTileDisplayName(start));
    lines.push_back(player.name + " rolls the dice...");
    lines.push_back(player.name + " rolled a " + std::to_string(roll) + ".");
    lines.push_back(player.name + " moves from Space " + std::to_string(startTile) +
                    " to Space " + std::to_string(endTile) + ".");
    lines.push_back(player.name + " landed on " + getTileDisplayName(end) + ".");
    lines.push_back("Effect: debug preview of the tile effect explanation.");
    lines.push_back("New Money: $65000");

    initialize_game_ui();
    showPopupMessage("Detailed Turn Output", lines, has_colors(), false);
    destroy_game_ui();
    std::cout << "Detailed turn output popup test completed.\n";
    pauseForEnter();
}

void debugUiPacing() {
    while (true) {
        std::cout << "\n===== UI / PACING DEBUG MENU =====\n"
                  << "1. Test blinking indicator\n"
                  << "2. Test minigame tutorial popup\n"
                  << "3. Test countdown timer text\n"
                  << "4. Test decision popup\n"
                  << "5. Test tile full-name display\n"
                  << "6. Test 2-player action-card minigame opponent draw\n"
                  << "7. Test CPU minigame difficulty behavior\n"
                  << "8. Test detailed turn output\n"
                  << "9. Return\n";

        const int choice = readMenuChoice(1, 9);
        if (choice == 1) {
            debugBlinkingIndicator();
        } else if (choice == 2) {
            debugMinigameTutorialPopup();
        } else if (choice == 3) {
            debugCountdownTimer();
        } else if (choice == 4) {
            debugDecisionPopup();
        } else if (choice == 5) {
            debugTileDisplayNames();
        } else if (choice == 6) {
            debugActionCardMinigameOpponentDraw();
        } else if (choice == 7) {
            debugCPUMinigameDifficulty();
        } else if (choice == 8) {
            debugDetailedTurnOutput();
        } else if (choice == 9) {
            return;
        }
    }
}

void debugTileColorsAndSymbols() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    players[0].tile = 10;
    players[1].tile = 31;
    players[2].tile = 55;
    players[3].tile = 86;
    showBoardPreview("Tile Colors and Symbols",
                     "Preview the board surface: quiet basic spaces, brighter specials, and stronger money/risk landmarks.",
                     players,
                     0,
                     std::vector<std::string>{
                         "Alex spun 6 and collected salary.",
                         "CPU Gold investment matched 7.",
                         "Jamie used an action card."
                     },
                     rules);
}

void debugTileFullNameDisplay() {
    std::cout << "\n===== TILE FULL-NAME DISPLAY DEBUG =====\n";
    Board board;
    for (int tileId = 0; tileId < TILE_COUNT; tileId += 7) {
        const Tile& tile = board.tileAt(tileId);
        std::cout << "Space " << tileId
                  << " | symbol [" << getTileBoardSymbol(tile) << "]"
                  << " | " << getTileDisplayName(tile)
                  << "\n";
    }
    pauseForEnter();
}

void debugPlayerTokenHighlighting() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    players[0].tile = 44;
    players[1].tile = 44;
    players[2].tile = 44;
    players[3].tile = 68;
    showBoardPreview("Player Token Highlight",
                     "Three players stack on one tile to test [3P], while the current player stays strongly highlighted.",
                     players,
                     2,
                     std::vector<std::string>{
                         "CPU Gold moved onto Marriage Stop.",
                         "Alex joined the same tile.",
                         "CPU Bronze stacked there too."
                     },
                     rules);
}

void debugBoardLegend() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    showBoardPreview("Board Legend",
                     "Check that the legend uses the same symbols and colors as the board tiles.",
                     players,
                     0,
                     std::vector<std::string>{
                         "Legend should explain payday, action, career, and risk."
                     },
                     rules);
}

void debugBoardRegions() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    players[0].tile = 12;
    players[1].tile = 38;
    players[2].tile = 58;
    players[3].tile = 83;
    showBoardPreview("Board Regions",
                     "Scan the board for Startup Street, Career City, Goldrush Valley, Family Avenue, Risky Road, and Retirement Ridge.",
                     players,
                     1,
                     std::vector<std::string>{
                         "CPU Bronze reached Career City.",
                         "CPU Gold entered Family Avenue.",
                         "Jamie approached Risky Road."
                     },
                     rules);
}

void debugBoardLandmarks() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    showBoardPreview("Board Landmarks",
                     "Look for BANK, UNI, LUCK HALL, and the retirement landmark without them covering tiles or player tokens.",
                     players,
                     3,
                     std::vector<std::string>{
                         "Decorative landmarks should stay outside the route."
                     },
                     rules);
}

void debugPlayerSidePanel() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    players[0].loans = 5;
    players[0].insuranceUses = 1;
    players[0].shieldCards = 2;
    players[0].sabotageCooldown = 1;
    showBoardPreview("Player Side Panel",
                     "Verify the roster, grouped STATUS/LIFE/DEFENSE sections, and current-player highlighting.",
                     players,
                     0,
                     std::vector<std::string>{
                         "Alex picked up insurance.",
                         "CPU Bronze took a loan.",
                         "CPU Gold banked a big payday."
                     },
                     rules);
}

void debugHistoryFormatting() {
    RuleSet rules = makeNormalRules();
    std::vector<Player> players = makeBoardPreviewPlayers();
    showBoardPreview("History Formatting",
                     "History should show colored category tags like [MOVE], [CARD], [MINIGAME], and [MONEY].",
                     players,
                     1,
                     std::vector<std::string>{
                         "CPU Bronze moved 4 spaces.",
                         "Alex used a Forced Duel card.",
                         "CPU Gold earned salary $45000."
                     },
                     rules);
}

void debugCurrentObjectiveBox() {
    RuleSet rules = makeNormalRules();
    rules.toggles.investmentEnabled = true;
    std::vector<Player> players = makeBoardPreviewPlayers();
    players[0].loans = 5;
    players[0].cash = 25000;
    players[0].tile = 78;
    players[0].actionCards.push_back("Shield Card");
    showBoardPreview("Current Objective Box",
                     "The hint box should warn about debt, low cash, action cards, or a nearby payday depending on the active player state.",
                     players,
                     0,
                     std::vector<std::string>{
                         "Alex is carrying heavy debt.",
                         "A payday is close ahead."
                     },
                     rules);
}

void debugEventMessagePanel() {
    initialize_game_ui();
    const UILayout layout = calculateUILayout();
    WINDOW* msgWin = newwin(layout.messageHeight,
                            layout.totalWidth,
                            layout.originY + layout.headerHeight + layout.boardHeight,
                            layout.originX);
    apply_ui_background(msgWin);
    keypad(msgWin, TRUE);
    drawEventMessage(msgWin,
                     "Hangman Sidegame",
                     "Alex exited early. No payout awarded. This preview checks the more polished bottom event panel.");
    waitForEnterPrompt(msgWin, layout.messageHeight - 2, 2, "Press ENTER to continue...");
    delwin(msgWin);
    destroy_game_ui();
}

void debugBoardUi() {
    while (true) {
        std::cout << "\n===== BOARD UI DEBUG MENU =====\n"
                  << "1. Test tile colors and symbols\n"
                  << "2. Test tile full-name display\n"
                  << "3. Test player token highlighting\n"
                  << "4. Test board legend\n"
                  << "5. Test board regions\n"
                  << "6. Test landmarks\n"
                  << "7. Test player side panel\n"
                  << "8. Test history formatting\n"
                  << "9. Test current objective box\n"
                  << "10. Test event message panel\n"
                  << "11. Return\n";

        const int choice = readMenuChoice(1, 11);
        if (choice == 1) {
            debugTileColorsAndSymbols();
        } else if (choice == 2) {
            debugTileFullNameDisplay();
        } else if (choice == 3) {
            debugPlayerTokenHighlighting();
        } else if (choice == 4) {
            debugBoardLegend();
        } else if (choice == 5) {
            debugBoardRegions();
        } else if (choice == 6) {
            debugBoardLandmarks();
        } else if (choice == 7) {
            debugPlayerSidePanel();
        } else if (choice == 8) {
            debugHistoryFormatting();
        } else if (choice == 9) {
            debugCurrentObjectiveBox();
        } else if (choice == 10) {
            debugEventMessagePanel();
        } else {
            return;
        }
    }
}

void debugSabotage() {
    while (true) {
        std::cout << "\n===== SABOTAGE DEBUG MENU =====\n"
                  << "1. Test Trap Tile\n"
                  << "2. Test Lawsuit\n"
                  << "3. Test Traffic Jam\n"
                  << "4. Test Steal Action Card\n"
                  << "5. Test Forced Duel Minigame\n"
                  << "6. Test Career Sabotage\n"
                  << "7. Test Position Swap\n"
                  << "8. Test Debt Trap\n"
                  << "9. Test Shield Card\n"
                  << "10. Test CPU Sabotage Decision\n"
                  << "11. Return\n";

        const int choice = readInt("Choose an option", 1, 11, 1);
        if (choice == 11) {
            return;
        }

        RuleSet rules = makeNormalRules();
        Bank bank(rules);
        RandomService rng(8080 + static_cast<std::uint32_t>(choice));
        SabotageManager manager(bank, rng);
        std::vector<Player> players;
        players.push_back(makeDebugPlayer("Attacker", 0));
        players.push_back(makeDebugPlayer("Target", 1));
        players[0].cash = 180000;
        players[1].cash = 160000;
        players[1].salary = 80000;
        players[1].actionCards.push_back("Debug Bonus Card");

        SabotageResult result;
        if (choice == 1) {
            ActiveTrap trap;
            trap.tileId = 20;
            trap.ownerIndex = 0;
            trap.effectType = SabotageType::MoneyLoss;
            trap.strengthLevel = 2;
            trap.armed = true;
            result = manager.triggerTrap(trap, players[1]);
        } else if (choice == 2) {
            result = manager.resolveLawsuit(players[0], players[1]);
        } else if (choice == 3) {
            result = manager.applyDirectSabotage(sabotageCardForType(SabotageType::MovementPenalty),
                                                 players[0], players[1], players, 0, 1);
        } else if (choice == 4) {
            result = manager.applyDirectSabotage(sabotageCardForType(SabotageType::StealCard),
                                                 players[0], players[1], players, 0, 1);
        } else if (choice == 5) {
            result = manager.resolveForcedDuel(players[0], players[1]);
        } else if (choice == 6) {
            result = manager.applyDirectSabotage(sabotageCardForType(SabotageType::CareerPenalty),
                                                 players[0], players[1], players, 0, 1);
        } else if (choice == 7) {
            players[0].tile = 12;
            players[1].tile = 72;
            result = manager.applyDirectSabotage(sabotageCardForType(SabotageType::PositionSwap),
                                                 players[0], players[1], players, 0, 1);
        } else if (choice == 8) {
            result = manager.applyDirectSabotage(sabotageCardForType(SabotageType::DebtIncrease),
                                                 players[0], players[1], players, 0, 1);
        } else if (choice == 9) {
            players[1].shieldCards = 1;
            result = manager.applyDirectSabotage(sabotageCardByName("Lawsuit"),
                                                 players[0], players[1], players, 0, 1);
        } else if (choice == 10) {
            CpuController cpuController(rng);
            players[0].type = PlayerType::CPU;
            players[0].cpuDifficulty = CpuDifficulty::Hard;
            const int targetIndex = cpuController.chooseSabotageTarget(players[0], players, 0);
            const SabotageType type = cpuController.chooseSabotageType(players[0], players[1], 12);
            std::cout << "CPU target index: " << targetIndex << "\n";
            std::cout << "CPU sabotage type: " << sabotageTypeName(type) << "\n";
            pauseForEnter();
            continue;
        }

        std::cout << "Result: " << result.summary << "\n";
        std::cout << "Attempted: " << (result.attempted ? "yes" : "no")
                  << " | success: " << (result.success ? "yes" : "no")
                  << " | blocked: " << (result.blocked ? "yes" : "no")
                  << " | roll: " << result.roll << "\n";
        printPlayerSummary(players[0], bank);
        printPlayerSummary(players[1], bank);
        std::cout << "Target states: shields " << players[1].shieldCards
                  << ", insurance " << players[1].insuranceUses
                  << ", skip " << (players[1].skipNextTurn ? "yes" : "no")
                  << ", movement penalty " << players[1].movementPenaltyPercent
                  << "%, salary reduction " << players[1].salaryReductionPercent
                  << "% for " << players[1].salaryReductionTurns << " turns"
                  << ", sabotage debt $" << players[1].sabotageDebt << "\n";
        pauseForEnter();
    }
}

void debugPreGameTutorial() {
    initialize_game_ui();
    showPreGameQuickGuide(has_colors());
    destroy_game_ui();
}

void debugFirstTimeTutorialPopup() {
    initialize_game_ui();
    showFirstTimeTutorial(TutorialTopic::AutomaticLoan, has_colors());
    destroy_game_ui();
}

void debugGuideScreen() {
    initialize_game_ui();
    Board board;
    RuleSet rules = makeNormalRules();
    showFullGuide(board, rules, false, has_colors());
    destroy_game_ui();
}

void debugTitleQuitConfirmation() {
    initialize_game_ui();
    const bool confirmed = showQuitConfirmation(has_colors());
    destroy_game_ui();
    std::cout << "Quit confirmation result: " << (confirmed ? "confirmed" : "cancelled") << "\n";
    pauseForEnter();
}

void debugEscCancelBehavior() {
    std::cout << "\n===== ESC CANCEL DEBUG =====\n";
    std::cout << "ESC maps to cancel: " << (isCancelKey(27) ? "yes" : "no") << "\n";
    std::cout << "Q maps to cancel: " << (isCancelKey('Q') ? "yes" : "no") << "\n";
    std::cout << "Single-player UP arrow action enum value: "
              << static_cast<int>(getInputAction(KEY_UP, ControlScheme::SinglePlayer)) << "\n";
    pauseForEnter();
}

void debugSabotageUnlockAnimation() {
    initialize_game_ui();
    showSabotageUnlockAnimation(has_colors());
    destroy_game_ui();
}

void debugSabotageTutorial() {
    initialize_game_ui();
    showSabotageTutorial(has_colors());
    destroy_game_ui();
}

void debugSabotageDiceResolution() {
    RuleSet rules = makeNormalRules();
    Bank bank(rules);
    RandomService rng(4242);
    SabotageManager manager(bank, rng);
    Player attacker = makeDebugPlayer("Attacker", 0);
    Player target = makeDebugPlayer("Target", 1);
    SabotageResult result = manager.resolveLawsuit(attacker, target, 8, 4);
    std::cout << "\n===== SABOTAGE DICE RESOLUTION DEBUG =====\n";
    std::cout << result.summary << "\n";
    printPlayerSummary(attacker, bank);
    printPlayerSummary(target, bank);
    pauseForEnter();
}

void debugSabotageDuelResolution() {
    RuleSet rules = makeNormalRules();
    Bank bank(rules);
    RandomService rng(5252);
    SabotageManager manager(bank, rng);
    Player attacker = makeDebugPlayer("Attacker", 0);
    Player target = makeDebugPlayer("Target", 1);
    SabotageResult result = manager.resolveForcedDuel(attacker, target);
    std::cout << "\n===== SABOTAGE DUEL RESOLUTION DEBUG =====\n";
    std::cout << result.summary << "\n";
    printPlayerSummary(attacker, bank);
    printPlayerSummary(target, bank);
    pauseForEnter();
}

void debugBattleshipControls() {
    initialize_game_ui();
    (void)playBattleshipMinigame("Debug Player", has_colors());
    destroy_game_ui();
}

void debugBattleshipAmmoReload() {
    debugBattleshipControls();
}

void debugCPUEndTurnWait() {
    TurnSummary summary;
    summary.playerName = "CPU 1";
    summary.turnNumber = 6;
    summary.moneyChange = 20000;
    summary.cardsGained.push_back("Salary Boost");
    summary.importantEvents.push_back("CPU 1 completed a clear automated turn.");
    initialize_game_ui();
    showTurnSummaryReport(summary, has_colors());
    destroy_game_ui();
}

void debugCleanTurnSummary() {
    TurnSummary summary;
    summary.playerName = "Player 1";
    summary.turnNumber = 6;
    summary.moneyChange = 20000;
    summary.loanChange = 1;
    summary.babyChange = 1;
    summary.gotMarried = true;
    summary.jobChanged = true;
    summary.oldJob = "Unemployed";
    summary.newJob = "Engineer";
    summary.sabotageEvents.push_back("Blocked a trap with a shield.");
    initialize_game_ui();
    showTurnSummaryReport(summary, has_colors());
    destroy_game_ui();
}

void debugMemoryMatchSymbols() {
    std::cout << "\n===== MEMORY MATCH SYMBOL DEBUG =====\n";
    const std::vector<std::string> symbols = getMemoryMatchSymbols(false);
    for (std::size_t i = 0; i < symbols.size(); ++i) {
        std::cout << symbols[i] << (i + 1 == symbols.size() ? "\n" : " ");
    }
    pauseForEnter();
}

void debugMemoryMatchMemorizePopup() {
    initialize_game_ui();
    showPopupMessage("Memory Match Memorize Preview",
                     std::vector<std::string>{
                         "MEMORIZE NOW!",
                         "Study the cards carefully.",
                         "Cards will flip over soon.",
                         "Time Remaining: 5",
                         "The in-game Memory Match screen now draws this as a large side popup."
                     },
                     has_colors(),
                     false);
    destroy_game_ui();
}

void debugInputMapping() {
    std::cout << "\n===== INPUT MAPPING DEBUG =====\n";
    std::cout << "W single-player: " << static_cast<int>(getInputAction('w')) << "\n";
    std::cout << "UP single-player: " << static_cast<int>(getInputAction(KEY_UP)) << "\n";
    std::cout << "A duel-left: " << static_cast<int>(getInputAction('a', ControlScheme::DuelLeftPlayer)) << "\n";
    std::cout << "LEFT duel-right: " << static_cast<int>(getInputAction(KEY_LEFT, ControlScheme::DuelRightPlayer)) << "\n";
    std::cout << "ESC: " << static_cast<int>(getInputAction(27)) << "\n";
    pauseForEnter();
}

void debugDiceRollAsciiArt() {
    initialize_game_ui();
    showPopupMessage("DEBUG PLAYER ROLLED",
                     std::vector<std::string>{
                         "Legacy large-number roll popup is active again.",
                         "Spin result: 4"
                     },
                     has_colors(),
                     false);
    destroy_game_ui();
}

void debugEndGameScreen() {
    initialize_game_ui();
    showPopupMessage("Player 1 Wins!",
                     std::vector<std::string>{
                         "FINAL SCORE BREAKDOWN",
                         "Cash: $250000",
                         "Investments: $40000",
                         "Home Value: $80000",
                         "Loan Penalty: -$30000",
                         "Final Score: $355000",
                         "Press ENTER to return to the main menu."
                     },
                     has_colors(),
                     false);
    destroy_game_ui();
}

void debugCompletedGameHistory() {
    CompletedGameEntry entry;
    entry.date = "2026-04-28 00:00";
    entry.gameId = "DEBUG_GAME";
    entry.winner = "Debug Player";
    entry.winnerScore = 355000;
    entry.winnerCash = 250000;
    entry.rounds = 42;
    entry.players = "Debug Player:355000,CPU 1:280000";
    entry.mode = "Debug";
    std::string error;
    appendCompletedGameHistory(entry, error);
    initialize_game_ui();
    showCompletedGameHistoryScreen(has_colors());
    destroy_game_ui();
}

void debugStoryCpuText() {
    std::cout << "\n===== STORY CPU TEXT DEBUG =====\n";
    std::cout << "CPU 1 pauses for a moment, weighing quick money against a stronger future.\n";
    std::cout << "CPU 1 chooses College.\n";
    pauseForEnter();
}

void debugHangmanStages() {
    std::cout << "\n===== HANGMAN STAGES DEBUG =====\n";
    std::cout << "Hangman now uses exactly 8 wrong guesses and 9 visual states (0-8).\n";
    std::cout << "Run the Hangman minigame from the minigame debug menu for the live view.\n";
    pauseForEnter();
}

void debugHangmanBlinkInputStop() {
    std::cout << "\n===== HANGMAN BLINK INPUT DEBUG =====\n";
    std::cout << "Hangman feedback blinks twice, holds for 1 second, and ungets early input.\n";
    std::cout << "This is verified during the live Hangman minigame.\n";
    pauseForEnter();
}

void debugChoiceCards() {
    initialize_game_ui();
    choose_branch_with_selector("Choice Card Preview",
                                std::vector<std::string>{
                                    "- College: Cost high, future jobs stronger, debt risk",
                                    "- Career: Cost low, income sooner, salary ceiling lower"
                                },
                                std::vector<int>{0, 1},
                                0);
    destroy_game_ui();
}

void debugPlaytestFixes() {
    while (true) {
        std::cout << "\n===== PLAYTEST FIXES DEBUG MENU =====\n"
                  << "1. Test pre-game tutorial\n"
                  << "2. Test first-time tutorial popup\n"
                  << "3. Test Guide screen with legend\n"
                  << "4. Test title quit confirmation\n"
                  << "5. Test ESC cancel behavior\n"
                  << "6. Test sabotage unlock animation\n"
                  << "7. Test sabotage tutorial\n"
                  << "8. Test sabotage dice roll resolution\n"
                  << "9. Test sabotage duel resolution\n"
                  << "10. Test Battleship controls\n"
                  << "11. Test Battleship ammo and reload\n"
                  << "12. Test CPU end-of-turn summary wait\n"
                  << "13. Test clean stat-change turn summary\n"
                  << "14. Test Memory Match ASCII symbols\n"
                  << "15. Test Memory Match memorize countdown popup\n"
                  << "16. Test WASD and arrow key input mapping\n"
                  << "17. Test dice roll ASCII art\n"
                  << "18. Test endgame winner screen\n"
                  << "19. Test completed game history\n"
                  << "20. Test story-style CPU decision text\n"
                  << "21. Test Hangman 8-stage image\n"
                  << "22. Test Hangman blink-stop-on-input\n"
                  << "23. Test choice cards\n"
                  << "24. Return\n";

        const int choice = readMenuChoice(1, 24);
        if (choice == 1) debugPreGameTutorial();
        else if (choice == 2) debugFirstTimeTutorialPopup();
        else if (choice == 3) debugGuideScreen();
        else if (choice == 4) debugTitleQuitConfirmation();
        else if (choice == 5) debugEscCancelBehavior();
        else if (choice == 6) debugSabotageUnlockAnimation();
        else if (choice == 7) debugSabotageTutorial();
        else if (choice == 8) debugSabotageDiceResolution();
        else if (choice == 9) debugSabotageDuelResolution();
        else if (choice == 10) debugBattleshipControls();
        else if (choice == 11) debugBattleshipAmmoReload();
        else if (choice == 12) debugCPUEndTurnWait();
        else if (choice == 13) debugCleanTurnSummary();
        else if (choice == 14) debugMemoryMatchSymbols();
        else if (choice == 15) debugMemoryMatchMemorizePopup();
        else if (choice == 16) debugInputMapping();
        else if (choice == 17) debugDiceRollAsciiArt();
        else if (choice == 18) debugEndGameScreen();
        else if (choice == 19) debugCompletedGameHistory();
        else if (choice == 20) debugStoryCpuText();
        else if (choice == 21) debugHangmanStages();
        else if (choice == 22) debugHangmanBlinkInputStop();
        else if (choice == 23) debugChoiceCards();
        else return;
    }
}

void printGameSettings(const GameSettings& settings) {
    std::cout << gameSettingsSummary(settings) << "\n";
    std::cout << "Mode type: " << (settings.customMode ? "Custom" : "Preset") << "\n";
    std::cout << "Payday: " << settings.paydayMultiplierPercent
              << "% | Events +/" << settings.eventRewardMultiplierPercent
              << "% -/" << settings.eventPenaltyMultiplierPercent << "%\n";
    std::cout << "Minigame reward unit: $" << settings.minigameReward
              << " | Sabotage unlock turn: " << settings.sabotageUnlockTurn << "\n";
    std::cout << "Automatic loans: " << (settings.allowAutomaticLoans ? "ON" : "OFF")
              << " | Sabotage: " << (settings.allowSabotage ? "ON" : "OFF")
              << " | Investments: " << (settings.allowInvestments ? "ON" : "OFF")
              << " | Pets: " << (settings.allowPets ? "ON" : "OFF") << "\n";
}

void debugRelaxModeSettings() {
    std::cout << "\n===== RELAX MODE SETTINGS DEBUG =====\n";
    printGameSettings(createRelaxModeSettings());
    pauseForEnter();
}

void debugLifeModeSettings() {
    std::cout << "\n===== LIFE MODE SETTINGS DEBUG =====\n";
    printGameSettings(createLifeModeSettings());
    pauseForEnter();
}

void debugHellModeSettings() {
    std::cout << "\n===== HELL MODE SETTINGS DEBUG =====\n";
    printGameSettings(createHellModeSettings());
    pauseForEnter();
}

void debugCustomModeMenu() {
    initialize_game_ui();
    GameSettings settings = createLifeModeSettings();
    settings.customMode = true;
    settings.modeName = "Custom Mode";
    const bool started = showCustomSettingsMenu(settings, has_colors());
    destroy_game_ui();
    std::cout << "\nCustom menu result: " << (started ? "start selected" : "cancelled") << "\n";
    printGameSettings(settings);
    pauseForEnter();
}

void debugSalaryRangeCustomization() {
    std::cout << "\n===== SALARY RANGE CUSTOMIZATION DEBUG =====\n";
    GameSettings settings = createLifeModeSettings();
    settings.minJobSalary = 30000;
    settings.maxJobSalary = 45000;
    const int sourceSalary = 120000;
    validateGameSettings(settings);
    const int adjusted = std::max(settings.minJobSalary, std::min(sourceSalary, settings.maxJobSalary));
    std::cout << "Source salary: $" << sourceSalary << "\n";
    std::cout << "Custom range: $" << settings.minJobSalary << "-$" << settings.maxJobSalary << "\n";
    std::cout << "Adjusted gameplay salary: $" << adjusted << "\n";
    pauseForEnter();
}

void debugCollegeCostIndependence() {
    std::cout << "\n===== COLLEGE COST INDEPENDENCE DEBUG =====\n";
    GameSettings settings = createLifeModeSettings();
    const int originalCollegeCost = settings.collegeCost;
    settings.minJobSalary = 10000;
    settings.maxJobSalary = 30000;
    validateGameSettings(settings);
    std::cout << "Changed salary range to $" << settings.minJobSalary
              << "-$" << settings.maxJobSalary << "\n";
    std::cout << "College cost stayed at $" << settings.collegeCost << "\n";
    std::cout << (settings.collegeCost == originalCollegeCost ? "PASS" : "FAIL") << "\n";
    pauseForEnter();
}

void debugTerminalResizeRecovery() {
    std::cout << "\n===== TERMINAL RESIZE RECOVERY DEBUG =====\n";
    std::cout << "Required size: " << minimumGameWidth() << " x " << minimumGameHeight() << "\n";
    std::cout << "In game, KEY_RESIZE now destroys old windows, recreates layout, and redraws current state.\n";
    pauseForEnter();
}

void debugGameSettingsSaveLoad() {
    std::cout << "\n===== GAME SETTINGS SAVE/LOAD DEBUG =====\n";
    Game game;
    SaveManager saveManager;
    std::string error;
    const std::string filename = "debug_settings_test.sav";
    if (!saveManager.saveGame(game, filename, error)) {
        std::cout << "Save failed: " << error << "\n";
        pauseForEnter();
        return;
    }

    const std::string path = saveManager.resolvePath(filename);
    std::ifstream in(path.c_str());
    std::string line;
    bool sawSettings = false;
    while (std::getline(in, line)) {
        if (line.find("GAME_SETTINGS") == 0) {
            sawSettings = true;
            break;
        }
    }

    std::cout << "Settings record written: " << (sawSettings ? "yes" : "no") << "\n";
    std::cout << "Save path: " << path << "\n";
    std::cout << "Load path uses the same GAME_SETTINGS records in SaveManager::loadGame.\n";
    pauseForEnter();
}

void debugRevertedDiceAnimation() {
    initialize_game_ui();
    showPopupMessage("YOU ROLLED",
                     std::vector<std::string>{
                         "Legacy large-number roll popup is active again.",
                         "The main game no longer uses the newer dice-face animation.",
                         "Spin result: 4"
                     },
                     has_colors(),
                     false);
    destroy_game_ui();
}

void debugGameSettingsMenu() {
    while (true) {
        std::cout << "\n===== GAME SETTINGS DEBUG MENU =====\n"
                  << "1. Test Relax Mode settings\n"
                  << "2. Test Life Mode settings\n"
                  << "3. Test Hell Mode settings\n"
                  << "4. Test Custom Mode menu\n"
                  << "5. Test salary range customization\n"
                  << "6. Test college cost stays independent\n"
                  << "7. Test terminal resize recovery\n"
                  << "8. Test game settings save/load\n"
                  << "9. Test reverted dice animation\n"
                  << "10. Return\n";

        const int choice = readMenuChoice(1, 10);
        if (choice == 1) debugRelaxModeSettings();
        else if (choice == 2) debugLifeModeSettings();
        else if (choice == 3) debugHellModeSettings();
        else if (choice == 4) debugCustomModeMenu();
        else if (choice == 5) debugSalaryRangeCustomization();
        else if (choice == 6) debugCollegeCostIndependence();
        else if (choice == 7) debugTerminalResizeRecovery();
        else if (choice == 8) debugGameSettingsSaveLoad();
        else if (choice == 9) debugRevertedDiceAnimation();
        else return;
    }
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
                  << "9. Test UI / pacing features\n"
                  << "10. Test board UI features\n"
                  << "11. Test playtest fixes\n"
                  << "12. Test game settings\n"
                  << "13. Exit\n";

        const int choice = readMenuChoice(1, 13);
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
                debugUiPacing();
                break;
            case 10:
                debugBoardUi();
                break;
            case 11:
                debugPlaytestFixes();
                break;
            case 12:
                debugGameSettingsMenu();
                break;
            case 13:
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
