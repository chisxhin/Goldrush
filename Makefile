CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LIBS = -lncurses

OBJS = main.o game.o board.o player.o ui.o rules.o cards.o bank.o history.o spins.o save_manager.o

all: gameoflife

gameoflife: $(OBJS)
	$(CXX) $(CXXFLAGS) -o gameoflife $(OBJS) $(LIBS)

clean:
	rm -f *.o gameoflife

run: gameoflife
	./gameoflife
