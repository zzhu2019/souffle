/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "profilerlib/Reader.h"
#include "profilerlib/Iteration.h"
#include "profilerlib/ProgramRun.h"
#include "profilerlib/Relation.h"
#include "profilerlib/Rule.h"
#include "profilerlib/StringUtils.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <thread>
#include <dirent.h>
#include <sys/stat.h>

void Reader::processFile() {
    auto programDuration =
            dynamic_cast<souffle::profile::DurationEntry*>(db.lookupEntry({"program", "runtime"}));
    if (programDuration == nullptr) {
        std::cout << "db is empty!" << std::endl;
        db.print(std::cout);
        return;
    }
    runtime = (programDuration->getEnd() - programDuration->getStart()).count() / 1000.0;

    std::cout << "runtime = " << runtime << std::endl;
    std::cout << "db:" << std::endl;

    for (const auto& cur :
            dynamic_cast<souffle::profile::DirectoryEntry*>(db.lookupEntry({"program", "relation"}))
                    ->getKeys()) {
        addRelation(*dynamic_cast<souffle::profile::DirectoryEntry*>(
                db.lookupEntry({"program", "relation", cur})));
    }
    run->SetRuntime(this->runtime);
    run->setRelation_map(this->relation_map);
    loaded = true;
}

namespace {
template <typename T>
class DSNVisitor : public souffle::profile::Visitor {
public:
    DSNVisitor(T& base) : base(base) {}
    void visit(souffle::profile::TextEntry& text) override {
        if (text.getKey() == "source-locator") {
            base.setLocator(text.getText());
        }
    }
    void visit(souffle::profile::DurationEntry& duration) override {
        if (duration.getKey() == "runtime") {
            auto runtime = (duration.getEnd() - duration.getStart()).count() / 1000.0;
            base.setRuntime(runtime);
        }
    }
    void visit(souffle::profile::SizeEntry& size) override {
        if (size.getKey() == "num-tuples") {
            base.setNum_tuples(size.getSize());
        }
    }
    void visit(souffle::profile::DirectoryEntry& ruleEntry) override {}

protected:
    T& base;
};
class RecursiveRuleVisitor : public DSNVisitor<Rule> {
public:
    RecursiveRuleVisitor(Rule& rule) : DSNVisitor(rule) {}
};

class RecursiveRulesVisitor : public souffle::profile::Visitor {
public:
    RecursiveRulesVisitor(Relation& relation) : relation(relation) {}
    void visit(souffle::profile::DirectoryEntry& ruleEntry) override {
        auto rule = std::make_shared<Rule>(ruleEntry.getKey(), relation.createID());
        RecursiveRuleVisitor visitor(*rule);
        for (const auto& key : ruleEntry.getKeys()) {
            ruleEntry.readEntry(key)->accept(visitor);
        }
        relation.getRuleMap()[rule->getLocator()] = rule;
    }

private:
    Relation& relation;
};

class NonRecursiveRuleVisitor : public DSNVisitor<Rule> {
public:
    NonRecursiveRuleVisitor(Rule& rule) : DSNVisitor(rule) {}
};

class NonRecursiveRulesVisitor : public souffle::profile::Visitor {
public:
    NonRecursiveRulesVisitor(Relation& relation) : relation(relation) {}
    void visit(souffle::profile::DirectoryEntry& ruleEntry) override {
        auto rule = std::make_shared<Rule>(ruleEntry.getKey(), relation.createID());
        NonRecursiveRuleVisitor visitor(*rule);
        for (const auto& key : ruleEntry.getKeys()) {
            ruleEntry.readEntry(key)->accept(visitor);
        }
        relation.getRuleMap()[rule->getLocator()] = rule;
    }

private:
    Relation& relation;
};

class RelationVisitor : public DSNVisitor<Relation> {
public:
    RelationVisitor(Relation& relation) : DSNVisitor(relation) {}
    void visit(souffle::profile::DurationEntry& duration) override {
        if (duration.getKey() == "runtime") {
            auto runtime = (duration.getEnd() - duration.getStart()).count() / 1000.0;
            base.setRuntime(runtime);
        }
    }
    void visit(souffle::profile::DirectoryEntry& directory) override {
        if (directory.getKey() == "iteration") {
            base.getIterations().push_back(std::make_unique<Iteration>());
            for (const auto& key : directory.getKeys()) {
                auto& iteration = *directory.readDirectoryEntry(key);
                auto runtime = dynamic_cast<souffle::profile::DurationEntry*>(iteration.readEntry("runtime"));
                base.getIterations().back()->setRuntime(
                        (runtime->getEnd() - runtime->getStart()).count() / 1000.0);
                auto copytime =
                        dynamic_cast<souffle::profile::DurationEntry*>(iteration.readEntry("copytime"));
                if (copytime != nullptr) {
                    base.getIterations().back()->setCopy_time(
                            (copytime->getEnd() - copytime->getStart()).count() / 1000.0);
                }
                auto numTuples =
                        dynamic_cast<souffle::profile::SizeEntry*>(iteration.readEntry("num-tuples"));
                base.getIterations().back()->setNum_tuples(numTuples->getSize());
            }
        } else if (directory.getKey() == "non-recursive-rule") {
            NonRecursiveRulesVisitor rulesVisitor(base);
            for (const auto& key : directory.getKeys()) {
                directory.readEntry(key)->accept(rulesVisitor);
            }
        } else if (directory.getKey() == "recursive-rule") {
            // std::vector<std::shared_ptr<Iteration>>& iterations = rel->getIterations();
            // get appropriate iteration (version or iteration? do we have this in old style?)
        }
    }
};
}  // namespace

