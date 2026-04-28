#include "tutorials.h"

#include <algorithm>

#include "input_helpers.h"
#include "ui.h"
#include "ui_helpers.h"

namespace {
WINDOW* centeredPopup(int height, int width) {
    int screenH = 0;
    int screenW = 0;
    getmaxyx(stdscr, screenH, screenW);
    const int popupH = std::min(height, std::max(8, screenH - 2));
    const int popupW = std::min(width, std::max(40, screenW - 2));
    WINDOW* popup = createCenteredWindow(popupH, popupW, 8, 40);
    if (!popup) {
        showTerminalSizeWarning(8, 40, has_colors());
        return nullptr;
    }
    keypad(popup, TRUE);
    return popup;
}

void drawCentered(WINDOW* win, int y, const std::string& text, int attrs = A_NORMAL) {
    int height = 0;
    int width = 0;
    getmaxyx(win, height, width);
    (void)height;
    const std::string clipped = clipUiText(text, static_cast<std::size_t>(std::max(1, width - 2)));
    if (attrs != A_NORMAL) {
        wattron(win, attrs);
    }
    mvwprintw(win, y, std::max(1, (width - static_cast<int>(clipped.size())) / 2), "%s", clipped.c_str());
    if (attrs != A_NORMAL) {
        wattroff(win, attrs);
    }
}
}

bool& tutorialFlagForTopic(TutorialFlags& flags, TutorialTopic topic) {
    switch (topic) {
        case TutorialTopic::AutomaticLoan: return flags.automaticLoan;
        case TutorialTopic::ManualLoan: return flags.manualLoan;
        case TutorialTopic::Investment: return flags.investment;
        case TutorialTopic::Baby: return flags.baby;
        case TutorialTopic::Pet: return flags.pet;
        case TutorialTopic::Marriage: return flags.marriage;
        case TutorialTopic::Insurance: return flags.insurance;
        case TutorialTopic::Shield: return flags.shield;
        case TutorialTopic::ActionCard: return flags.actionCard;
        case TutorialTopic::Minigame: return flags.minigame;
        case TutorialTopic::Sabotage: return flags.sabotage;
    }
    return flags.automaticLoan;
}

std::string tutorialTitle(TutorialTopic topic) {
    switch (topic) {
        case TutorialTopic::AutomaticLoan: return "Automatic Loans";
        case TutorialTopic::ManualLoan: return "Manual Loans";
        case TutorialTopic::Investment: return "Investments";
        case TutorialTopic::Baby: return "Babies and Kids";
        case TutorialTopic::Pet: return "Pets";
        case TutorialTopic::Marriage: return "Marriage";
        case TutorialTopic::Insurance: return "Insurance";
        case TutorialTopic::Shield: return "Shields";
        case TutorialTopic::ActionCard: return "Action Cards";
        case TutorialTopic::Minigame: return "Minigames";
        case TutorialTopic::Sabotage: return "Sabotage";
    }
    return "Guide";
}

std::vector<std::string> tutorialLines(TutorialTopic topic) {
    switch (topic) {
        case TutorialTopic::AutomaticLoan:
            return {
                "You received an automatic loan because you could not afford the payment.",
                "Loans keep you moving now, but each loan lowers your final score later."
            };
        case TutorialTopic::ManualLoan:
            return {
                "Manual loans give emergency cash when you choose to take them.",
                "Use them carefully: every loan becomes a final-score penalty."
            };
        case TutorialTopic::Investment:
            return {
                "Investments can grow your wealth when the spinner matches your number.",
                "Market and money events can still change your position, so watch your cash."
            };
        case TutorialTopic::Baby:
            return {
                "You had a baby! Kids can affect life events and final scoring.",
                "Family spaces may reward or charge you based on how big your family becomes."
            };
        case TutorialTopic::Pet:
            return {
                "You adopted a pet! Pets add to your story and can improve final scoring.",
                "Some editions award pets through family and house events."
            };
        case TutorialTopic::Marriage:
            return {
                "A major life moment arrives.",
                "Marriage can trigger gift money and opens up some family-related events."
            };
        case TutorialTopic::Insurance:
            return {
                "Insurance can reduce certain losses or protect against negative events.",
                "It is especially useful when sabotage and risky routes start hitting harder."
            };
        case TutorialTopic::Shield:
            return {
                "Shields can block or reduce harmful effects.",
                "A shield is consumed when it blocks an incoming sabotage."
            };
        case TutorialTopic::ActionCard:
            return {
                "Action cards give special abilities.",
                "Some help, some start duels, and some become more tactical after sabotage unlocks."
            };
        case TutorialTopic::Minigame:
            return {
                "Minigames are short challenges that can award money or settle duels.",
                "Read the one-page tutorial before each minigame starts."
            };
        case TutorialTopic::Sabotage:
            return {
                "Sabotage lets players interfere with opponents after Turn 3.",
                "Use it strategically: CPU players can sabotage too, and shields or insurance may answer back."
            };
    }
    return std::vector<std::string>();
}

