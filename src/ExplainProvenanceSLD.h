#pragma once

#include "ExplainProvenance.h"
#include "SouffleInterface.h"

namespace souffle {
    
class ExplainProvenanceSLD : public ExplainProvenance {
private:
    std::map<std::pair<std::string, std::vector<RamDomain>>, std::pair<RamDomain, RamDomain>> tuples;

public:
    ExplainProvenanceSLD(SouffleProgram& prog, SymbolTable& symTable) : ExplainProvenance(prog, symTable) {}

    /** Set up environment to run explain queries */
    void setup() {
        // store each tuple in a map marking its rule number and level number
        for (auto& rel : prog.getAllRelations()) {
            std::string name = rel.getName();

            // get each tuple
            for (auto& tuple : *rel) {
                std::vector<RamDomain> tupleElements;

                for (size_t i = 0; i < tuple.size() - 2; i++) {
                    if (*(rel->getAttrType(i)) == 'i' || *(rel->getAttrType(i)) == 'r') {
                        RamDomain n;
                        tuple >> n;
                        tupleElements.push_back(n);
                    } else if (*(rel->getAttrType(i)) == 's') {
                        std::string s;
                        tuple >> s;
                        tupleElements.push_back(symTable.lookup(s.c_str()));
                    }
                }

                RamDomain ruleNum;
                tuple >> ruleNum;

                RamDomain levelNum;
                tuple >> levelNum;

                tuples.insert({std::make_pair(name, tupleElements), std::make_pair(ruleNum, levelNum)});
            }
        }
    }

    TreeNode explain(std::string relName, std::vector<RamDomain> tuple) {

    }
};

} // end of namespace souffle