void Reader::addRelation(const souffle::profile::DirectoryEntry& relation) {
    const std::string& name = relation.getKey();

    relation_map.emplace(name, std::make_shared<Relation>(name, createId()));
    auto& rel = *relation_map[name];
    RelationVisitor relationVisitor(rel);

    for (const auto& key : relation.getKeys()) {
        relation.readEntry(key)->accept(relationVisitor);
    }
}

void Reader::process(const std::vector<std::string>& data) {
    {
        if (data[0].find("recursive") != std::string::npos) {
            // insert into the map if it does not exist already
            if (relation_map.find(data[1]) == relation_map.end()) {
                relation_map.emplace(data[1], std::make_shared<Relation>(Relation(data[1], createId())));
            }
            std::shared_ptr<Relation> _rel = relation_map[data[1]];
            addIteration(_rel, data);
        } else if (data[0].find("frequency") != std::string::npos) {
            if (relation_map.find(data[1]) == relation_map.end()) {
                relation_map.emplace(data[1], std::make_shared<Relation>(Relation(data[1], createId())));
            }
            std::shared_ptr<Relation> _rel = relation_map[data[1]];
            addFrequency(_rel, data);
        }
    }

    run->SetRuntime(this->runtime);
    run->setRelation_map(this->relation_map);
}

void Reader::addIteration(std::shared_ptr<Relation> rel, std::vector<std::string> data) {
    bool ready = rel->isReady();
    std::vector<std::shared_ptr<Iteration>>& iterations = rel->getIterations();

    // add an iteration if we require one
    if (ready || iterations.empty()) {
        iterations.push_back(std::make_shared<Iteration>(Iteration()));
        rel->setReady(false);
    }

    std::shared_ptr<Iteration>& iter = iterations.back();

    if (data[0].find("rule") != std::string::npos) {
        std::string temp = rel->createRecID(data[4]);
        iter->addRule(data, temp);
    } else if (data[0].at(0) == 't' && data[0].find("relation") != std::string::npos) {
        iter->setRuntime(std::stod(data[5]));
        iter->setLocator(data[2]);
        rel->setLocator(data[2]);
    } else if (data[0].at(0) == 'n' && data[0].find("relation") != std::string::npos) {
        iter->setNum_tuples(std::stol(data[3]));
    } else if (data[0].at(0) == 'c' && data[0].find("relation") != std::string::npos) {
        iter->setCopy_time(std::stod(data[5]));
        rel->setReady(true);
    }
}

void Reader::addFrequency(std::shared_ptr<Relation> rel, std::vector<std::string> data) {
    std::unordered_map<std::string, std::shared_ptr<Rule>>& ruleMap = rel->getRuleMap();

    // If we can't find the rule then it must be an Iteration
    if (ruleMap.find(data[3]) == ruleMap.end()) {
        for (auto& iter : rel->getIterations()) {
            for (auto& rule : iter->getRul_rec()) {
                if (rule.second->getVersion() == std::stoi(data[2])) {
                    rule.second->addAtomFrequency(data[4], data[5], std::stoi(data[2]), std::stoi(data[7]));
                    return;
                }
            }
        }
    } else {
        std::shared_ptr<Rule> _rul = ruleMap[data[3]];
        // @frequency-rule;relationName;version;srcloc;stringify(clause);stringify(atom);level;count
        // Generate name from atom + version
        _rul->addAtomFrequency(data[4], data[5], std::stoi(data[2]), std::stoi(data[7]));
    }
}

std::string Reader::createId() {
    return "R" + std::to_string(++rel_id);
}
