#  Copyright (c) 2012 Marcelo Pasin. All rights reserved.

CFLAGS=-O3 -Wall
LDFLAGS=-O3 -lm -latomic

all: tspcc atomic

tspcc: tspcc.o
	c++ -o tspcc $(LDFLAGS) tspcc.o

tspcc.o: tspcc.cpp graph.hpp path.hpp tspfile.hpp
	c++ $(CFLAGS) -c tspcc.cpp

atomic: atomic.cpp
	g++ -o atomic atomic.cpp -latomic -pthread

omp:
	make tspcc CFLAGS="-fopenmp -O3" LDFLAGS="-fopenmp -O3 -latomic"

clean:
	rm -f *.o tspcc atomic
