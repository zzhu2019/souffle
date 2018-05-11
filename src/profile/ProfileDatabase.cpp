#include "ProfileDatabase.h"
#include <chrono>

int main() {
    souffle::profile::ProfileDatabase x;
    x.addSizeEntry({"akey"}, 1);
    x.addSizeEntry({"a", "b", "bkey"}, 2);
    x.addSizeEntry({"a", "c", "bkey"}, 3);
    x.addTextEntry({"a", "x", "akey"}, "blabla");
    x.addDurationEntry({"a", "x", "akey"},
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock().now().time_since_epoch()),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock().now().time_since_epoch()));
    std::cout << "print(std::cout" << std::endl;
    x.print(std::cout);

    std::cout << "\n\nSum of bkey:" << x.computeSum({"a", "bkey"}) << std::endl;

    return 0;
}
