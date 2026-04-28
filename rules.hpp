#pragma once

#include <string>

struct RuleToggles {
    bool petsEnabled;
    bool investmentEnabled;
    bool spinToWinEnabled;
    bool electronicBankingEnabled;
    bool familyPathEnabled;
    bool riskyRouteEnabled;
    bool nightSchoolEnabled;
    bool tutorialEnabled;
    bool houseSaleSpinEnabled;
    bool retirementBonusesEnabled;
};

struct ComponentSet {
    int actionCards;
    int collegeCareerCards;
    int careerCards;
    int houseCards;
    int investCards;
    int petCards;
    int spinToWinTokens;
};

struct RuleSet {
    std::string editionName;
    RuleToggles toggles;
    ComponentSet components;
    int loanUnit;
    int loanRepaymentCost;
    int maxLoans;
    bool automaticLoansEnabled;
    int investmentMatchPayout;
    int spinToWinPrize;
};

RuleSet makeNormalRules();
RuleSet makeCustomRules();
