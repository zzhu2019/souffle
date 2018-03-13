#include "ParallelUtils.h"

#include <iostream>
#include <mutex>

static constexpr size_t SIZE = 1000000001u;
volatile size_t count = 0;
using namespace souffle;

int main() {
    Lock lock;
    for (size_t i = 0; i < SIZE; ++i) {
        ++count;
    }
    std::cout << count << std::endl;
}

// int main() {
//    std::mutex lock;
//    for (size_t i = 0; i < SIZE; ++i) {
//        std::lock_guard<std::mutex> guard(lock);
//        ++count;
//    }
//    std::cout << count << std::endl;
//}
