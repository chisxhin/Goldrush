#include "minigame_tutorials.h"

#include "ui_helpers.h"

#include <string>
#include <vector>

void showMinigameTutorial(const std::string& title,
                          const std::string& objective,
                          const std::string& controls,
                          const std::string& winCondition,
                          const std::string& rewardPenalty,
                          bool hasColor) {
    std::vector<std::string> lines;
    lines.push_back("Minigame: " + title);
    lines.push_back("");
    lines.push_back("Objective: " + objective);
    lines.push_back("Controls: " + controls);
    lines.push_back("Win Condition: " + winCondition);
    lines.push_back("Reward/Penalty: " + rewardPenalty);
    lines.push_back("");
    lines.push_back("Sabotage note: Forced Duel and trap minigames use this screen before play.");
    lines.push_back("Investment note: normal spinner investment payouts resume after the minigame.");
    showPopupMessage("QUICK TUTORIAL", lines, hasColor, false);
}
