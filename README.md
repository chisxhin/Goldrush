# Goldrush - Terminal Edition

A text-based board game inspired by *The Game of Life*, built with C++ and `ncurses`.

## Features

- Branching path system with college/career, family/life, and safe/risky choices
- `2-4` players
- Terminal UI with an ASCII title screen, board view, sidebar, and popup windows
- Normal mode and custom mode rule toggles
- Save/load support through `.sav` files
- Action, career, house, investment, and pet cards
- Deterministic deck and RNG state restoration when loading a saved game

## Game Flow

```text
TITLE SCREEN
  |
  +-- New Game
  |     |
  |     +-- Normal Mode / Custom Mode
  |     |
  |     +-- Player Setup
  |     |
  |     +-- Tutorial (optional)
  |     |
  |     +-- Main Game Loop
  |
  +-- Load Game
  |
  +-- Quit
```

Main board progression:

```text
START
  |
  +-- COLLEGE -----+
  |                |
  +-- CAREER ------+
         |
     GRADUATION
         |
      MARRIAGE
         |
     FAMILY STOP
      /       \
  FAMILY      LIFE
    PATH      PATH
      \       /
       HOUSE / PAYDAY / ACTION
             |
         RISK SPLIT
         /       \
      SAFE      RISKY
         \       /
          RETIREMENT
```

## Space Effects

The exact outcomes depend on the current ruleset, but the main space types are:

| Space | Effect |
| --- | --- |
| `START` | Begin the journey |
| `COLLEGE` | Pay tuition and head toward degree-based careers |
| `CAREER` | Start working immediately |
| `GRADUATION` | Choose or confirm a career path |
| `MARRIAGE` | Resolve marriage and gift spin |
| `FAMILY STOP` | Choose family path or life path |
| `ACTION` | Draw and resolve an action card |
| `PAYDAY` | Collect salary |
| `HOUSE` | Buy a house |
| `SAFE` | Take a lower-risk reward spin |
| `RISKY` | Take a higher-risk reward/loss spin |
| `RETIREMENT` | End active play and lock in retirement bonuses |

## Winning

At the end of the game, final worth includes:

- cash
- house sale value
- action card value
- pet card value
- baby bonuses
- retirement bonus
- loan penalties

The player with the highest final worth wins.

## Requirements

- C++17 or later
- `ncurses`
- Unix-like terminal environment

Install `ncurses`:

macOS:

```bash
brew install ncurses
```

Ubuntu / Debian:

```bash
sudo apt-get install libncurses5-dev
```

## Build

Compile:

```bash
make
```

The Makefile links with wide-character ncurses by default:

```bash
g++ *.cpp -std=c++17 -lncursesw
```

Use plain ncurses only if `ncursesw` is not available on the target system:

```bash
make LIBS=-lncurses
```

Run:

```bash
./gameoflife
```

Or:

```bash
make run
```

Clean generated files:

```bash
make clean
```

## How to Play

1. Launch the game.
2. Choose:
   - `N` for a new game
   - `L` to load a saved game
   - `Q` to quit
3. For a new game, choose `Normal Mode` or `Custom Mode`.
4. Enter player count and player names.
   - each seat can be a human or CPU player
   - CPU players support Easy, Normal, and Hard difficulty
5. Review or skip the tutorial.
6. During a turn:
   - press `Enter` to begin the turn
   - press `B` to use sabotage or buy defenses
   - press `Tab` to view the player scoreboard
   - press `G` to view the tile abbreviation guide
   - hold/release `Space` to spin
   - use menu keys when branch choices or popups appear
7. When all players retire, the final scoring screen declares the winner.

## Controls

- `N`: new game from the title screen
- `L`: load a saved game
- `Q`: quit or back out of many menus
- `Enter`: confirm / continue / start a turn
- `Space`: spin the wheel
- `B`: open sabotage and defense actions during a turn
- `Tab`: open the player scoreboard during a turn
- `G`: open the tile abbreviation guide during a turn
- `Up/Down`: move through lists and custom-mode toggles
- `Left/Right`: switch between normal mode and custom mode
- `K` or `?`: open the controls popup during the game

## Save / Load

Save files are stored in [`saves/`](/Users/chrlynn/comp2113/Goldrush/saves).

The save system preserves:

- rules and toggles
- player state
- current player and turn counter
- action history
- deck draw/discard piles
- RNG state

This means a loaded game resumes from the same state instead of rebuilding an approximate version.

## Project Structure

- [main.cpp]: application entry point
- [game.hpp], [game.cpp]: main game flow, turn resolution, menus, save/load integration
- [board.hpp], [board.cpp]: board definition and rendering
- [player.hpp], [player.cpp]: player data and worth calculation
- [cards.hpp], [cards.cpp]: card models, deck setup, roll-based card behavior
- [cpu_player.hpp], [cpu_player.cpp]: CPU player strategy, difficulty, and automated minigame results
- [sabotage.h], [sabotage.cpp], [sabotage_card.h], [sabotage_card.cpp]: sabotage actions, traps, defenses, and effect resolution
- [save_manager.hpp], [save_manager.cpp]: save-file listing, parsing, writing, duplicate archival
- [ui.h], [ui.cpp]: color setup and shared UI drawing helpers
- [rules.hpp], [rules.cpp]: rulesets and toggle presets
- [random_service.hpp]: deterministic RNG helpers
- [deck.hpp]: reusable deck container
- [Makefile]: build targets

## Contributors

- Cheralyn
- Michelle
- Yin
- Carla
- Joylin

## Notes

This project is for educational purposes. *The Game of Life* is a trademark of Hasbro.

Enjoy the game! May the best life win! 🎉
