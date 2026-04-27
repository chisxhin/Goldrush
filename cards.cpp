#include "cards.hpp"

#include <map>
#include <set>
#include <vector>

namespace {
template <typename T>
std::vector<T> expandDeck(const std::vector<T>& prototypes, int desiredCount) {
    std::vector<T> cards;
    if (desiredCount <= 0 || prototypes.empty()) {
        return cards;
    }

    cards.reserve(desiredCount);
    std::vector<int> instanceCounts(prototypes.size(), 0);

    for (int i = 0; i < desiredCount; ++i) {
        const std::size_t prototypeIndex = static_cast<std::size_t>(i % prototypes.size());
        T card = prototypes[prototypeIndex];
        ++instanceCounts[prototypeIndex];
        card.id += "-" + std::to_string(instanceCounts[prototypeIndex]);
        cards.push_back(card);
    }
    return cards;
}

ActionEffect makeActionEffect(ActionEffectKind kind, int amount, bool useTileValue) {
    ActionEffect effect;
    effect.kind = kind;
    effect.amount = amount;
    effect.useTileValue = useTileValue;
    return effect;
}

RollCondition makeAnyCondition() {
    RollCondition condition;
    condition.kind = ROLL_ANY;
    condition.minValue = 0;
    condition.maxValue = 0;
    condition.exactValue = 0;
    return condition;
}

RollCondition makeOddCondition() {
    RollCondition condition = makeAnyCondition();
    condition.kind = ROLL_ODD;
    return condition;
}

RollCondition makeEvenCondition() {
    RollCondition condition = makeAnyCondition();
    condition.kind = ROLL_EVEN;
    return condition;
}

RollCondition makeRangeCondition(int minValue, int maxValue) {
    RollCondition condition = makeAnyCondition();
    condition.kind = ROLL_RANGE;
    condition.minValue = minValue;
    condition.maxValue = maxValue;
    return condition;
}

RollCondition makeExactCondition(int exactValue) {
    RollCondition condition = makeAnyCondition();
    condition.kind = ROLL_EXACT;
    condition.exactValue = exactValue;
    return condition;
}

ActionRollOutcome makeRollOutcome(const RollCondition& condition,
                                  const ActionEffect& effect,
                                  const std::string& text) {
    ActionRollOutcome outcome;
    outcome.condition = condition;
    outcome.effect = effect;
    outcome.text = text;
    return outcome;
}

ActionCard makeActionCard(const std::string& id,
                          const std::string& title,
                          const std::string& description,
                          const ActionEffect& effect,
                          bool keepAfterUse) {
    ActionCard card;
    card.id = id;
    card.title = title;
    card.category = CARD_ACTION;
    card.description = description;
    card.effect = effect;
    card.keepAfterUse = keepAfterUse;
    return card;
}

ActionCard makeRollActionCard(const std::string& id,
                              const std::string& title,
                              const std::string& description,
                              const std::vector<ActionRollOutcome>& outcomes,
                              bool keepAfterUse) {
    ActionCard card = makeActionCard(id, title, description, makeActionEffect(ACTION_NO_EFFECT, 0, false), keepAfterUse);
    card.rollOutcomes = outcomes;
    return card;
}

std::vector<ActionCard> actionPrototypes() {
    std::vector<ActionCard> cards;
    cards.push_back(makeActionCard("action-tax-refund", "Tax Refund",
                                   "Collect a surprise refund from the bank.",
                                   makeActionEffect(ACTION_GAIN_CASH, 25000, true), true));
    cards.push_back(makeActionCard("action-car-trouble", "Car Trouble",
                                   "Pay for a major repair bill.",
                                   makeActionEffect(ACTION_PAY_CASH, 20000, true), false));
    cards.push_back(makeActionCard("action-family-picnic", "Family Picnic",
                                   "Collect $10K per child for the photo shoot.",
                                   makeActionEffect(ACTION_GAIN_PER_KID, 10000, false), true));
    cards.push_back(makeActionCard("action-school-supplies", "School Supplies",
                                   "Pay $8K per child for school supplies.",
                                   makeActionEffect(ACTION_PAY_PER_KID, 8000, false), false));
    cards.push_back(makeActionCard("action-side-hustle", "Side Hustle",
                                   "Your side gig boosts your salary by $10K.",
                                   makeActionEffect(ACTION_GAIN_SALARY_BONUS, 10000, false), true));
    cards.push_back(makeActionCard("action-anniversary-bonus", "Anniversary Bonus",
                                   "If married, collect $30K from the bank.",
                                   makeActionEffect(ACTION_BONUS_IF_MARRIED, 30000, false), true));
    cards.push_back(makeActionCard("action-lucky-day", "Lucky Day",
                                   "Collect a sudden windfall.",
                                   makeActionEffect(ACTION_GAIN_CASH, 40000, true), true));
    cards.push_back(makeActionCard("action-unexpected-bill", "Unexpected Bill",
                                   "Pay an emergency household expense.",
                                   makeActionEffect(ACTION_PAY_CASH, 30000, true), false));
    cards.push_back(makeActionCard("action-duel-minigame", "Duel Minigame Card",
                                   "Draw a random opponent, play a minigame, and win money if your score is higher.",
                                   makeActionEffect(ACTION_DUEL_MINIGAME, 0, false), false));

    std::vector<ActionRollOutcome> casinoNight;
    casinoNight.push_back(makeRollOutcome(makeOddCondition(),
                                          makeActionEffect(ACTION_GAIN_CASH, 50000, false),
                                          "Odd roll"));
    casinoNight.push_back(makeRollOutcome(makeEvenCondition(),
                                          makeActionEffect(ACTION_PAY_CASH, 50000, false),
                                          "Even roll"));
    cards.push_back(makeRollActionCard("action-casino-night", "Casino Night",
                                       "Spin the wheel. Odd collects $50K, even pays $50K.",
                                       casinoNight, false));

    std::vector<ActionRollOutcome> riskyMove;
    riskyMove.push_back(makeRollOutcome(makeRangeCondition(1, 5),
                                        makeActionEffect(ACTION_MOVE_SPACES, -2, false),
                                        "Roll 1-5"));
    riskyMove.push_back(makeRollOutcome(makeRangeCondition(6, 10),
                                        makeActionEffect(ACTION_MOVE_SPACES, 2, false),
                                        "Roll 6-10"));
    cards.push_back(makeRollActionCard("action-risky-move", "Risky Move",
                                       "Spin the wheel. Low rolls move back 2 spaces, high rolls move forward 2.",
                                       riskyMove, false));

    std::vector<ActionRollOutcome> jackpot;
    jackpot.push_back(makeRollOutcome(makeExactCondition(10),
                                      makeActionEffect(ACTION_GAIN_CASH, 200000, false),
                                      "Exact 10"));
    jackpot.push_back(makeRollOutcome(makeRangeCondition(6, 9),
                                      makeActionEffect(ACTION_GAIN_CASH, 100000, false),
                                      "Roll 6-9"));
    jackpot.push_back(makeRollOutcome(makeAnyCondition(),
                                      makeActionEffect(ACTION_NO_EFFECT, 0, false),
                                      "Roll 1-5"));
    cards.push_back(makeRollActionCard("action-jackpot", "Jackpot",
                                       "10 pays $200K, 6-9 pays $100K, and 1-5 pays nothing.",
                                       jackpot, true));

    std::vector<ActionRollOutcome> trafficTrouble;
    trafficTrouble.push_back(makeRollOutcome(makeOddCondition(),
                                             makeActionEffect(ACTION_PAY_CASH, 50000, false),
                                             "Odd roll"));
    trafficTrouble.push_back(makeRollOutcome(makeEvenCondition(),
                                             makeActionEffect(ACTION_NO_EFFECT, 0, false),
                                             "Even roll"));
    cards.push_back(makeRollActionCard("action-traffic-trouble", "Traffic Trouble",
                                       "Odd rolls pay $50K in delays. Even rolls escape the jam.",
                                       trafficTrouble, false));
    return cards;
}

std::vector<CareerCard> collegeCareerPrototypes() {
    std::vector<CareerCard> cards;
    cards.push_back({"career-doctor", "Doctor", CARD_CAREER,
                     "Long degree path, strong salary.",
                     80000, true, true});
    cards.push_back({"career-architect", "Architect", CARD_CAREER,
                     "Design work with steady pay.",
                     70000, true, true});
    cards.push_back({"career-scientist", "Scientist", CARD_CAREER,
                     "Research role with good growth.",
                     75000, true, true});
    cards.push_back({"career-engineer", "Engineer", CARD_CAREER,
                     "Problem solving career with reliable income.",
                     72000, true, true});
    cards.push_back({"career-professor", "Professor", CARD_CAREER,
                     "Academic path with good pay.",
                     68000, true, true});
    return cards;
}

std::vector<CareerCard> careerPrototypes() {
    std::vector<CareerCard> cards;
    cards.push_back({"career-chef", "Chef", CARD_CAREER,
                     "Fast-paced kitchen career.",
                     42000, false, true});
    cards.push_back({"career-photographer", "Photographer", CARD_CAREER,
                     "Creative work on the go.",
                     38000, false, true});
    cards.push_back({"career-mechanic", "Mechanic", CARD_CAREER,
                     "Hands-on technical career.",
                     43000, false, true});
    cards.push_back({"career-sales-rep", "Sales Rep", CARD_CAREER,
                     "People-first role with commissions.",
                     41000, false, true});
    cards.push_back({"career-developer", "Developer", CARD_CAREER,
                     "Technical work with steady income.",
                     46000, false, true});
    return cards;
}

std::vector<HouseCard> housePrototypes() {
    std::vector<HouseCard> cards;
    cards.push_back({"house-lake-cabin", "Lake Cabin", CARD_HOUSE,
                     "Quiet retreat by the water.",
                     50000, 90000, true});
    cards.push_back({"house-townhouse", "Townhouse", CARD_HOUSE,
                     "Compact city home with good resale value.",
                     60000, 95000, true});
    cards.push_back({"house-hilltop-villa", "Hilltop Villa", CARD_HOUSE,
                     "Big views and a higher buy-in.",
                     80000, 130000, true});
    cards.push_back({"house-starter-home", "Starter Home", CARD_HOUSE,
                     "Affordable first house.",
                     45000, 75000, true});
    cards.push_back({"house-beach-house", "Beach House", CARD_HOUSE,
                     "Premium location with a big upside.",
                     90000, 140000, true});
    return cards;
}

std::vector<InvestCard> investPrototypes(const RuleSet& rules) {
    std::vector<InvestCard> cards;
    cards.push_back({"invest-3", "Invest on 3", CARD_INVEST,
                     "Collect when any player spins a 3.",
                     3, rules.investmentMatchPayout, true});
    cards.push_back({"invest-5", "Invest on 5", CARD_INVEST,
                     "Collect when any player spins a 5.",
                     5, rules.investmentMatchPayout, true});
    cards.push_back({"invest-7", "Invest on 7", CARD_INVEST,
                     "Collect when any player spins a 7.",
                     7, rules.investmentMatchPayout, true});
    cards.push_back({"invest-9", "Invest on 9", CARD_INVEST,
                     "Collect when any player spins a 9.",
                     9, rules.investmentMatchPayout, true});
    return cards;
}

std::vector<PetCard> petPrototypes() {
    std::vector<PetCard> cards;
    cards.push_back({"pet-dog", "Dog", CARD_PET,
                     "A loyal family dog joins the car.",
                     100000, true});
    cards.push_back({"pet-cat", "Cat", CARD_PET,
                     "A calm cat settles into the household.",
                     100000, true});
    cards.push_back({"pet-rabbit", "Rabbit", CARD_PET,
                     "A tiny pet with a big personality.",
                     100000, true});
    cards.push_back({"pet-parrot", "Parrot", CARD_PET,
                     "A chatty bird becomes part of the trip.",
                     100000, true});
    return cards;
}

template <typename T>
SerializedDeckState serializeDeckState(const Deck<T>& deck) {
    SerializedDeckState state;
    const std::vector<T>& drawCards = deck.drawCards();
    const std::vector<T>& discardCards = deck.discardCards();

    state.drawIds.reserve(drawCards.size());
    for (std::size_t i = 0; i < drawCards.size(); ++i) {
        state.drawIds.push_back(drawCards[i].id);
    }

    state.discardIds.reserve(discardCards.size());
    for (std::size_t i = 0; i < discardCards.size(); ++i) {
        state.discardIds.push_back(discardCards[i].id);
    }
    return state;
}

template <typename T>
std::map<std::string, T> buildCardIndex(const std::vector<T>& cards) {
    std::map<std::string, T> index;
    for (std::size_t i = 0; i < cards.size(); ++i) {
        index[cards[i].id] = cards[i];
    }
    return index;
}

template <typename T>
bool restoreDeckStateInternal(Deck<T>& deck,
                              const std::vector<T>& allCards,
                              const SerializedDeckState& state,
                              std::string& error) {
    const std::map<std::string, T> index = buildCardIndex(allCards);
    std::vector<T> drawCards;
    std::vector<T> discardCards;
    std::set<std::string> seenIds;

    // Save files store deck contents by card id. On load we rebuild the exact
    // physical card instances from the current ruleset and then map ids back to
    // real card objects, while rejecting duplicates and unknown ids.
    drawCards.reserve(state.drawIds.size());
    for (std::size_t i = 0; i < state.drawIds.size(); ++i) {
        const typename std::map<std::string, T>::const_iterator found = index.find(state.drawIds[i]);
        if (found == index.end()) {
            error = "Unknown card id in draw pile: " + state.drawIds[i];
            return false;
        }
        if (!seenIds.insert(state.drawIds[i]).second) {
            error = "Duplicate card id in deck state: " + state.drawIds[i];
            return false;
        }
        drawCards.push_back(found->second);
    }

    discardCards.reserve(state.discardIds.size());
    for (std::size_t i = 0; i < state.discardIds.size(); ++i) {
        const typename std::map<std::string, T>::const_iterator found = index.find(state.discardIds[i]);
        if (found == index.end()) {
            error = "Unknown card id in discard pile: " + state.discardIds[i];
            return false;
        }
        if (!seenIds.insert(state.discardIds[i]).second) {
            error = "Duplicate card id in deck state: " + state.discardIds[i];
            return false;
        }
        discardCards.push_back(found->second);
    }

    deck.setState(drawCards, discardCards);
    return true;
}

}

