#pragma once

#include <string>

void showMinigameTutorial(const std::string& title,
                          const std::string& objective,
                          const std::string& controls,
                          const std::string& winCondition,
                          const std::string& rewardPenalty,
                          bool hasColor);