void showPagedGuide(const std::string& title,
                    const std::vector<std::vector<std::string> >& pages,
                    bool hasColor) {
    WINDOW* popup = centeredPopup(20, 84);
    if (!popup) {
        return;
    }
    const int pageCount = std::max(1, static_cast<int>(pages.size()));

    for (int page = 0; page < pageCount;) {
        int height = 0;
        int width = 0;
        getmaxyx(popup, height, width);
        werase(popup);
        drawBoxSafe(popup);
        if (hasColor) {
            wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }
        drawCentered(popup, 1, title + " (" + std::to_string(page + 1) + "/" + std::to_string(pageCount) + ")", A_BOLD);
        if (hasColor) {
            wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
        }

        const std::vector<std::string>& lines = pages[static_cast<std::size_t>(page)];
        const int maxLines = std::max(1, height - 6);
        for (int i = 0; i < maxLines && i < static_cast<int>(lines.size()); ++i) {
            mvwprintw(popup,
                      3 + i,
                      2,
                      "%s",
                      clipUiText(lines[static_cast<std::size_t>(i)],
                                 static_cast<std::size_t>(std::max(8, width - 4))).c_str());
        }

        mvwprintw(popup, height - 2, 2, "%s",
                  clipUiText("ENTER next  ESC back", static_cast<std::size_t>(std::max(1, width - 4))).c_str());
        wrefresh(popup);
        const int ch = wgetch(popup);
        if (isCancelKey(ch)) {
            break;
        }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == ' ') {
            ++page;
        }
    }

    delwin(popup);
    touchwin(stdscr);
    refresh();
}

void showPreGameQuickGuide(bool hasColor) {
    std::vector<std::vector<std::string> > pages;
    pages.push_back({
        "MONEY AND LOANS",
        "",
        "Cash pays for choices, penalties, houses, and big events.",
        "If you cannot afford a payment, the game may automatically give you loans.",
        "Manual loans are emergency cash, but every loan reduces your final score."
    });
    pages.push_back({
        "JOBS, SALARY, AND INVESTMENTS",
        "",
        "Jobs set your salary. Paydays add salary plus the space payout.",
        "College costs more early, but can unlock stronger careers.",
        "Investments pay when any spinner result matches your investment number."
    });
    pages.push_back({
        "LIFE EVENTS",
        "",
        "Marriage, babies, pets, and houses can change your money and final score.",
        "Kids and pets are part of your story and may create bonuses or costs.",
        "Houses can be sold near the end of the game."
    });
    pages.push_back({
        "CARDS, DEFENSE, AND MINIGAMES",
        "",
        "Action cards can help, hurt, move you, or start duels.",
        "Minigames award money and resolve some competitions.",
        "Insurance reduces some losses. Shields block harmful sabotage."
    });
    pages.push_back({
        "SABOTAGE AND ENDGAME",
        "",
        "Sabotage unlocks on each player's Turn 3.",
        "After that, players and CPUs can interfere with opponents.",
        "Final score counts cash, homes, pets, kids, cards,",
        "retirement bonuses, and loan penalties.",
        "Press G in-game to look at guide and tutorials again."
    });
    showPagedGuide("QUICK GUIDE", pages, hasColor);
}

void showFirstTimeTutorial(TutorialTopic topic, bool hasColor) {
    std::vector<std::vector<std::string> > pages;
    std::vector<std::string> page;
    page.push_back(tutorialTitle(topic));
    page.push_back("");
    const std::vector<std::string> lines = tutorialLines(topic);
    page.insert(page.end(), lines.begin(), lines.end());
    pages.push_back(page);
    showPagedGuide("FIRST-TIME TIP", pages, hasColor);
}

void showFullGuide(const Board& board, const RuleSet& rules, bool sabotageUnlocked, bool hasColor) {
    std::vector<std::vector<std::string> > pages;
    std::vector<std::string> legendPage;
    legendPage.push_back("TILE LEGEND");
    legendPage.push_back("");
    const std::vector<std::string> legend = board.tutorialLegend();
    legendPage.insert(legendPage.end(), legend.begin(), legend.end());
    pages.push_back(legendPage);
    pages.push_back({
        "CONTROLS",
        "",
        "ENTER confirms and starts turns.",
        "SPACE spins or fires in minigames.",
        "TAB opens the scoreboard.",
        "G opens this guide. K opens controls.",
        "ESC is the universal back/cancel/quit key."
    });
    pages.push_back({
        "ECONOMY AND LOANS",
        "",
        "Cash is your main resource.",
        "Automatic loans appear when a payment would drop cash below zero.",
        "Loan unit: $" + std::to_string(rules.loanUnit) + ". Repayment penalty: $" + std::to_string(rules.loanRepaymentCost) + ".",
        "Loans help in the moment but lower final worth."
    });
    pages.push_back({
        "INVESTMENTS AND LIFE",
        "",
        "Investments pay when the spinner matches your number.",
        "Marriage can trigger gift money.",
        "Babies, pets, and houses add story events and final-score effects."
    });
    pages.push_back({
        "CARDS, MINIGAMES, AND SABOTAGE",
        "",
        "Action cards create one-time effects, stored bonuses, or duels.",
        "Minigames explain their controls before play starts.",
        sabotageUnlocked ? "Sabotage is unlocked." : "Sabotage unlocks on Turn 3.",
        "Sabotage can place traps, start lawsuits, force duels, slow movement, steal cards, reduce salary, or add debt."
    });
    pages.push_back({
        "ENDGAME SCORING",
        "",
        "Final score includes cash, home value, cards, pets, kids, and retirement bonuses.",
        "Loan debt is subtracted.",
        "Highest final net worth wins."
    });
    showPagedGuide("GUIDE", pages, hasColor);
}

