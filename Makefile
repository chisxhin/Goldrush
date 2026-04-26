CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lncurses
FS_LIBS ?=

CORE_OBJS = game.o board.o player.o ui.o rules.o \
		cards.o bank.o history.o spins.o save_manager.o \
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
debug.o: debug.cpp debug.h bank.hpp battleship.hpp board.hpp cards.hpp game.hpp hangman.hpp memory.hpp minesweeper.hpp player.hpp pong.hpp random_service.hpp rules.hpp save_manager.hpp spins.hpp ui.h
hangman.o: hangman.cpp hangman.hpp ui.h
memory.o: memory.cpp memory.hpp ui.h
game.o: game.cpp game.hpp hangman.hpp memory.hpp pong.hpp battleship.hpp minesweeper.hpp
minesweeper.o: minesweeper.cpp minesweeper.hpp ui.h
