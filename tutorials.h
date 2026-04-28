#pragma once

#include <ncurses.h>

#include <string>
#include <vector>

#include "board.hpp"
#include "rules.hpp"

struct TutorialFlags {
    bool automaticLoan = false;
    bool manualLoan = false;
    bool investment = false;
    bool baby = false;
    bool pet = false;
    bool marriage = false;
    bool insurance = false;
    bool shield = false;
    bool actionCard = false;
    bool minigame = false;
    bool sabotage = false;
};

enum class TutorialTopic {
    AutomaticLoan,
    ManualLoan,
    Investment,
    Baby,
    Pet,
    Marriage,
    Insurance,
    Shield,
    ActionCard,
    Minigame,
    Sabotage
};

bool& tutorialFlagForTopic(TutorialFlags& flags, TutorialTopic topic);
std::string tutorialTitle(TutorialTopic topic);
std::vector<std::string> tutorialLines(TutorialTopic topic);

void showPagedGuide(const std::string& title,
                    const std::vector<std::vector<std::string> >& pages,
                    bool hasColor);
void showPreGameQuickGuide(bool hasColor);
void showFirstTimeTutorial(TutorialTopic topic, bool hasColor);
void showFullGuide(const Board& board, const RuleSet& rules, bool sabotageUnlocked, bool hasColor);
bool showQuitConfirmation(bool hasColor);
void showSabotageUnlockAnimation(bool hasColor);
void showSabotageTutorial(bool hasColor);
