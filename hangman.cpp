#include "hangman.hpp"
#include "input_helpers.h"
#include "minigame_tutorials.h"
#include "ui.h"
#include "ui_helpers.h"

#include <ncurses.h>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <ctime>
#include <vector>

namespace {

const int MAX_WRONG_GUESSES = 8;

struct HangmanWord {
    std::string word;
    std::string category;
    std::string hint;
    int difficulty;
};

const std::vector<HangmanWord> WORD_BANK = {
    // Disney Characters
    {"MICKEY", "Disney", "Famous mouse with round ears", 2},
    {"MINNIE", "Disney", "Mickey's girlfriend", 2},
    {"DONALD", "Disney", "White duck in a sailor suit", 2},
    {"DAISY", "Disney", "Donald's girlfriend", 1},
    {"GOOFY", "Disney", "Clumsy dog friend", 1},
    {"PLUTO", "Disney", "Mickey's pet dog", 1},
    {"SIMBA", "Disney", "Lion King cub", 1},
    {"ELSA", "Disney", "Frozen snow queen", 1},
    {"ANNA", "Disney", "Elsa's sister", 1},
    {"OLAF", "Disney", "Snowman who loves summer", 1},
    {"STITCH", "Disney", "Blue alien experiment 626", 2},
    {"LILO", "Disney", "Girl who adopted Stitch", 1},
    {"ARIEL", "Disney", "Little mermaid", 2},
    {"BELLE", "Disney", "Beauty and the Beast", 2},
    {"MOANA", "Disney", "Polynesian voyager", 2},
    {"MAUI", "Disney", "Demigod with fishhook", 1},
    {"WOODY", "Disney", "Cowboy doll from Toy Story", 1},
    {"BUZZ", "Disney", "Space ranger toy", 1},
    {"NEMO", "Disney", "Clownfish who got lost", 1},
    {"DORY", "Disney", "Forgetful blue tang fish", 1},

    // Animals
    {"DUCK", "Animals", "Water bird that quacks", 1},
    {"RABBIT", "Animals", "Long ears, hops fast", 2},
    {"TURTLE", "Animals", "Slow reptile with a shell", 2},
    {"EAGLE", "Animals", "Large bird of prey", 2},
    {"HAWK", "Animals", "Hunting bird with sharp eyes", 1},
    {"RAVEN", "Animals", "Black intelligent bird", 1},
    {"WOLF", "Animals", "Pack animal that howls", 1},
    {"FOX", "Animals", "Clever red animal", 1},
    {"BEAR", "Animals", "Large hibernating mammal", 1},
    {"DEER", "Animals", "Forest animal with antlers", 1},
    {"OTTER", "Animals", "River animal that plays", 2},
    {"BEAVER", "Animals", "Builds dams in rivers", 2},
    {"SQUIRREL", "Animals", "Nut-collecting tree climber", 3},
    {"RACCOON", "Animals", "Masked face, washes food", 3},
    {"BADGER", "Animals", "Black and white striped digger", 2},
    {"FALCON", "Animals", "Fastest bird of prey", 2},
    {"OSPREY", "Animals", "Fish-eating hawk", 2},
    {"COYOTE", "Animals", "Desert wild dog", 2},
    {"LYNX", "Animals", "Wild cat with tufted ears", 1},
    {"MOOSE", "Animals", "Large antlered animal", 2},

    // Hobbies
    {"PAINTING", "Hobbies", "Creating art with brushes", 3},
    {"DRAWING", "Hobbies", "Sketching with pencils", 3},
    {"READING", "Hobbies", "Enjoying books", 3},
    {"WRITING", "Hobbies", "Creating stories or poems", 3},
    {"HIKING", "Hobbies", "Walking in nature trails", 2},
    {"CAMPING", "Hobbies", "Sleeping outdoors in a tent", 3},
    {"FISHING", "Hobbies", "Catching fish with a rod", 3},
    {"GARDENING", "Hobbies", "Growing plants and flowers", 3},
    {"COOKING", "Hobbies", "Preparing delicious meals", 3},
    {"BAKING", "Hobbies", "Making bread and cakes", 2},
    {"DANCING", "Hobbies", "Moving rhythmically to music", 3},
    {"SINGING", "Hobbies", "Using your voice musically", 3},
    {"PHOTOGRAPHY", "Hobbies", "Taking pictures with a camera", 3},
    {"KNITTING", "Hobbies", "Making fabric from yarn", 3},
    {"YOGA", "Hobbies", "Breathing and stretching exercise", 1},
    {"RUNNING", "Hobbies", "Jogging for fitness", 3},
    {"SWIMMING", "Hobbies", "Moving through water", 3},
    {"CYCLING", "Hobbies", "Riding a bicycle", 3},
    {"GAMING", "Hobbies", "Playing video games", 2},
    {"CLIMBING", "Hobbies", "Scaling rocks or walls", 3},

    // Colors
    {"RED", "Colors", "Color of apples", 1},
    {"BLUE", "Colors", "Color of sky", 1},
    {"GREEN", "Colors", "Color of grass", 2},
    {"YELLOW", "Colors", "Color of sun", 2},
    {"PINK", "Colors", "Cotton candy color", 1},
    {"PURPLE", "Colors", "Grape color", 2},
    {"ORANGE", "Colors", "Fruit color", 2},
    {"BROWN", "Colors", "Chocolate color", 2},
    {"GOLD", "Colors", "Precious metal color", 1},
    {"BLACK", "Colors", "Night sky color", 2},
    {"WHITE", "Colors", "Snow color", 2},
    {"SILVER", "Colors", "Shiny metal color", 2},
    {"BRONZE", "Colors", "Brownish metal color", 2},
    {"MAROON", "Colors", "Dark reddish brown", 2},
    {"VIOLET", "Colors", "Purple-blue flower color", 2},

    // Food
    {"PIZZA", "Food", "Italian cheese dish", 2},
    {"BURGER", "Food", "Meat between buns", 2},
    {"TACO", "Food", "Mexican folded food", 1},
    {"SUSHI", "Food", "Raw fish with rice", 2},
    {"NOODLE", "Food", "Long thin pasta", 2},
    {"APPLE", "Food", "Red or green fruit", 2},
    {"BANANA", "Food", "Yellow curved fruit", 2},
    {"GRAPE", "Food", "Small purple fruit", 2},
    {"COOKIE", "Food", "Sweet baked treat", 2},
    {"CANDY", "Food", "Sweet sugar treat", 2},
    {"SALAD", "Food", "Mixed vegetables", 2},
    {"PASTA", "Food", "Italian dough food", 2},
    {"CHEESE", "Food", "Dairy made from milk", 2},
    {"BUTTER", "Food", "Yellow dairy spread", 2},
    {"HONEY", "Food", "Sweet bee-made syrup", 2},

    // Everyday Objects
    {"BOOK", "Objects", "Pages to read", 1},
    {"PEN", "Objects", "Tool for writing", 1},
    {"CUP", "Objects", "Holds drinks", 1},
    {"BALL", "Objects", "Round toy to throw", 1},
    {"HAT", "Objects", "Worn on head", 1},
    {"SHOE", "Objects", "Worn on foot", 1},
    {"CLOCK", "Objects", "Tells time", 2},
    {"PHONE", "Objects", "Device for calling", 2},
    {"LAPTOP", "Objects", "Portable computer", 2},
    {"GUITAR", "Objects", "String instrument", 2},
    {"PIANO", "Objects", "Keyboard instrument", 2},
    {"DRUM", "Objects", "Percussion instrument", 1},
    {"CAMERA", "Objects", "Takes photographs", 2},
    {"WALLET", "Objects", "Holds money and cards", 2},
    {"BACKPACK", "Objects", "Bag worn on shoulders", 3},
};

const std::vector<std::string> HANGMAN_TITLE = {
    " __   __  _______  __    _  _______  __   __  _______  __    _ ",
    "|  | |  ||   _   ||  |  | ||       ||  |_|  ||   _   ||  |  | |",
    "|  |_|  ||  |_|  ||   |_| ||    ___||       ||  |_|  ||   |_| |",
    "|       ||       ||       ||   | __ |       ||       ||       |",
    "|       ||       ||  _    ||   ||  ||       ||       ||  _    |",
    "|   _   ||   _   || | |   ||   |_| || ||_|| ||   _   || | |   |",
    "|__| |__||__| |__||_|  |__||_______||_|   |_||__| |__||_|  |__|"
};

const std::vector<std::string> HANGMAN_STAGES = {
    "\n  +---+\n  |   |\n      |\n      |\n      |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n      |\n      |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n  |   |\n      |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n /|   |\n      |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n /|\\  |\n      |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n /|\\  |\n /    |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n /|\\  |\n / \\  |\n      |\n=========",
    "\n  +---+\n  |   |\n  O   |\n /|\\  |\n / \\  |\n      |\n=========",
    "\n  +---+\n  |   |\n  X   |\n /|\\  |\n / \\  |\n      |\n========= GAME OVER!"
};

HangmanWord getRandomWord() {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }

    const int pickedIndex = std::rand() % static_cast<int>(WORD_BANK.size());
    return WORD_BANK[static_cast<std::size_t>(pickedIndex)];
}

void drawHangman(WINDOW* win, int y, int x, int wrong, bool hasColor) {
    int stage = wrong > MAX_WRONG_GUESSES ? MAX_WRONG_GUESSES : wrong;
    std::string art = HANGMAN_STAGES[stage];
    
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
    
    int ly = y, lx = x;
    for (char c : art) {
        if (c == '\n') {
            ly++;
            lx = x;
        } else {
            mvwaddch(win, ly, lx++, c);
        }
    }
    
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
    }
}

void drawWord(WINDOW* win, int y, int x, const std::string& disp, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD | A_UNDERLINE);
    } else {
        wattron(win, A_BOLD | A_UNDERLINE);
    }
    
    for (size_t i = 0; i < disp.size(); i++) {
        mvwaddch(win, y, x + i * 2, disp[i]);
        mvwaddch(win, y, x + i * 2 + 1, ' ');
    }
    
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD | A_UNDERLINE);
    } else {
        wattroff(win, A_BOLD | A_UNDERLINE);
    }
}

void drawGuessed(WINDOW* win, int y, int x, const std::string& guessed, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    }
    
    mvwprintw(win, y, x, "Guessed: ");
    for (int i = 0; i < 26; i++) {
        if (guessed[i] != ' ') {
            if (hasColor) {
                wattron(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
            }
            mvwaddch(win, y, x + 9 + i * 2, guessed[i]);
            mvwaddch(win, y, x + 9 + i * 2 + 1, ' ');
            if (hasColor) {
                wattroff(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
            }
        }
    }
    
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
    }
}

