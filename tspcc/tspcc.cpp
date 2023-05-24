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
#include "atomic.hpp"
#include "ConcurrentReuseQueue.hpp"
#include <atomic>
#include <fstream> // Ajoutez cette ligne en haut de votre fichier
static const int MAX_THREAD_DEPTH = 2;

// Vector of threads
std::vector<std::thread> threads;

// Vector of paths
ConcurrentReuseQueue<Path> paths;

// Mutex print
// std::mutex print_mutex;

// std::mutex shortest_mutex;

// Number of cities
int cities;

enum Verbosity {
	VER_NONE = 0,
	VER_GRAPH = 1,
	VER_SHORTER = 2,
	VER_BOUND = 4,
	VER_ANALYSE = 8,
	VER_COUNTERS = 16,
};

static struct {
	std::atomic<Path*> shortest;
	Verbosity verbose;
	struct {
		std::atomic<int> verified;	// # of paths checked
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

// Concurent list for atomic_stamped<Path*>


static void concurrent_branch_and_bound(Path* current, int depth);

static void thread_work(){
	
	Path* current = nullptr;
	while (global.total != global.counter.verified){
		// Print the path current
		/*print_mutex.lock();
		std::cout << "Global counter verified : " << global.counter.verified << "\n";
		std::cout << "Global total : " << global.total << "\n";
		print_mutex.unlock();*/
		current = paths.dequeue();
		if (current == nullptr){
			// Go to the beginning of the loop
			continue;
		}
		// Cout the path current
		//std::cout << current << '\n';
		concurrent_branch_and_bound(current, current->size());
		// print_mutex.lock();
		// std::cout << "Global counter verified : " << global.counter.verified << "\n";
		// print_mutex.unlock();
	}
	/*print_mutex.lock();
	std::cout << "Bye, thread!" << "\n";
	print_mutex.unlock();*/
}

static void concurrent_branch_and_bound(Path* current, int depth=0){
	//print_mutex.lock();
	//shortest_mutex.lock();
	/*print_mutex.lock();
	std::cout << '\n';
	print_mutex.unlock();*/
	if (current->leaf()){
		
		/*print_mutex.lock();
		std::cout << "LEAF     depth: " << depth << "   counter: " << global.counter.verified << "     current: " << current << '\n';
		print_mutex.unlock();*/
		// Print the current path node with for loop
		current->add(0);

		global.counter.verified ++;
		//std::cout << "verified: " << current << " depth : " << depth << " global struct :  counter : " << global.counter.verified << "\n";
		if (global.verbose & VER_COUNTERS){
			global.counter.verified ++;
		}
		/*print_mutex.lock();
		std::cout << "Current distance: " << current->distance() << "                     Shortest distance: " << global.shortest.load(std::memory_order_relaxed)->distance() << "\n";
		print_mutex.unlock();*/
		if (current->distance() < global.shortest.load(std::memory_order_relaxed)->distance()) {
			if (global.verbose & VER_SHORTER){
				/*print_mutex.lock();
				std::cout << "shorter: " << current << " depth : " << depth << " global struct :  counter : " << global.counter.verified << "\n";
				print_mutex.unlock();*/
			}
			global.shortest.load(std::memory_order_relaxed)->copy(current);
			if (global.verbose & VER_COUNTERS){
				global.counter.found ++;
			}
		}
	}
	else{
		/*print_mutex.lock();
		std::cout << "NOT LEAF     depth: " << depth << "   counter: " << global.counter.verified << "     current: " << current << '\n';
		print_mutex.unlock();*/
		// Not a leaf
		// not yet a leaf
		if (current->distance() < global.shortest.load(std::memory_order_relaxed)->distance()) {
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
						paths.enqueue(next);
					}
				}
			} else {
				// current already >= shortest known so far, bound
				if (global.verbose & VER_BOUND )
					std::cout << "bound " << current << '\n';
				if (global.verbose & VER_COUNTERS){
					global.counter.bound[current->size()] ++;
				}

				// Calculate the number of paths that killed
				int temp = (cities - current->size());

				int result = 1;
		
				for (int i = 1; i <= temp; ++i) {
					result *= i;
				}	
				global.counter.verified += result;
			}
	}
	//shortest_mutex.unlock();
	//print_mutex.unlock();
}
// static void branch_and_bound(Path* current, int depth=0)
// {
// 	if (global.verbose & VER_ANALYSE)
// 		std::cout << "analysing " << current << '\n';

// 	if (current->leaf()) {
// 		// this is a leaf
// 		current->add(0);
// 		if (global.verbose & VER_COUNTERS){
// 			global.counter.verified ++;
// 		}
// 		if (current->distance() < global.shortest->distance()) {
// 			if (global.verbose & VER_SHORTER){
// 				std::cout << "shorter: " << current << '\n';
// 			}
// 			global.shortest->copy(current);
// 			if (global.verbose & VER_COUNTERS){
// 				global.counter.found ++;
// 			}
// 		}
// 		current->pop();
// 	} else {

//      // not yet a leaf
//     if (current->distance() < global.shortest->distance()) {
// 			// continue branching
// 			for (int i=1; i<current->max(); i++) {
// 				if (!current->contains(i)) {
// 					current->add(i);
// 					std::cout << "depth: " << depth << "   Index: " << i << '\n';
// 					if (depth < MAX_THREAD_DEPTH) {
// 						// threads.push_back(std::thread(branch_and_bound, current, depth + 1));

