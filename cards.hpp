#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "deck.hpp"
#include "random_service.hpp"
#include "rules.hpp"

enum CardCategory {
    CARD_ACTION,
    CARD_CAREER,
    CARD_HOUSE,
    CARD_INVEST,
    CARD_PET
};

enum ActionEffectKind {
    ACTION_NO_EFFECT,
    ACTION_GAIN_CASH,
    ACTION_PAY_CASH,
    ACTION_GAIN_PER_KID,
    ACTION_PAY_PER_KID,
    ACTION_GAIN_SALARY_BONUS,
    ACTION_BONUS_IF_MARRIED,
    ACTION_MOVE_SPACES
};

enum RollConditionKind {
    ROLL_ANY,
    ROLL_ODD,
    ROLL_EVEN,
    ROLL_RANGE,
    ROLL_EXACT
};

struct RollCondition {
    RollConditionKind kind;
    int minValue;
    int maxValue;
    int exactValue;
};

struct ActionEffect {
    ActionEffectKind kind;
    int amount;
    bool useTileValue;
};

struct ActionRollOutcome {
    RollCondition condition;
    ActionEffect effect;
    std::string text;
};

struct ActionCard {
    std::string id;
    std::string title;
    CardCategory category;
    std::string description;
    ActionEffect effect;
    std::vector<ActionRollOutcome> rollOutcomes;
    bool keepAfterUse;
};

struct CareerCard {
    std::string id;
    std::string title;
    CardCategory category;
    std::string description;
    int salary;
    bool requiresDegree;
    bool keepAfterUse;
};

struct HouseCard {
    std::string id;
    std::string title;
    CardCategory category;
    std::string description;
    int cost;
    int saleValue;
    bool keepAfterUse;
};

struct InvestCard {
    std::string id;
    std::string title;
    CardCategory category;
    std::string description;
    int number;
    int payout;
    bool keepAfterUse;
};

struct PetCard {
    std::string id;
    std::string title;
    CardCategory category;
    std::string description;
    int endValue;
    bool keepAfterUse;
};

class DeckManager {
public:
    DeckManager(const RuleSet& rules, RandomService& random);

    void reset(const RuleSet& rules, bool reshuffle = true);
    const RuleSet& rules() const;

    bool drawActionCard(ActionCard& card);
    void resolveActionCard(const ActionCard& card, bool keepCard);
    std::vector<CareerCard> drawCareerChoices(bool requiresDegree, int count);
    void resolveCareerChoices(bool requiresDegree,
                              const std::vector<CareerCard>& choices,
                              int keptIndex);
    bool drawHouseCard(HouseCard& card);
    bool drawInvestCard(InvestCard& card);
    bool drawPetCard(PetCard& card);

private:
    RuleSet ruleset;
    Deck<ActionCard> actionDeck;
    Deck<CareerCard> collegeCareerDeck;
    Deck<CareerCard> careerDeck;
    Deck<HouseCard> houseDeck;
    Deck<InvestCard> investDeck;
    Deck<PetCard> petDeck;

    void initDecks(bool reshuffle = true);
    void initActionDeck(bool reshuffle);
    void initCareerDecks(bool reshuffle);
    void initHouseDeck(bool reshuffle);
    void initInvestDeck(bool reshuffle);
    void initPetDeck(bool reshuffle);
};

bool actionCardUsesRoll(const ActionCard& card);
bool matchesRollCondition(const RollCondition& condition, int roll);
const ActionRollOutcome* findMatchingRollOutcome(const ActionCard& card, int roll);
std::string describeRollCondition(const RollCondition& condition);
