#pragma once

#include "SouffleInterface.h"

namespace souffle {

inline void explain(SouffleProgram& prog) {
    std::vector<RamDomain>* ret = new std::vector<RamDomain>();
    const std::vector<RamDomain>* args = new std::vector<RamDomain>({1, 4, 3});
    prog.executeSubroutine("path_1_subproof", args, ret);

    for (auto i : *ret) {
        std::cout << i << std::endl;
    }
}

}  // end of namespace souffle
