CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lncurses
FS_LIBS ?=

OBJS = main.o game.o board.o player.o ui.o rules.o cards.o bank.o history.o spins.o save_manager.o pong.o

all: gameoflife

gameoflife: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gameoflife $(OBJS) $(LIBS) $(FS_LIBS)

clean:
	rm -f *.o gameoflife

run: gameoflife
	./gameoflife