bool actionCardUsesRoll(const ActionCard& card) {
    return !card.rollOutcomes.empty();
}

bool matchesRollCondition(const RollCondition& condition, int roll) {
    switch (condition.kind) {
        case ROLL_ANY:
            return true;
        case ROLL_ODD:
            return RandomService::isOdd(roll);
        case ROLL_EVEN:
            return RandomService::isEven(roll);
        case ROLL_RANGE:
            return RandomService::inRange(roll, condition.minValue, condition.maxValue);
        case ROLL_EXACT:
            return roll == condition.exactValue;
        default:
            return false;
    }
}

const ActionRollOutcome* findMatchingRollOutcome(const ActionCard& card, int roll) {
    for (std::size_t i = 0; i < card.rollOutcomes.size(); ++i) {
        if (matchesRollCondition(card.rollOutcomes[i].condition, roll)) {
            return &card.rollOutcomes[i];
        }
    }
    return 0;
}

std::string describeRollCondition(const RollCondition& condition) {
    switch (condition.kind) {
        case ROLL_ANY:
            return "Default";
        case ROLL_ODD:
            return "Odd roll";
        case ROLL_EVEN:
            return "Even roll";
        case ROLL_RANGE:
            return "Roll " + std::to_string(condition.minValue) + "-" + std::to_string(condition.maxValue);
        case ROLL_EXACT:
            return "Roll " + std::to_string(condition.exactValue);
        default:
            return "Unknown roll";
    }
}

