all: gcc.out icpc.out
SRC=$(shell ls *.cpp)
OBJ=$(SRC:.cpp=.o)

GCCFLAGS=-O3 -march=native -Wall -Wextra -std=c++11 
ICPCFLAGS=-O3 -xHOST -Wall -Wextra -std=c++11 

-include makefile.opt

gcc.out: $(SRC)
	g++ $(GCCFLAGS) $^ -o $@

icpc.out: $(SRC)
	icpc $(ICPCFLAGS) $^ -o $@

run: gcc.out icpc.out
	./gcc.out
	./icpc.out

clean:
	rm -f gcc.out icpc.out
