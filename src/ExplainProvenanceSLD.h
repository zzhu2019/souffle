#pragma once

#include "ExplainProvenance.h"
#include "SouffleInterface.h"

namespace souffle {
    
class ExplainProvenanceSLD : public ExplainProvenance {
private:
    std::map<std::pair<std::string, std::vector<RamDomain>>, std::pair<RamDomain, RamDomain>> tuples;
    std::map<std::pair<std::string, size_t>, std::vector<std::string>> info;
    std::map<std::pair<std::string, size_t>, std::string> rules;

    std::pair<int, int> findTuple(std::string relName, std::vector<RamDomain> tuple) {
        auto rel = prog.getRelation(relName);

        if (rel == nullptr) {
            return std::make_pair<-1, -1>;
        }

        // find correct tuple
        for (auto& tuple : *rel) {
            bool match = true;
            std::vector<RamDomain> currentTuple;

            for (size_t i = 0; i < rel.getArity() - 2; i++) {
                RamDomain n;
                if (*rel.getAttrType(i) == 's') {
                    std::string s;
                    tuple >> s;
                    n = prog.getSymbolTable().lookup(s.c_str());
                } else {
                    tuple >> n;
                }

                if (n != tuple[n]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                RamDomain ruleNum;
                tuple >> ruleNum;

                RamDomain levelNum;
                tuple >> levelNum;

                return std::make_pair<ruleNum, levelNum>;
            }
        }

        // if no tuple exists
        return std::make_pair<-1, -1>;
    }

public:
    ExplainProvenanceSLD(SouffleProgram& prog, SymbolTable& symTable) : ExplainProvenance(prog, symTable) {}

    /** Set up environment to run explain queries */
    /*
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
    */

    void setup() {
        for (auto& rel : prog.getAllRelations()) {
            std::string name = rel.getName();

            if (name.find("@info") == std::string.npos) {
                continue;
            }

            for (auto& tuple : *rel) {
                std::vector<std::string> bodyRels;

                RamDomain ruleNum;
                tuple >> ruleNum;

                for (size_t i = 1; i < rel.getArity() - 1; i++) {
                    std::string bodyRel;
                    tuple >> bodyRel;
                    bodyRels.push_back(bodyRel);
                }

                std::string rule;
                tuple >> rule;

                info.insert({std::make_pair(name, ruleNum), bodyRels});
                rules.insert({std::make_pair(name, ruleNum), rule});
            }
        }
    }

    std::unique_ptr<TreeNode> explain(std::string relName, std::vector<RamDomain> tuple) {
        std::pair<int, int> tupleInfo = find(relName, tuple);
        int ruleNum = tupleInfo.first;
        int levelNum = tupleInfo.second;

        if (ruleNum == -1 || levelNum == -1) {
            return std::unique_ptr<TreeNode>(new LeafNode("Tuple not found"));
        }

        auto internalNode = new InnerNode(
        LeafNode l("");
        return l;
    }
};

} // end of namespace souffle
