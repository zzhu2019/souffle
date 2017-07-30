#pragma once

#include "ExplainTree.h"

#include <vector>
#include <string>

namespace souffle {

class ExplainProvenance {
private:
    SouffleProgram& prog;

public:
    ExplainProvenance(SouffleProgram& prog) : prog(prog) {}

    virtual void setup();
    virtual TreeNode explain(std::string relName, std::vector<RamDomain> tuple);
};

} // end of namespace souffle


