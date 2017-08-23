#pragma once

#include "ExplainProvenance.h"
#include <regex>

namespace souffle {

class ProvenanceInfo {
private:
    SouffleProgram& prog;

    std::map<std::pair<std::string, std::vector<RamDomain>>, RamDomain> valuesToLabel;
    std::map<std::pair<std::string, RamDomain>, std::vector<RamDomain>> labelToValue;
    std::map<std::pair<std::string, RamDomain>, std::vector<RamDomain>> labelToProof;
    std::map<std::string, std::vector<std::string>> info;
    std::map<std::pair<std::string, int>, std::string> rule;

public:
    ProvenanceInfo(SouffleProgram& p) : prog(p) {}

    void setup() {
        for (Relation* rel : prog.getAllRelations()) {
            std::string relName = rel->getName();

            for (auto& tuple : *rel) {
                RamDomain label;

                if (relName.find("-output") != std::string::npos) {
                    std::vector<RamDomain> tuple_elements;
                    tuple >> label;

                    // construct tuple elements
                    for (size_t i = 1; i < tuple.size(); i++) {
                        if (*(rel->getAttrType(i)) == 'i' || *(rel->getAttrType(i)) == 'r') {
                            RamDomain n;
                            tuple >> n;
                            tuple_elements.push_back(n);
                        } else if (*(rel->getAttrType(i)) == 's') {
                            std::string s;
                            tuple >> s;
                            auto n = prog.getSymbolTable().lookupExisting(s);
                            tuple_elements.push_back(n);
                        }
                    }

                    // insert into maps
                    valuesToLabel.insert({std::make_pair(rel->getName(), tuple_elements), label});
                    labelToValue.insert({std::make_pair(rel->getName(), label), tuple_elements});
                } else if (relName.find("-provenance-") != std::string::npos) {
                    std::vector<RamDomain> refs;
                    tuple >> label;

                    // construct vector of proof references
                    for (size_t i = 1; i < tuple.size(); i++) {
                        RamDomain l;
                        tuple >> l;
                        refs.push_back(l);
                    }

                    labelToProof.insert({std::make_pair(rel->getName(), label), refs});
                } else if (relName.find("-info") != std::string::npos) {
                    std::vector<std::string> rels;
                    for (size_t i = 0; i < tuple.size() - 2; i++) {
                        std::string s;
                        tuple >> s;
                        rels.push_back(s);
                    }

                    // second last argument is original relation name
                    std::string relName;
                    tuple >> relName;

                    // last argument is representation of clause
                    std::string clauseRepr;
                    tuple >> clauseRepr;

                    // extract rule number from relation name
                    int ruleNum = atoi((*(split(rel->getName(), '-').rbegin() + 1)).c_str());

                    info.insert({rel->getName(), rels});
                    rule.insert({std::make_pair(relName, ruleNum), clauseRepr});
                }
            }
        }
    }

    RamDomain getLabel(std::string relName, std::vector<RamDomain> e) {
        auto key = std::make_pair(relName, e);

        if (valuesToLabel.find(key) != valuesToLabel.end()) {
            return valuesToLabel[key];
        } else {
            return -1;
        }
    }

    std::vector<RamDomain> getTuple(std::string relName, RamDomain l) {
        auto key = std::make_pair(relName, l);

        if (labelToValue.find(key) != labelToValue.end()) {
            return labelToValue[key];
        } else {
            return std::vector<RamDomain>();
        }
    }

    std::vector<RamDomain> getSubproofs(std::string relName, RamDomain l) {
        auto key = std::make_pair(relName, l);

        if (labelToProof.find(key) != labelToProof.end()) {
            return labelToProof[key];
        } else {
            return std::vector<RamDomain>();
        }
    }

    std::vector<std::string> getInfo(std::string relName) {
        assert(info.find(relName) != info.end());
        return info[relName];
    }

    std::string getRule(std::string relName, int ruleNum) {
        auto key = std::make_pair(relName, ruleNum);

        if (rule.find(key) != rule.end()) {
            return rule[key];
        } else {
            return std::string("no rule found");
        }
    }
};

class ExplainProvenanceRecords : public ExplainProvenance {
private:
    ProvenanceInfo provInfo;
    std::unique_ptr<TreeNode> root;

public:
    ExplainProvenanceRecords(SouffleProgram& p) : ExplainProvenance(p), provInfo(p) {
        setup();
    }

