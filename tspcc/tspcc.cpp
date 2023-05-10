//
//  tspcc.cpp
//  
//  Copyright (c) 2022 Marcelo Pasin. All rights reserved.
//

#include "graph.hpp"
#include "path.hpp"
#include "tspfile.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
static const int MAX_THREAD_DEPTH = 2;

// Vector of threads
std::vector<std::thread> threads;

// Vector of paths
std::vector<Path*> paths;

// Create a mutex
std::mutex mtx;

// Create shortest path mutex
std::mutex shortest_mtx;

enum Verbosity {
	VER_NONE = 0,
	VER_GRAPH = 1,
	VER_SHORTER = 2,
	VER_BOUND = 4,
	VER_ANALYSE = 8,
	VER_COUNTERS = 16,
};

static struct {
	Path* shortest;
	Verbosity verbose;
	struct {
		int verified;	// # of paths checked
		int found;	// # of times a shorter path was found
		int* bound;	// # of bound operations per level
	} counter;
	int size;
	int total;		// number of paths to check
	int* fact;
} global;

static const struct {
	char RED[6];
	char BLUE[6];
	char ORIGINAL[6];
} COLOR = {
	.RED = { 27, '[', '3', '1', 'm', 0 },
	.BLUE = { 27, '[', '3', '6', 'm', 0 },
	.ORIGINAL = { 27, '[', '3', '9', 'm', 0 },
};
static void concurrent_branch_and_bound(Path* current, int depth);

static void thread_work(){
	
	Path* current = nullptr;
	while (paths.size() > 0){
		mtx.lock();
		// Print the path current
		current = paths.back();
		paths.pop_back();
		mtx.unlock();
		// Cout the path current
		concurrent_branch_and_bound(current, current->size());
	}
	std::cout << "Bye, thread!" << "\n";
}

static void concurrent_branch_and_bound(Path* current, int depth=0){
	if (current->leaf()){
		// Print the current path node with for loop
		current->add(0);
		if (global.verbose & VER_COUNTERS){
			global.counter.verified ++;
		}
		if (current->distance() < global.shortest->distance()) {
			if (global.verbose & VER_SHORTER){
				std::cout << "shorter: " << current << '\n';
			}
			shortest_mtx.lock();
			global.shortest->copy(current);
			shortest_mtx.unlock();
			if (global.verbose & VER_COUNTERS){
				global.counter.found ++;
			}
		}
	}
	else{
		// Not a leaf
     // not yet a leaf
    if (current->distance() < global.shortest->distance()) {
			// continue branching
			for (int i=1; i<current->max(); i++) {
				Path* next;
				if (!current->contains(i)) {
					// create a new path
					next = new Path(*current);
					// Cout the path next
					// std::cout << next << "\n";
					next->add(i);
					// enqueue it
					paths.push_back(next);
				}
			}
		} else {
			// current already >= shortest known so far, bound
			if (global.verbose & VER_BOUND )
				std::cout << "bound " << current << '\n';
			if (global.verbose & VER_COUNTERS){
				global.counter.bound[current->size()] ++;
			}
		}
	}
}
static void branch_and_bound(Path* current, int depth=0)
{
	if (global.verbose & VER_ANALYSE)
		std::cout << "analysing " << current << '\n';

	if (current->leaf()) {
		// this is a leaf
		current->add(0);
		if (global.verbose & VER_COUNTERS){
			global.counter.verified ++;
		}
		if (current->distance() < global.shortest->distance()) {
			if (global.verbose & VER_SHORTER){
				std::cout << "shorter: " << current << '\n';
			}
			mtx.lock();
			global.shortest->copy(current);
			mtx.unlock();
			if (global.verbose & VER_COUNTERS){
				global.counter.found ++;
			}
		}
		current->pop();
	} else {

     // not yet a leaf
    if (current->distance() < global.shortest->distance()) {
			// continue branching
			for (int i=1; i<current->max(); i++) {
				if (!current->contains(i)) {
					current->add(i);
					std::cout << "depth: " << depth << "   Index: " << i << '\n';
					if (depth < MAX_THREAD_DEPTH) {
						// threads.push_back(std::thread(branch_and_bound, current, depth + 1));

						// branch_and_bound(current, depth + 1);
					} else {
						branch_and_bound(current, depth + 1);
					}
					current->pop();
				}
			}
			// Attente de l'achèvement des threads si nous sommes à la racine de l'arbre de recherche.
			// if (depth == 0) {
			// 	for (std::thread &t : threads) {
			// 		t.join();
			// 		std::cout << "thread joined\n";
			// 	}
			// 	threads.clear();
			// }
		} else {
			// current already >= shortest known so far, bound
			if (global.verbose & VER_BOUND )
				std::cout << "bound " << current << '\n';
			if (global.verbose & VER_COUNTERS){
				global.counter.bound[current->size()] ++;
			}
		}
	}
}


