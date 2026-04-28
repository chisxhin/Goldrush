#include "rules.hpp"

namespace {
RuleSet makeBaseRules(const std::string& name) {
    RuleSet rules;
    rules.editionName = name;
    rules.toggles.petsEnabled = true;
    rules.toggles.investmentEnabled = true;
    rules.toggles.spinToWinEnabled = true;
    rules.toggles.electronicBankingEnabled = true;
    rules.toggles.familyPathEnabled = true;
    rules.toggles.riskyRouteEnabled = true;
    rules.toggles.nightSchoolEnabled = true;
    rules.toggles.tutorialEnabled = true;
    rules.toggles.houseSaleSpinEnabled = true;
    rules.toggles.retirementBonusesEnabled = true;
    rules.components.actionCards = 55;
    rules.components.collegeCareerCards = 10;
    rules.components.careerCards = 10;
    rules.components.houseCards = 11;
    rules.components.investCards = 4;
    rules.components.petCards = 12;
    rules.components.spinToWinTokens = 5;
    rules.loanUnit = 50000;
    rules.loanRepaymentCost = 60000;
    rules.maxLoans = 99;
    rules.automaticLoansEnabled = true;
    rules.investmentMatchPayout = 25000;
    rules.spinToWinPrize = 25000;
    return rules;
}
}

RuleSet makeNormalRules() {
    return makeBaseRules("Normal Mode");
}

RuleSet makeCustomRules() {
    return makeBaseRules("Custom Mode");
}
