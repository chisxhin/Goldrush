#include "minigame_tutorials.h"

#include "ui_helpers.h"

#include <string>
#include <vector>

//Input: title (string), objective (string), controls (string), winCondition (string), rewardPenalty (string), hasColor (bool)
//Output: none
//Purpose: builds a quick tutorial popup message for a minigame, showing its title, objective, controls, win condition and reward/penalty information.
//Relation: constructs a vector of tutorial lines and passes them to showPopupMessage func, which handles the actual UI display.
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
    showPopupMessage("QUICK TUTORIAL", lines, hasColor, false);
}