void reset_counters(int size)
{
	global.size = size;
	global.counter.verified = 0;
	global.counter.found = 0;
	global.counter.bound = new int[global.size];
	global.fact = new int[global.size];
	for (int i=0; i<global.size; i++) {
		global.counter.bound[i] = 0;
		if (i) {
			int pos = global.size - i;
			global.fact[pos] = (i-1) ? (i * global.fact[pos+1]) : 1;
		}
	}
	global.total = global.fact[0] = global.fact[1];
}

void print_counters()
{
	std::cout << "total: " << global.total << '\n';
	std::cout << "verified: " << global.counter.verified << '\n';
	std::cout << "found shorter: " << global.counter.found << '\n';
	std::cout << "bound (per level):";
	for (int i=0; i<global.size; i++)
		std::cout << ' ' << global.counter.bound[i];
	std::cout << "\nbound equivalent (per level): ";
	int equiv = 0;
	for (int i=0; i<global.size; i++) {
		int e = global.fact[i] * global.counter.bound[i];
		std::cout << ' ' << e;
		equiv += e;
	}
	std::cout << "\nbound equivalent (total): " << equiv << '\n';
	std::cout << "check: total " << (global.total==(global.counter.verified + equiv) ? "==" : "!=") << " verified + total bound equivalent\n";
}

int main(int argc, char* argv[])
{
	char* fname = 0;
	if (argc == 2) {
		fname = argv[1];
		global.verbose = VER_NONE;
	} else {
		if (argc == 3 && argv[1][0] == '-' && argv[1][1] == 'v') {
			global.verbose = (Verbosity) (argv[1][2] ? atoi(argv[1]+2) : 1);
			fname = argv[2];
		} else {
			fprintf(stderr, "usage: %s [-v#] filename\n", argv[0]);
			exit(1);
		}
	}

	Graph* g = TSPFile::graph(fname);
	if (global.verbose & VER_GRAPH)
		std::cout << COLOR.BLUE << g << COLOR.ORIGINAL;

	if (global.verbose & VER_COUNTERS)
		reset_counters(g->size());

	global.shortest = new Path(g);
	for (int i=0; i<g->size(); i++) {
		global.shortest->add(i);
	}
	global.shortest->add(0);

	Path* current = new Path(g);
	current->add(0);
	paths.push_back(current);
	// Create a thread and start branching
	// Create 10 threads with the same function
	// for (int i = 0; i < 1; ++i)
	// {
	// 	threads.push_back(std::thread(thread_work));
	// }
	threads.push_back(std::thread(thread_work));
	// threads.push_back(std::thread(thread_work));
	//threads.push_back(std::thread(thread_work));

	// Join the threads with the main thread
	for(auto& thread : threads){
		thread.join();
	}




	// Wait for the thread to finish

	threads.clear();

	//thread_work();
	

	// branch_and_bound(current);

	std::cout << COLOR.RED << "shortest " << global.shortest << COLOR.ORIGINAL << '\n';

	if (global.verbose & VER_COUNTERS)
		print_counters();

	return 0;
}