void drawAsciiTitle(WINDOW* win, int screenW, bool hasColor) {
    if (hasColor) {
        wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
    for (std::size_t i = 0; i < HANGMAN_TITLE.size(); ++i) {
        mvwprintw(win, static_cast<int>(i), (screenW - static_cast<int>(HANGMAN_TITLE[i].size())) / 2,
                  "%s", HANGMAN_TITLE[i].c_str());
    }
    if (hasColor) {
        wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_SAND) | A_BOLD);
    }
}

bool blinkHangmanIndicatorUntilInput(WINDOW* win,
                                     int y,
                                     int x,
                                     const std::string& text,
                                     bool hasColor,
                                     int colorPair,
                                     int blinkCount = 2,
                                     int finalHoldMs = 1000) {
    const int width = static_cast<int>(text.size());
    nodelay(win, TRUE);
    auto pollDelay = [&](int totalMs) -> bool {
        const int stepMs = 40;
        for (int elapsed = 0; elapsed < totalMs; elapsed += stepMs) {
            const int ch = wgetch(win);
            if (ch != ERR) {
                ungetch(ch);
                nodelay(win, FALSE);
                return true;
            }
            napms(stepMs);
        }
        return false;
    };

    for (int i = 0; i < blinkCount; ++i) {
        mvwprintw(win, y, x, "%*s", width, "");
        wrefresh(win);
        if (pollDelay(140)) return true;

        if (hasColor) wattron(win, COLOR_PAIR(colorPair) | A_BOLD);
        mvwprintw(win, y, x, "%s", text.c_str());
        if (hasColor) wattroff(win, COLOR_PAIR(colorPair) | A_BOLD);
        wrefresh(win);
        if (pollDelay(180)) return true;
    }

    if (hasColor) wattron(win, COLOR_PAIR(colorPair) | A_BOLD);
    mvwprintw(win, y, x, "%s", text.c_str());
    if (hasColor) wattroff(win, COLOR_PAIR(colorPair) | A_BOLD);
    wrefresh(win);
    const bool interrupted = pollDelay(finalHoldMs);
    nodelay(win, FALSE);
    return interrupted;
}

