CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lncurses
FS_LIBS ?=

CORE_OBJS = game.o board.o player.o ui.o rules.o \
		cards.o bank.o history.o spins.o save_manager.o \
		cpu_player.o sabotage.o sabotage_card.o ui_helpers.o \
		minigame_tutorials.o timer_display.o tile_display.o \
		pong.o battleship.o hangman.o memory.o minesweeper.o

OBJS = main.o $(CORE_OBJS)
DEBUG_OBJS = debug.o $(CORE_OBJS)

all: gameoflife

gameoflife: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gameoflife $(OBJS) $(LIBS) $(FS_LIBS)

debug: $(DEBUG_OBJS)
	$(CXX) $(CXXFLAGS) -o debug $(DEBUG_OBJS) $(LIBS) $(FS_LIBS)

clean:
	rm -f *.o gameoflife debug

run: gameoflife
	./gameoflife

run-debug: debug
	./debug

# Dependencies
debug.o: debug.cpp debug.h bank.hpp battleship.hpp board.hpp cards.hpp cpu_player.hpp game.hpp hangman.hpp memory.hpp minesweeper.hpp player.hpp pong.hpp random_service.hpp rules.hpp sabotage.h sabotage_card.h save_manager.hpp spins.hpp ui.h ui_helpers.h minigame_tutorials.h timer_display.h tile_display.h
board.o: board.cpp board.hpp tile_display.h
cards.o: cards.cpp cards.hpp deck.hpp random_service.hpp rules.hpp
cpu_player.o: cpu_player.cpp cpu_player.hpp cards.hpp player.hpp random_service.hpp rules.hpp sabotage_card.h
sabotage.o: sabotage.cpp sabotage.h sabotage_card.h bank.hpp player.hpp random_service.hpp
sabotage_card.o: sabotage_card.cpp sabotage_card.h
save_manager.o: save_manager.cpp save_manager.hpp game.hpp
ui_helpers.o: ui_helpers.cpp ui_helpers.h ui.h
minigame_tutorials.o: minigame_tutorials.cpp minigame_tutorials.h ui_helpers.h
timer_display.o: timer_display.cpp timer_display.h ui.h
tile_display.o: tile_display.cpp tile_display.h board.hpp
hangman.o: hangman.cpp hangman.hpp ui.h ui_helpers.h minigame_tutorials.h
memory.o: memory.cpp memory.hpp ui.h ui_helpers.h minigame_tutorials.h timer_display.h
game.o: game.cpp game.hpp cpu_player.hpp sabotage.h sabotage_card.h hangman.hpp memory.hpp pong.hpp battleship.hpp minesweeper.hpp ui_helpers.h timer_display.h tile_display.h
minesweeper.o: minesweeper.cpp minesweeper.hpp ui.h ui_helpers.h minigame_tutorials.h timer_display.h
pong.o: pong.cpp pong.hpp ui.h ui_helpers.h minigame_tutorials.h
battleship.o: battleship.cpp battleship.hpp ui.h ui_helpers.h minigame_tutorials.h