    void setup() override {
        provInfo.setup();
    }

    std::unique_ptr<TreeNode> explainSubproof(std::string relName, RamDomain label, size_t depth) override {
        bool isEDB = true;
        bool found = false;
        for (auto rel : prog.getAllRelations()) {
            if (rel->getName().find(relName) != std::string::npos) {
                found = true;
            }
            std::regex provRelName(relName + "-provenance-[0-9]+");
            if (std::regex_match(rel->getName(), provRelName)) {
                isEDB = false;
                break;
            }
        }

        // check that relation is in the program
        if (!found) {
            return std::unique_ptr<TreeNode>(new LeafNode("Relation " + relName + " not found"));
        }

        // if EDB relation, make a leaf node in the tree
        if (prog.getRelation(relName) != nullptr && isEDB) {
            auto subproof = provInfo.getTuple(relName + "-output", label);

            // construct label
            std::stringstream tup;
            tup << join(numsToArgs(relName, subproof), ", ");
            std::string lab = relName + "(" + tup.str() + ")";

            // leaf node
            return std::unique_ptr<TreeNode>(new LeafNode(lab));
        } else {
            if (depth > 1) {
                std::string internalRelName;

                // find correct relation
                for (auto rel : prog.getAllRelations()) {
                    if (rel->getName().find(relName + "-provenance-") != std::string::npos) {
                        // if relation contains the correct tuple label
                        if (provInfo.getSubproofs(rel->getName(), label) != std::vector<RamDomain>()) {
                            // found the correct relation
                            internalRelName = rel->getName();
                            break;
                        }
                    }
                }

                // either fact or relation doesn't exist
                if (internalRelName == "") {
                    // if fact
                    auto tup = provInfo.getTuple(relName + "-output", label);
                    if (tup != std::vector<RamDomain>()) {
                        // output leaf provenance node
                        std::stringstream tupleText;
                        tupleText << join(numsToArgs(relName, tup), ", ");
                        std::string lab = relName + "(" + tupleText.str() + ")";

                        // leaf node
                        return std::unique_ptr<TreeNode>(new LeafNode(lab));
                    } else {
                        return std::unique_ptr<TreeNode>(new LeafNode("Relation " + relName + " not found"));
                    }
                }

                // label and rule number for current node
                auto subproof = provInfo.getTuple(relName + "-output", label);

                // construct label
                std::stringstream tup;
                tup << join(numsToArgs(relName, subproof), ", ");
                std::string lab = relName + "(" + tup.str() + ")";
                auto ruleNum = split(internalRelName, '-').back();

                // internal node representing current value
                auto inner =
                        std::unique_ptr<InnerNode>(new InnerNode(lab, std::string("(R" + ruleNum + ")")));

                // key for info map
                std::string infoKey = relName + "-info-" + ruleNum;

                // recursively add all provenance values for this value
                for (size_t i = 0; i < provInfo.getInfo(infoKey).size(); i++) {
                    auto rel = provInfo.getInfo(infoKey)[i];
                    auto newLab = provInfo.getSubproofs(internalRelName, label)[i];
                    inner->add_child(explainSubproof(rel, newLab, depth - 1));
                }
                return std::move(inner);

                // add subproof label if depth limit is exceeded
            } else {
                std::string lab = "subproof " + relName + "(" + std::to_string(label) + ")";
                return std::unique_ptr<TreeNode>(new LeafNode(lab));
            }
        }
    }

    std::unique_ptr<TreeNode> explain(
            std::string relName, std::vector<std::string> tuple, size_t depthLimit) override {
        auto lab = provInfo.getLabel(relName + "-output", argsToNums(relName, tuple));
        if (lab == -1) {
            return std::unique_ptr<TreeNode>(new LeafNode("Tuple not found"));
        }

        return explainSubproof(relName, lab, depthLimit);
    }

    std::string getRule(std::string relName, size_t ruleNum) override {
        return provInfo.getRule(relName, ruleNum);
    }
};

}  // end of namespace souffle
