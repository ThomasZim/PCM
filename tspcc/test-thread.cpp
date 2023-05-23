// Test thread
// -----------------------------------------------------------------------

#include <iostream>
#include <thread>

void thread_func(int id)
{
    std::cout << "Hello, thread! id :" << id << "\n";
}

int main(int argc, char* argv[])
{
    std::thread t(thread_func, 1);
    t.join();
    std::cout << "Hello, main!\n";
    return 0;
}