// 						// branch_and_bound(current, depth + 1);
// 					} else {
// 						branch_and_bound(current, depth + 1);
// 					}
// 					current->pop();
// 				}
// 			}
// 			// Attente de l'achèvement des threads si nous sommes à la racine de l'arbre de recherche.
// 			// if (depth == 0) {
// 			// 	for (std::thread &t : threads) {
// 			// 		t.join();
// 			// 		std::cout << "thread joined\n";
// 			// 	}
// 			// 	threads.clear();
// 			// }
// 		} else {
// 			// current already >= shortest known so far, bound
// 			if (global.verbose & VER_BOUND )
// 				std::cout << "bound " << current << '\n';
// 			if (global.verbose & VER_COUNTERS){
// 				global.counter.bound[current->size()] ++;
// 			}
// 		}
// 	}
// }

/*static void branch_and_bound(Path* current)
{
	std::cout << '\n';
	if (global.verbose & VER_ANALYSE)
		std::cout << "analysing " << current << '\n';

	
	if (current->leaf()) {
		// this is a leaf
		current->add(0);
		if (global.verbose & VER_COUNTERS)
			global.counter.verified ++;
		std::cout << "LEAF   counter: " << global.counter.verified << "     current: " << current << '\n';
		std::cout << "Current distance: " << current->distance() << "                     Shortest distance: " << global.shortest->distance() << "\n";
		if (current->distance() < global.shortest->distance()) {
			if (global.verbose & VER_SHORTER)
				std::cout << "shorter: " << current << '\n';
			global.shortest->copy(current);
			if (global.verbose & VER_COUNTERS)
				global.counter.found ++;
		}
		current->pop();
	} else {
		// not yet a leaf
		if (current->distance() < global.shortest->distance()) {
			// continue branching
			for (int i=1; i<current->max(); i++) {
				if (!current->contains(i)) {
					current->add(i);
					branch_and_bound(current);
					current->pop();
				}
			}
		} else {
			// current already >= shortest known so far, bound
			if (global.verbose & VER_BOUND )
				std::cout << "bound " << current << '\n';
			if (global.verbose & VER_COUNTERS)
				global.counter.bound[current->size()] ++;
		}
	}
}*/


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
    std::ofstream outputFile("output.csv"); // Ouvrir un nouveau fichier CSV
    outputFile << "i_avg,i_city,i_thread,elapsed_time\n"; // Écrire les en-têtes des colonnes
	char* fname = 0;
	int MAX_THREAD = 1;
	int THREAD_INCREMENT = 1;
	int MAX_CITY = 1;
	int THREAD_NUMBER = 2;
	if (argc == 2) {
		fname = argv[1];
		global.verbose = VER_NONE;
	} else {
		if (argc == 6 && argv[1][0] == '-' && argv[1][1] == 'v') {
			global.verbose = (Verbosity) (argv[1][2] ? atoi(argv[1]+2) : 1);
			fname = argv[2];
			MAX_THREAD = atoi(argv[3]);
			THREAD_INCREMENT = atoi(argv[4]);
			MAX_CITY = atoi(argv[5]);
		} else {
			fprintf(stderr, "usage: %s [-v#] filename\n", argv[0]);
			exit(1);
		}
	}
	for (int i_avg = 0; i_avg < 10; i_avg++){
		for (int i_thread=1; i_thread<MAX_THREAD+1; i_thread+=THREAD_INCREMENT) {
			if (i_thread == 1+THREAD_INCREMENT)
				i_thread = THREAD_INCREMENT;
			for (int i_city=1; i_city<MAX_CITY+1; i_city++) {	
				std::cout << "Thread: " << i_thread << "     City: " << i_city << '\n';
				Graph* g = TSPFile::graph(fname, i_city);
				if (global.verbose & VER_GRAPH)
					std::cout << COLOR.BLUE << g << COLOR.ORIGINAL;

				reset_counters(g->size());

				std::cout << "Graph size: " << g->size() << '\n';
				cities = g->size();
				// Calculate the total number of paths there is 8! = 40320
				// 12! = 479001600
				// 9! = 362880
				global.total = 1;
				// Calc factorial of g->size()
				for (int i=1; i<g->size(); i++) {
					global.total *= (i);
				}
				std::cout << "Total number of paths: " << global.total << '\n';

				global.shortest = new Path(g);
				for (int i=0; i<g->size(); i++) {
					global.shortest.load(std::memory_order_relaxed)->add(i);
				}
				global.shortest.load(std::memory_order_relaxed)->add(0);

				Path* current = new Path(g);
				current->add(0);
				paths.enqueue(current);

				// Calculate time taken to run the program
				auto start = std::chrono::high_resolution_clock::now();

				// Create a thread and start branching
				// Create 10 threads with the same function
				for (int i = 0; i < i_thread; ++i)
				{
					//std::cout << "Creating thread " << i << '\n';
					threads.push_back(std::thread(thread_work));
				}

				// Join the threads with the main thread
				for(auto& thread : threads){
					thread.join();
				}

				auto finish = std::chrono::high_resolution_clock::now();


				// Wait for the thread to finish

				threads.clear();

				//thread_work();
				

				//branch_and_bound(current);

				std::cout << COLOR.RED << "shortest " << global.shortest << COLOR.ORIGINAL << '\n';

				std::chrono::duration<double> elapsed = finish - start;
				std::cout << "Time taken: " << elapsed.count() << " s\n";

				// Calculate time taken to run the program

				outputFile << i_avg << "," << i_city << "," << i_thread << "," << elapsed.count() << "\n";


				if (global.verbose & VER_COUNTERS)
					print_counters();
			}
		}
	}
	outputFile.close(); // Fermer le fichier
	return 0;
}