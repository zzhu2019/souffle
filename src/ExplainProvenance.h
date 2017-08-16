#pragma once

#include "ExplainTree.h"
#include "SouffleInterface.h"

#include <string>
#include <vector>

namespace souffle {

class ExplainProvenance {
protected:
    SouffleProgram& prog;

public:
    ExplainProvenance(SouffleProgram& prog) : prog(prog) {}

    virtual void setup() = 0;
    virtual std::unique_ptr<TreeNode> explain(
            std::string relName, std::vector<std::string> tuple, size_t depthLimit) = 0;
};

}  // end of namespace souffle
