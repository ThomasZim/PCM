// SynchronizedPath.hpp

#ifndef _synchronized_path_hpp
#define _synchronized_path_hpp

#include <iostream>
#include <atomic>
#include "path.hpp"
#include "graph.hpp"

class SynchronizedPath : public Path {
private:
    std::atomic_flag _atomicLock = ATOMIC_FLAG_INIT;

public:

	SynchronizedPath(Graph* givenGraph)
	:Path(givenGraph)
	{}

	void compareAndUpdate(Path* otherPath)
	{
		while (_atomicLock.test_and_set(std::memory_order_acquire)) {
            // Spinlock in action until the lock can be acquired
        }

		if(otherPath->distance() < distance()){
			copy(otherPath);
		}

		_atomicLock.clear(std::memory_order_release); // Unlocking for other threads
	}
};
#endif // _synchronized_path_hpp
