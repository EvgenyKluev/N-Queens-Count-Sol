#include <chrono>
#include <cstdlib>
#include <iostream>

#include "solcounter.h"

int main(int argc, char* argv[])
{
    int threads = 4;
    int part = 0;
    int parts = 1;

    if (argc >= 2)
        threads = std::atoi(argv[1]);

    if (argc >= 3)
        parts = std::atoi(argv[2]);

    if (argc >= 4)
        part = std::atoi(argv[3]);

    auto startClock = std::chrono::high_resolution_clock::now();
    auto res = countSolutions(threads, part, parts);
    auto finishClock = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finishClock - startClock;
    std::cout << "Result: " << res << '\n';
    std::cout << "Elapsed time: " << elapsed.count() << " s\n";
}
