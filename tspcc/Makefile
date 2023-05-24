CFLAGS=-O3 -Wall -mcx16
LDFLAGS=-O3 -lm -latomic -Xlinker -latomic
DCMAKE_CXX_FLAGS=-latomic

all: tspcc atomic

tspcc: tspcc.o
	c++ -o tspcc $(LDFLAGS) tspcc.o

tspcc.o: tspcc.cpp graph.hpp path.hpp tspfile.hpp
	c++ $(CFLAGS) -c tspcc.cpp

atomic: atomic.cpp
	g++ -o atomic atomic.cpp -latomic

omp:
	make tspcc CFLAGS="-fopenmp -O3" LDFLAGS="-fopenmp -O3"

clean:
	rm -f *.o tspcc atomic
