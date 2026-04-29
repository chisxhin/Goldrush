#include "bank.hpp"

#include <algorithm>

//Input: RuleSet object (rules)
//Output: none
//Purpose: sets up the Bank with the provided ruleset
Bank::Bank(const RuleSet& rules)
    : ruleset(rules) {
}

//Input: RuleSet object (rules)
//Output: none
//Purpose: updates the Bank's ruleset to the new one
void Bank::configure(const RuleSet& rules) {
    ruleset = rules;
}

//Input: Player reference (in form of "player"), amount(in form of integer)
//Output: none
//Purpose: adds money to player's cash if amount [<(smaller than) or =(equal)] 0
void Bank::credit(Player& player, int amount) const {
    if (amount <= 0) {
        return;
    }
    player.cash += amount;
}

// Input: Player reference (in form of "player"), amount(in form of integer)
// Output: PaymentResult struct (contains charged amount and loans taken)
// Purpose: deducts money from player's cash; if cash goes negative,
//          automatically issues loans (if allowed by ruleset) until cash is non-neg
PaymentResult Bank::charge(Player& player, int amount) const {
    PaymentResult result;
    result.charged = amount;
    result.loansTaken = 0;

    if (amount <= 0) {
        return result;
    }

    player.cash -= amount;
    // If player goes negative, issue automatic loans until cash >= 0 or max loans reached
    while (player.cash < 0 &&
           ruleset.automaticLoansEnabled &&
           player.loans < ruleset.maxLoans) {
        player.cash += ruleset.loanUnit;
        ++player.loans;
        ++result.loansTaken;
    }
    return result;
}

// Input: Player reference (in form of "player"), amount requested(in form of integer)
// Output: number of loans actually issued(in form of integer)
// Purpose: issues loans to player in multiples of loanUnit, capped by maxLoans if defined
int Bank::issueLoan(Player& player, int amount) const {
    if (amount <= 0) {
        return 0;
    }

    int loansIssued = amount / ruleset.loanUnit;
    if (amount % ruleset.loanUnit != 0) {
        ++loansIssued;
    }
    if (ruleset.maxLoans >= 0) {
        loansIssued = std::min(loansIssued, std::max(0, ruleset.maxLoans - player.loans));
    }

    player.loans += loansIssued;
    player.cash += loansIssued * ruleset.loanUnit;
    return loansIssued;
}

// Input: Player reference (in form of "player")
// Output: bool (true if repayment succeeded, false otherwise)
// Purpose: repays one loan if player has loans and enough cash
bool Bank::repayOneLoan(Player& player) const {
    if (player.loans <= 0) {
        return false;
    }
    if (player.cash < ruleset.loanRepaymentCost) {
        return false;
    }
    player.cash -= ruleset.loanRepaymentCost;
    --player.loans;
    return true;
}

// Input: const Player reference (in form of "player")
// Output: total debt amount(in form of integer)
// Purpose: calculates total debt = number of loans * repayment cost
int Bank::totalLoanDebt(const Player& player) const {
    return player.loans * ruleset.loanRepaymentCost;
}
