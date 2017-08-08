#pragma once

#include "ExplainProvenance.h"
#include "SouffleInterface.h"
#include "Util.h"

namespace souffle {

class ExplainProvenanceSLD : public ExplainProvenance {
private:
    // std::map<std::pair<std::string, std::vector<RamDomain>>, std::pair<RamDomain, RamDomain>> tuples;
    std::map<std::pair<std::string, size_t>, std::vector<std::string>> info;
    std::map<std::pair<std::string, size_t>, std::string> rules;

    std::pair<int, int> findTuple(std::string relName, std::vector<RamDomain> tup) {
        auto rel = prog.getRelation(relName);

        if (rel == nullptr) {
            return std::make_pair(-1, -1);
        }

        // find correct tuple
        for (auto& tuple : *rel) {
            bool match = true;
            std::vector<RamDomain> currentTuple;

            for (size_t i = 0; i < rel->getArity() - 2; i++) {
                RamDomain n;
                if (*rel->getAttrType(i) == 's') {
                    std::string s;
                    tuple >> s;
                    n = prog.getSymbolTable().lookupExisting(s);
                } else {
                    tuple >> n;
                }

                currentTuple.push_back(n);

                if (n != tup[i]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                RamDomain ruleNum;
                tuple >> ruleNum;

                RamDomain levelNum;
                tuple >> levelNum;

                return std::make_pair(ruleNum, levelNum);
            }
        }

        // if no tuple exists
        return std::make_pair(-1, -1);
    }

    std::vector<RamDomain> argsToNums(std::string relName, std::vector<std::string>& args) {
        std::vector<RamDomain> nums;

        auto rel = prog.getRelation(relName);
        if (rel == nullptr) {
            return std::vector<RamDomain>();
        }

        for (size_t i = 0; i < args.size(); i++) {
            if (*rel->getAttrType(i) == 's') {
                nums.push_back(prog.getSymbolTable().lookupExisting(args[i]));
            } else {
                nums.push_back(std::atoi(args[i].c_str()));
            }
        }

        return nums;
    }

    std::vector<std::string> numsToArgs(
            std::string relName, std::vector<RamDomain>& nums, std::vector<bool>* err = 0) {
        std::vector<std::string> args;

        auto rel = prog.getRelation(relName);
        if (rel == nullptr) {
            return std::vector<std::string>();
        }

        for (size_t i = 0; i < nums.size(); i++) {
            if (err && (*err)[i]) {
                args.push_back("_");
            } else {
                if (*rel->getAttrType(i) == 's') {
                    args.push_back(prog.getSymbolTable().resolve(nums[i]));
                } else {
                    args.push_back(std::to_string(nums[i]));
                }
            }
        }

        return args;
    }

public:
    ExplainProvenanceSLD(SouffleProgram& prog) : ExplainProvenance(prog) {
        setup();
    }

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

    void setup() override {
        // for each clause, store a mapping from the head relation name to body relation names
        for (auto& rel : prog.getAllRelations()) {
            std::string name = rel->getName();

            // only process info relations
            if (name.find("@info") == std::string::npos) {
                continue;
            }

            // find all the info tuples
            for (auto& tuple : *rel) {
                std::vector<std::string> bodyRels;

                RamDomain ruleNum;
                tuple >> ruleNum;

                for (size_t i = 1; i < rel->getArity() - 1; i++) {
                    std::string bodyRel;
                    tuple >> bodyRel;
                    bodyRels.push_back(bodyRel);
                }

                std::string rule;
                tuple >> rule;

                info.insert({std::make_pair(name.substr(0, name.find("-@info")), ruleNum), bodyRels});
                rules.insert({std::make_pair(name.substr(0, name.find("-@info")), ruleNum), rule});
            }
        }
    }

    std::unique_ptr<TreeNode> explain(
            std::string relName, std::vector<RamDomain> tuple, int ruleNum, int levelNum) {
        std::stringstream joinedArgs;
        joinedArgs << join(numsToArgs(relName, tuple), ", ");
        auto joinedArgsStr = joinedArgs.str();

        // if fact
        if (levelNum == 0) {
            return std::unique_ptr<TreeNode>(new LeafNode(relName + "(" + joinedArgsStr + ")"));
        }

        assert(info.find(std::make_pair(relName, ruleNum)) != info.end() && "invalid rule for tuple");

        auto internalNode = new InnerNode(relName + "(" + joinedArgsStr + ")");

        // make return vector pointer
        std::vector<RamDomain>* ret = new std::vector<RamDomain>();
        std::vector<bool>* err = new std::vector<bool>();

        // add level number to tuple
        tuple.push_back(levelNum);

        // execute subroutine to get subproofs
        prog.executeSubroutine(relName + "_" + std::to_string(ruleNum) + "_subproof", &tuple, ret, err);

        // recursively get nodes for subproofs
        size_t tupleCurInd = 0;
        for (std::string bodyRel : info[std::make_pair(relName, ruleNum)]) {
            // handle negated atom names
            auto bodyRelAtomName = bodyRel;
            if (bodyRel[0] == '!') {
                bodyRelAtomName = bodyRel.substr(1);
            }

            // traverse subroutine return using pointers
            size_t arity = prog.getRelation(bodyRelAtomName)->getArity();
            auto tupleEnd = tupleCurInd + arity;

            // store current tuple and error
            std::vector<RamDomain> subproofTuple;
            std::vector<bool> subproofTupleError;

            for (; tupleCurInd < tupleEnd - 2; tupleCurInd++) {
                subproofTuple.push_back((*ret)[tupleCurInd]);
                subproofTupleError.push_back((*err)[tupleCurInd]);
            }

            int subproofRuleNum = (*ret)[tupleCurInd];
            int subproofLevelNum = (*ret)[tupleCurInd + 1];

            if (bodyRel[0] == '!') {
                std::stringstream joinedTuple;
                joinedTuple << join(numsToArgs(bodyRelAtomName, subproofTuple, &subproofTupleError), ", ");
                auto joinedTupleStr = joinedTuple.str();
                internalNode->add_child(
                        std::unique_ptr<TreeNode>(new LeafNode(bodyRel + "(" + joinedTupleStr + ")")));
            } else {
                internalNode->add_child(explain(bodyRel, subproofTuple, subproofRuleNum, subproofLevelNum));
            }

            tupleCurInd = tupleEnd;
        }

        delete ret;
        delete err;

        return std::unique_ptr<TreeNode>(internalNode);
    }

    std::unique_ptr<TreeNode> explain(std::string relName, std::vector<std::string> args) override {
        auto tuple = argsToNums(relName, args);
        if (tuple == std::vector<RamDomain>()) {
            return std::unique_ptr<TreeNode>(new LeafNode("Relation not found"));
        }

        std::pair<int, int> tupleInfo = findTuple(relName, tuple);
        int ruleNum = tupleInfo.first;
        int levelNum = tupleInfo.second;

        if (ruleNum < 0 || levelNum == -1) {
            return std::unique_ptr<TreeNode>(new LeafNode("Tuple not found"));
        }

        return explain(relName, tuple, ruleNum, levelNum);
    }
};

}  // end of namespace souffle
