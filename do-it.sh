#!/bin/bash
g++ -g tspcc/tspcc.cpp -o tspcc/heyy.exe -latomic -pthread -O3
./tspcc/heyy.exe -v2 tspcc/dj38.tsp 1 256 4 1 16 1 8 8
