#pragma once

#include <string>

//Input: title (string), objective (string), controls (string), winCondition (string), rewardPenalty (string), hasColor (bool)
//Output: none
//Purpose: displays a quick tutorial popup for a minigame and summarize its title, objective, controls, win condition, and reward/penalty information.
//Relation: prepares tutorial text and delegates the actual UI rendering to showPopupMessage func 
void showMinigameTutorial(const std::string& title,
                          const std::string& objective,
                          const std::string& controls,
                          const std::string& winCondition,
                          const std::string& rewardPenalty,
                          bool hasColor);