DeckManager::DeckManager(const RuleSet& rules, RandomService& randomService)
    : ruleset(rules),
      actionDeck(randomService),
      collegeCareerDeck(randomService),
      careerDeck(randomService),
      houseDeck(randomService),
      investDeck(randomService),
      petDeck(randomService) {
    initDecks(true);
}

void DeckManager::reset(const RuleSet& rules, bool reshuffle) {
    ruleset = rules;
    initDecks(reshuffle);
}

const RuleSet& DeckManager::rules() const {
    return ruleset;
}

bool DeckManager::drawActionCard(ActionCard& card) {
    return actionDeck.draw(card);
}

void DeckManager::resolveActionCard(const ActionCard& card, bool keepCard) {
    if (keepCard) {
        return;
    }
    actionDeck.discard(card);
}

std::vector<CareerCard> DeckManager::drawCareerChoices(bool requiresDegree, int count) {
    std::vector<CareerCard> choices;
    if (count <= 0) {
        return choices;
    }

    choices.reserve(static_cast<std::size_t>(count));
    Deck<CareerCard>& deck = requiresDegree ? collegeCareerDeck : careerDeck;
    CareerCard card;

    for (int i = 0; i < count; ++i) {
        if (!deck.draw(card)) {
            break;
        }
        choices.push_back(card);
    }
    return choices;
}

