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

namespace souffle {
namespace profile {

void Reader::processFile() {
    auto programDuration = dynamic_cast<DurationEntry*>(db.lookupEntry({"program", "runtime"}));
    if (programDuration == nullptr) {
        std::cout << "db is empty!" << std::endl;
        db.print(std::cout);
        return;
    }
    runtime = (programDuration->getEnd() - programDuration->getStart()).count() / 1000.0;

    for (const auto& cur :
            dynamic_cast<DirectoryEntry*>(db.lookupEntry({"program", "relation"}))->getKeys()) {
        addRelation(*dynamic_cast<DirectoryEntry*>(db.lookupEntry({"program", "relation", cur})));
    }
    run->SetRuntime(this->runtime);
    run->setRelation_map(this->relation_map);
    loaded = true;
}

namespace {
template <typename T>
class DSNVisitor : public Visitor {
public:
    DSNVisitor(T& base) : base(base) {}
    void visit(TextEntry& text) override {
        if (text.getKey() == "source-locator") {
            base.setLocator(text.getText());
        }
    }
    void visit(DurationEntry& duration) override {
        if (duration.getKey() == "runtime") {
            auto runtime = (duration.getEnd() - duration.getStart()).count() / 1000.0;
            base.setRuntime(runtime);
        }
    }
    void visit(SizeEntry& size) override {
        if (size.getKey() == "num-tuples") {
            base.setNum_tuples(size.getSize());
        }
    }
    void visit(DirectoryEntry& ruleEntry) override {}

protected:
    T& base;
};

/**
 * Visit ProfileDB recursive rule.
 * ruleversion: {DSN}
 */
class RecursiveRuleVisitor : public DSNVisitor<Rule> {
public:
    RecursiveRuleVisitor(Rule& rule) : DSNVisitor(rule) {}
};

/**
 * Visit ProfileDB non-recursive rules.
 * rule: {versionNum : {DSN}, versionNum+1: {DSN}}
 */
class RecursiveRulesVisitor : public Visitor {
public:
    RecursiveRulesVisitor(Iteration& iteration, Relation& relation)
            : iteration(iteration), relation(relation) {}
    void visit(DirectoryEntry& ruleEntry) override {
        for (const auto& key : ruleEntry.getKeys()) {
            auto& versions = *ruleEntry.readDirectoryEntry(key);
            auto rule = std::make_shared<Rule>(
                    ruleEntry.getKey(), std::stoi(key), relation.createRecID(ruleEntry.getKey()));
            RecursiveRuleVisitor visitor(*rule);
            for (const auto& versionKey : versions.getKeys()) {
                versions.readEntry(versionKey)->accept(visitor);
            }
            // To match map keys defined in Iteration::addRule()
            std::string ruleKey = key + rule->getLocator() + key;
            iteration.addRule(ruleKey, rule);
        }
    }

protected:
    Iteration& iteration;
    Relation& relation;
};

/**
 * Visit ProfileDB non-recursive rule.
 * rule: {DSN}
 */
class NonRecursiveRuleVisitor : public DSNVisitor<Rule> {
public:
    NonRecursiveRuleVisitor(Rule& rule) : DSNVisitor(rule) {}
};

/**
 * Visit ProfileDB non-recursive rules.
 * non-recursive-rule: {rule1: {DSN}, ...}
 */
class NonRecursiveRulesVisitor : public Visitor {
public:
    NonRecursiveRulesVisitor(Relation& relation) : relation(relation) {}
    void visit(DirectoryEntry& ruleEntry) override {
        auto rule = std::make_shared<Rule>(ruleEntry.getKey(), relation.createID());
        NonRecursiveRuleVisitor visitor(*rule);
        for (const auto& key : ruleEntry.getKeys()) {
            ruleEntry.readEntry(key)->accept(visitor);
        }
        relation.getRuleMap()[rule->getLocator()] = rule;
    }

protected:
    Relation& relation;
};

/**
 * Visit a ProfileDB relation iteration.
 * iterationNumber: {DSN, recursive-rule: {}}
 */
class IterationVisitor : public DSNVisitor<Iteration> {
public:
    IterationVisitor(Iteration& iteration, Relation& relation) : DSNVisitor(iteration), relation(relation) {}
    void visit(DurationEntry& duration) override {
        if (duration.getKey() == "runtime") {
            auto runtime = (duration.getEnd() - duration.getStart()).count() / 1000.0;
            base.setRuntime(runtime);
        } else if (duration.getKey() == "copytime") {
            auto copytime = (duration.getEnd() - duration.getStart()).count() / 1000.0;
            base.setCopy_time(copytime);
        }
    }
    void visit(DirectoryEntry& directory) override {
        if (directory.getKey() == "recursive-rule") {
            RecursiveRulesVisitor rulesVisitor(base, relation);
            for (const auto& key : directory.getKeys()) {
                directory.readEntry(key)->accept(rulesVisitor);
            }
        } else {
            std::cerr << "Unexpected entry: " << std::endl;
            directory.print(std::cerr, 0);
        }
    }

protected:
    Relation& relation;
};

/**
 * Visit ProfileDB iterations.
 * iteration: {num: {}, num2: {}, ...}
 */
class IterationsVisitor : public Visitor {
public:
    IterationsVisitor(Relation& relation) : relation(relation) {}
    void visit(DirectoryEntry& ruleEntry) override {
        auto iteration = std::make_shared<Iteration>();
        IterationVisitor visitor(*iteration, relation);
        for (const auto& key : ruleEntry.getKeys()) {
            ruleEntry.readEntry(key)->accept(visitor);
        }
        relation.getIterations().push_back(iteration);
    }

protected:
    Relation& relation;
};

/**
 * Visit ProfileDB relations.
 * relname: {DSN, non-recursive-rule: {}, iteration: {...}}
 */
class RelationVisitor : public DSNVisitor<Relation> {
public:
    RelationVisitor(Relation& relation) : DSNVisitor(relation) {}
    void visit(DurationEntry& duration) override {
        if (duration.getKey() == "runtime") {
            auto runtime = (duration.getEnd() - duration.getStart()).count() / 1000.0;
            base.setRuntime(runtime);
        }
    }
    void visit(DirectoryEntry& directory) override {
        if (directory.getKey() == "iteration") {
            IterationsVisitor iterationsVisitor(base);
            for (const auto& key : directory.getKeys()) {
                directory.readEntry(key)->accept(iterationsVisitor);
            }
        } else if (directory.getKey() == "non-recursive-rule") {
            NonRecursiveRulesVisitor rulesVisitor(base);
            for (const auto& key : directory.getKeys()) {
                directory.readEntry(key)->accept(rulesVisitor);
            }
        }
    }
};
}  // namespace

void Reader::addRelation(const DirectoryEntry& relation) {
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

}  // namespace profile
}  // namespace souffle