void flashFeedback(WINDOW* win,
                   int y,
                   int arenaLeft,
                   int arenaWidth,
                   const std::string& text,
                   bool positive,
                   bool hasColor) {
    const int textX = arenaLeft + (arenaWidth - static_cast<int>(text.size())) / 2;
    const int colorPair = positive ? GOLDRUSH_BLACK_FOREST : GOLDRUSH_GOLD_TERRA;

    blinkHangmanIndicatorUntilInput(win, y, textX, text, hasColor, colorPair, 2, 1000);
}

} // anonymous namespace

HangmanResult playHangmanMinigame(const std::string& playerName, bool hasColor) {
    HangmanResult res;
    res.won = false;
    res.attemptsLeft = MAX_WRONG_GUESSES;
    res.lettersGuessed = 0;
    res.abandoned = false;

    showMinigameTutorial("Hangman",
                         "Guess the hidden word one letter at a time.",
                         "Type A-Z to guess, ESC exits.",
                         "Reveal the full word before 8 wrong guesses.",
                         "Each revealed letter slot pays $100. Exiting early pays nothing.",
                         hasColor);

    WINDOW* win = newwin(0, 0, 0, 0);
    keypad(win, TRUE);
    nodelay(win, FALSE);

    const HangmanWord selection = getRandomWord();
    const std::string word = selection.word;
    std::string disp(word.size(), '_');
    std::string guessed(26, ' ');
    int wrong = 0;

    while (true) {
        int h, w;
        getmaxyx(stdscr, h, w);
        wresize(win, h, w);
        mvwin(win, 0, 0);
        werase(win);
        if (hasColor) {
            wbkgd(win, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
        }

        const int arenaWidth = 86;
        const int arenaHeight = 28;
        const int arenaLeft = (w - arenaWidth) / 2;
        const int arenaTop = 9;
        const int arenaRight = arenaLeft + arenaWidth - 1;
        const int arenaBottom = arenaTop + arenaHeight - 1;
        const int arenaCenterY = arenaTop + arenaHeight / 2;

        drawAsciiTitle(win, w, hasColor);

        if (hasColor) {
            wattron(win, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        }
        const std::string statusLine =
            "Player: " + playerName + "  |  Wrong guesses left: " +
            std::to_string(std::max(0, MAX_WRONG_GUESSES - wrong)) + " / " +
            std::to_string(MAX_WRONG_GUESSES) + "  |  Current payout: $" +
            std::to_string(res.lettersGuessed * 100);
        mvwprintw(win, 8, (w - static_cast<int>(statusLine.size())) / 2, "%s", statusLine.c_str());
        if (hasColor) {
            wattroff(win, COLOR_PAIR(GOLDRUSH_BROWN_CREAM));
        }

        if (hasColor) {
            wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }
        mvwhline(win, arenaTop, arenaLeft, ACS_HLINE, arenaWidth);
        mvwhline(win, arenaBottom, arenaLeft, ACS_HLINE, arenaWidth);
        mvwvline(win, arenaTop, arenaLeft, ACS_VLINE, arenaHeight);
        mvwvline(win, arenaTop, arenaRight, ACS_VLINE, arenaHeight);
        mvwaddch(win, arenaTop, arenaLeft, ACS_ULCORNER);
        mvwaddch(win, arenaTop, arenaRight, ACS_URCORNER);
        mvwaddch(win, arenaBottom, arenaLeft, ACS_LLCORNER);
        mvwaddch(win, arenaBottom, arenaRight, ACS_LRCORNER);
        if (hasColor) {
            wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
        }

        drawHangman(win, arenaTop + 3, arenaLeft + 4, wrong, hasColor);

        const int wordX = arenaLeft + (arenaWidth - static_cast<int>(disp.size()) * 2) / 2;
        mvwprintw(win, arenaTop + 2, arenaLeft + 34, "Category: %s", selection.category.c_str());
        mvwprintw(win, arenaTop + 3, arenaLeft + 34, "Length: %d letters", static_cast<int>(word.size()));
        if (wrong >= 5) {
            mvwprintw(win, arenaTop + 5, arenaLeft + 34, "Hint:");
            mvwprintw(win, arenaTop + 6, arenaLeft + 34, "%s", selection.hint.c_str());
        } else {
            mvwprintw(win, arenaTop + 5, arenaLeft + 34, "Hint unlocks after 5 misses");
        }
        drawWord(win, arenaTop + 14, wordX, disp, hasColor);
        drawGuessed(win, arenaTop + 17, arenaLeft + 3, guessed, hasColor);

        if (hasColor) {
            wattron(win, COLOR_PAIR(GOLDRUSH_BLACK_CREAM));
        }
        mvwprintw(win, arenaBottom - 3, arenaLeft + 3,
                  "Guess letters A-Z. Category is shown and the hint appears after 5 misses.");
        mvwprintw(win, arenaBottom - 2, arenaLeft + 3,
            "Each revealed letter slot is worth $100. One finished hangman ends the game.");
        if (hasColor) {
            wattroff(win, COLOR_PAIR(GOLDRUSH_BLACK_CREAM));
        }

        if (disp == word) {
            res.won = true;
            res.attemptsLeft = std::max(0, MAX_WRONG_GUESSES - wrong);

            werase(win);
            if (hasColor) {
                wbkgd(win, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
                wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
            }
            mvwhline(win, arenaTop, arenaLeft, ACS_HLINE, arenaWidth);
            mvwhline(win, arenaBottom, arenaLeft, ACS_HLINE, arenaWidth);
            mvwvline(win, arenaTop, arenaLeft, ACS_VLINE, arenaHeight);
            mvwvline(win, arenaTop, arenaRight, ACS_VLINE, arenaHeight);
            mvwaddch(win, arenaTop, arenaLeft, ACS_ULCORNER);
            mvwaddch(win, arenaTop, arenaRight, ACS_URCORNER);
            mvwaddch(win, arenaBottom, arenaLeft, ACS_LLCORNER);
            mvwaddch(win, arenaBottom, arenaRight, ACS_LRCORNER);
            if (hasColor) {
                wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
                wattron(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
            }
            const std::string winLine1 = "CONGRATULATIONS!";
            const std::string winLine2 = "You guessed the word: " + word;
            const std::string winLine3 = "Letters revealed: " + std::to_string(res.lettersGuessed) +
                                         "  |  Earned $" + std::to_string(res.lettersGuessed * 100);
            const std::string winLine4 = "Press ENTER to continue.";
            mvwprintw(win, arenaCenterY - 1, arenaLeft + (arenaWidth - static_cast<int>(winLine2.size())) / 2,
                      "%s", winLine2.c_str());
            mvwprintw(win, arenaCenterY, arenaLeft + (arenaWidth - static_cast<int>(winLine3.size())) / 2,
                      "%s", winLine3.c_str());
            mvwprintw(win, arenaCenterY + 2, arenaLeft + (arenaWidth - static_cast<int>(winLine4.size())) / 2,
                      "%s", winLine4.c_str());
            blinkIndicator(win,
                           arenaCenterY - 2,
                           arenaLeft + (arenaWidth - static_cast<int>(winLine1.size())) / 2,
                           winLine1,
                           hasColor,
                           GOLDRUSH_BLACK_FOREST,
                           2,
                           2000,
                           static_cast<int>(winLine1.size()));
            if (hasColor) {
                wattroff(win, COLOR_PAIR(GOLDRUSH_BLACK_FOREST) | A_BOLD);
            }
            wrefresh(win);

            int ch;
            do {
                ch = wgetch(win);
            } while (ch != '\n' && ch != '\r' && ch != KEY_ENTER);
            break;
        }

        if (wrong >= MAX_WRONG_GUESSES) {
            res.won = false;
            res.attemptsLeft = 0;

            werase(win);
            if (hasColor) {
                wbkgd(win, COLOR_PAIR(GOLDRUSH_GOLD_BLACK));
                wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
            }
            mvwhline(win, arenaTop, arenaLeft, ACS_HLINE, arenaWidth);
            mvwhline(win, arenaBottom, arenaLeft, ACS_HLINE, arenaWidth);
            mvwvline(win, arenaTop, arenaLeft, ACS_VLINE, arenaHeight);
            mvwvline(win, arenaTop, arenaRight, ACS_VLINE, arenaHeight);
            mvwaddch(win, arenaTop, arenaLeft, ACS_ULCORNER);
            mvwaddch(win, arenaTop, arenaRight, ACS_URCORNER);
            mvwaddch(win, arenaBottom, arenaLeft, ACS_LLCORNER);
            mvwaddch(win, arenaBottom, arenaRight, ACS_LRCORNER);
            if (hasColor) {
                wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_FOREST) | A_BOLD);
                wattron(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
            }
            const std::string loseLine1 = "GAME OVER!";
            const std::string loseLine2 = "The word was: " + word;
            const std::string loseLine3 = "Letters revealed: " + std::to_string(res.lettersGuessed) +
                                          "  |  Earned $" + std::to_string(res.lettersGuessed * 100);
            const std::string loseLine4 = "Press ENTER to continue.";
            mvwprintw(win, arenaCenterY - 1, arenaLeft + (arenaWidth - static_cast<int>(loseLine2.size())) / 2,
                      "%s", loseLine2.c_str());
            mvwprintw(win, arenaCenterY, arenaLeft + (arenaWidth - static_cast<int>(loseLine3.size())) / 2,
                      "%s", loseLine3.c_str());
            mvwprintw(win, arenaCenterY + 2, arenaLeft + (arenaWidth - static_cast<int>(loseLine4.size())) / 2,
                      "%s", loseLine4.c_str());
            blinkIndicator(win,
                           arenaCenterY - 2,
                           arenaLeft + (arenaWidth - static_cast<int>(loseLine1.size())) / 2,
                           loseLine1,
                           hasColor,
                           GOLDRUSH_GOLD_TERRA,
                           2,
                           2000,
                           static_cast<int>(loseLine1.size()));
            if (hasColor) {
                wattroff(win, COLOR_PAIR(GOLDRUSH_GOLD_TERRA) | A_BOLD);
            }
            wrefresh(win);

            int ch;
            do {
                ch = wgetch(win);
            } while (ch != '\n' && ch != '\r' && ch != KEY_ENTER);
            break;
        }

        wrefresh(win);

        int ch = wgetch(win);

        if (isCancelKey(ch)) {
            res.abandoned = true;
            break;
        }

        if (ch >= 'a' && ch <= 'z') {
            ch = ch - 'a' + 'A';
        }

        if (ch >= 'A' && ch <= 'Z') {
            int idx = ch - 'A';

            if (guessed[idx] != ' ') {
                flashFeedback(win,
                              arenaBottom - 1,
                              arenaLeft,
                              arenaWidth,
                              "Already guessed '" + std::string(1, static_cast<char>(ch)) + "'!",
                              false,
                              hasColor);
                continue;
            }

            guessed[idx] = static_cast<char>(ch);

            bool found = false;
            int revealedThisGuess = 0;
            for (size_t i = 0; i < word.size(); i++) {
                if (word[i] == ch) {
                    disp[i] = static_cast<char>(ch);
                    res.lettersGuessed++;
                    revealedThisGuess++;
                    found = true;
                }
            }
            
            if (!found) {
                wrong++;
                flashFeedback(win,
                              arenaBottom - 1,
                              arenaLeft,
                              arenaWidth,
                              "MISS! Attempts left: " +
                                  std::to_string(std::max(0, MAX_WRONG_GUESSES - wrong)),
                              false,
                              hasColor);
            } else {
                flashFeedback(win,
                              arenaBottom - 1,
                              arenaLeft,
                              arenaWidth,
                              "CORRECT! +$" + std::to_string(revealedThisGuess * 100),
                              true,
                              hasColor);
            }
        }
    }

    delwin(win);
    touchwin(stdscr);
    refresh();
    
    return res;
}