void DeckManager::resolveCareerChoices(bool requiresDegree,
                                       const std::vector<CareerCard>& choices,
                                       int keptIndex) {
    Deck<CareerCard>& deck = requiresDegree ? collegeCareerDeck : careerDeck;
    for (std::size_t i = 0; i < choices.size(); ++i) {
        if (static_cast<int>(i) == keptIndex) {
            continue;
        }
        deck.discard(choices[i]);
    }
}

bool DeckManager::drawHouseCard(HouseCard& card) {
    return houseDeck.draw(card);
}

bool DeckManager::drawInvestCard(InvestCard& card) {
    return investDeck.draw(card);
}

bool DeckManager::drawPetCard(PetCard& card) {
    return petDeck.draw(card);
}

SerializedDeckState DeckManager::deckState(DeckSlot slot) const {
    switch (slot) {
        case DECK_ACTION:
            return serializeDeckState(actionDeck);
        case DECK_COLLEGE_CAREER:
            return serializeDeckState(collegeCareerDeck);
        case DECK_CAREER:
            return serializeDeckState(careerDeck);
        case DECK_HOUSE:
            return serializeDeckState(houseDeck);
        case DECK_INVEST:
            return serializeDeckState(investDeck);
        case DECK_PET:
            return serializeDeckState(petDeck);
        default:
            return SerializedDeckState();
    }
}

