#pragma once

#include "RamStatement.h"

namespace souffle {

class RamProgram {
private:
    std::unique_ptr<RamStatement> main;
    std::map<std::string, std::unique_ptr<RamStatement>> subroutines;

public:
    std::unique_ptr<RamStatement> getMain() {
        return main;
    }

    std::unique_ptr<RamStatement> getSubroutine(std::string name) {
        if (subroutines.get(name) != subroutines.end()) {
            return subroutines[name];
        } else {
            return nullptr;
        }
    }
}

} // end of namespace souffle
