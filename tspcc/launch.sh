#!/bin/bash
g++ -g tspcc.cpp -o heyy.exe -latomic -pthread -O3
./heyy.exe -v2 dj38.tsp 1 256 4 1 16 1 8 8