bool DeckManager::restoreDeckState(DeckSlot slot,
                                   const SerializedDeckState& state,
                                   std::string& error) {
    // Each slot regenerates its full card pool first so save files only need to
    // persist ids, not a full serialized copy of every card definition.
    switch (slot) {
        case DECK_ACTION:
            return restoreDeckStateInternal(
                actionDeck,
                expandDeck(actionPrototypes(), ruleset.components.actionCards),
                state,
                error);
        case DECK_COLLEGE_CAREER:
            return restoreDeckStateInternal(
                collegeCareerDeck,
                expandDeck(collegeCareerPrototypes(), ruleset.components.collegeCareerCards),
                state,
                error);
        case DECK_CAREER:
            return restoreDeckStateInternal(
                careerDeck,
                expandDeck(careerPrototypes(), ruleset.components.careerCards),
                state,
                error);
        case DECK_HOUSE:
            return restoreDeckStateInternal(
                houseDeck,
                expandDeck(housePrototypes(), ruleset.components.houseCards),
                state,
                error);
        case DECK_INVEST:
            return restoreDeckStateInternal(
                investDeck,
                expandDeck(investPrototypes(ruleset), ruleset.components.investCards),
                state,
                error);
        case DECK_PET:
            return restoreDeckStateInternal(
                petDeck,
                expandDeck(petPrototypes(), ruleset.components.petCards),
                state,
                error);
        default:
            error = "Unknown deck slot.";
            return false;
    }
}

void DeckManager::initDecks(bool reshuffle) {
    initActionDeck(reshuffle);
    initCareerDecks(reshuffle);
    initHouseDeck(reshuffle);
    initInvestDeck(reshuffle);
    initPetDeck(reshuffle);
}

void DeckManager::initActionDeck(bool reshuffle) {
    actionDeck.reset(expandDeck(actionPrototypes(), ruleset.components.actionCards), reshuffle);
}

void DeckManager::initCareerDecks(bool reshuffle) {
    collegeCareerDeck.reset(
        expandDeck(collegeCareerPrototypes(), ruleset.components.collegeCareerCards),
        reshuffle);
    careerDeck.reset(
        expandDeck(careerPrototypes(), ruleset.components.careerCards),
        reshuffle);
}

void DeckManager::initHouseDeck(bool reshuffle) {
    houseDeck.reset(expandDeck(housePrototypes(), ruleset.components.houseCards), reshuffle);
}

void DeckManager::initInvestDeck(bool reshuffle) {
    investDeck.reset(expandDeck(investPrototypes(ruleset), ruleset.components.investCards), reshuffle);
}

void DeckManager::initPetDeck(bool reshuffle) {
    petDeck.reset(expandDeck(petPrototypes(), ruleset.components.petCards), reshuffle);
}