bool showQuitConfirmation(bool hasColor) {
    WINDOW* popup = centeredPopup(10, 56);
    if (!popup) {
        return false;
    }
    werase(popup);
    drawBoxSafe(popup);
    int height = 0;
    int width = 0;
    getmaxyx(popup, height, width);
    const int contentW = std::max(1, width - 4);
    if (hasColor) {
        wattron(popup, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
    drawCentered(popup, 1, "QUIT GAME?", A_BOLD);
    if (hasColor) {
        wattroff(popup, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
    mvwprintw(popup, 3, 2, "%s",
              clipUiText("Are you sure you want to quit?", static_cast<std::size_t>(contentW)).c_str());
    mvwprintw(popup, height - 4, 2, "%s",
              clipUiText("Press Enter to quit.", static_cast<std::size_t>(contentW)).c_str());
    mvwprintw(popup, height - 3, 2, "%s",
              clipUiText("Press ESC to go back.", static_cast<std::size_t>(contentW)).c_str());
    wrefresh(popup);

    while (true) {
        const int ch = wgetch(popup);
        if (isCancelKey(ch)) {
            delwin(popup);
            return false;
        }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            delwin(popup);
            return true;
        }
    }
}

void showSabotageUnlockAnimation(bool hasColor) {
    std::vector<std::string> art;
    art.push_back(" ____    _    ____   ___ _____  _    ____ _____");
    art.push_back("/ ___|  / \\  | __ ) / _ \\_   _|/ \\  / ___| ____|");
    art.push_back("\\___ \\ / _ \\ |  _ \\| | | || | / _ \\| |  _|  _|");
    art.push_back(" ___) / ___ \\| |_) | |_| || |/ ___ \\ |_| | |___");
    art.push_back("|____/_/   \\_\\____/ \\___/ |_/_/   \\_\\____|_____|");

    WINDOW* popup = centeredPopup(15, 72);
    if (!popup) {
        return;
    }
    for (int frame = 0; frame < 6; ++frame) {
        werase(popup);
        drawBoxSafe(popup);
        const int colorPair = frame % 2 == 0 ? GOLDRUSH_GOLD_TERRA : GOLDRUSH_GOLD_SAND;
        if (hasColor) {
            wattron(popup, COLOR_PAIR(colorPair) | A_BOLD);
        }
        for (std::size_t i = 0; i < art.size(); ++i) {
            drawCentered(popup, 3 + static_cast<int>(i), art[i], A_BOLD);
        }
        drawCentered(popup, 10, "SABOTAGE UNLOCKED!", A_BOLD);
        if (hasColor) {
            wattroff(popup, COLOR_PAIR(colorPair) | A_BOLD);
        }
        wrefresh(popup);
        napms(180);
    }
    int height = 0;
    int width = 0;
    getmaxyx(popup, height, width);
    mvwprintw(popup, height - 2, 2, "%s",
              clipUiText("Press ENTER to continue...",
                         static_cast<std::size_t>(std::max(1, width - 4))).c_str());
    wrefresh(popup);
    waitForEnterPrompt(popup, height - 2, 2, "");
    delwin(popup);
}

void showSabotageTutorial(bool hasColor) {
    std::vector<std::vector<std::string> > pages;
    pages.push_back({
        "SABOTAGE BASICS",
        "",
        "Sabotage lets players interfere with opponents.",
        "CPU players can sabotage too.",
        "Some sabotage uses spinner rolls, some starts duels, and some applies status effects.",
        "Shields and insurance may block or reduce the damage."
    });
    pages.push_back({
        "SABOTAGE TYPES",
        "",
        "Trap Tile: place a trap on the board.",
        "Lawsuit: both players roll; higher roll wins money.",
        "Forced Duel: challenge another player to a minigame.",
        "Traffic Jam: slow another player's next movement.",
        "Steal Card: steal one actual card from an opponent."
    });
    pages.push_back({
        "MORE SABOTAGE TYPES",
        "",
        "Career Sabotage: temporarily reduce salary.",
        "Debt Trap: force another player to take debt or loans.",
        "Use sabotage strategically, not randomly."
    });
    showPagedGuide("SABOTAGE GUIDE", pages, hasColor);
}
