#pragma once

#include "ExplainTree.h"

#include <vector>
#include <string>

namespace souffle {

class ExplainProvenance {
private:
    SouffleProgram& prog;
    SymbolTable& symTable;

public:
    ExplainProvenance(SouffleProgram& prog, SymbolTable& symTable) : prog(prog), symTable(symTable) {}

    virtual void setup();
    virtual TreeNode explain(std::string relName, std::vector<RamDomain> tuple);
};

} // end of namespace souffle


