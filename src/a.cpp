#include <iostream>
#include <limits>

int main() {
    uint64_t u64 = std::numeric_limits<uint64_t>::max();
    std::cout << u64 << std::endl;
    std::cout << static_cast<int64_t>(u64) << std::endl;
    std::cout << static_cast<int32_t>(u64) << std::endl;
    std::cout << static_cast<uint32_t>(u64) << std::endl;
    std::cout << std::endl;

    int32_t one = 1;
    int32_t minusOne = -1;

    std::cout << static_cast<int64_t>(one) << std::endl;
    std::cout << static_cast<int64_t>(minusOne) << std::endl;
}
