CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lncurses
FS_LIBS ?=

OBJS = main.o game.o board.o player.o ui.o rules.o \
		cards.o bank.o history.o spins.o save_manager.o \
		pong.o battleship.o hangman.o memory.o minesweeper.o

all: gameoflife

gameoflife: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gameoflife $(OBJS) $(LIBS) $(FS_LIBS)

clean:
	rm -f *.o gameoflife

run: gameoflife
	./gameoflife

# Dependencies
hangman.o: hangman.cpp hangman.hpp ui.h
memory.o: memory.cpp memory.hpp ui.h
game.o: game.cpp game.hpp hangman.hpp memory.hpp pong.hpp battleship.hpp minesweeper.hpp
minesweeper.o: minesweeper.cpp minesweeper.hpp ui.h
