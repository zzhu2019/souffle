/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Interpreter.h
 *
 * Declares the interpreter class for executing RAM programs.
 *
 ***********************************************************************/

#pragma once

#include "InterpreterContext.h"
#include "InterpreterRelation.h"
#include "RamTranslationUnit.h"
#include "SymbolTable.h"

#include <map>
#include <memory>
#include <ostream>
#include <vector>

namespace souffle {

class InterpreterProgInterface;

/**
 * Interpreter executing a RAM translation unit
 */

class Interpreter {
    friend InterpreterProgInterface;

private:
    /** RAM translation Unit */
    RamTranslationUnit& translationUnit;

    /** relation environment type */
    using relation_map = std::map<std::string, InterpreterRelation*>;

    /** relation environment */
    relation_map environment;

    /** counter for $ operator */
    int counter;

protected:
    /** Evaluate value */
    RamDomain evalVal(const RamValue& value, const InterpreterContext& ctxt = InterpreterContext());

    /** Evaluate operation */
    void evalOp(const RamOperation& op, const InterpreterContext& args = InterpreterContext());

    /** Evaluate conditions */
    bool evalCond(const RamCondition& cond, const InterpreterContext& ctxt = InterpreterContext());

    /** Evaluate statement */
    void evalStmt(const RamStatement& stmt);

    /** Get symbol table */
    SymbolTable& getSymbolTable() {
        return translationUnit.getSymbolTable();
    }

    /** Get counter */
    int getCounter() const {
        return counter;
    }

    /** Increment counter */
    int incCounter() {
        return counter++;
    }

    /** Create relation */
    void createRelation(const RamRelation& id) {
        auto pos = environment.find(id.getName());
        InterpreterRelation* res = nullptr;
        assert(pos == environment.end());
        if (!id.isEqRel()) {
            res = new InterpreterRelation(id.getArity());
        } else {
            res = new InterpreterEqRelation(id.getArity());
        }
        environment[id.getName()] = res;
    }

    /** Get relation */
    InterpreterRelation& getRelation(const std::string& name) {
        // look up relation
        auto pos = environment.find(name);
        assert(pos != environment.end());
        return *pos->second;
    }

    /** Get relation */
    inline InterpreterRelation& getRelation(const RamRelation& id) {
        return getRelation(id.getName());
    }

    /** Get relation map */
    relation_map& getRelationMap() const {
        return const_cast<relation_map&>(environment);
    }

    /** Drop relation */
    void dropRelation(const RamRelation& id) {
        InterpreterRelation& rel = getRelation(id);
        environment.erase(id.getName());
        delete &rel;
    }

    /** Swap relation */
    void swapRelation(const RamRelation& ramRel1, const RamRelation& ramRel2) {
        InterpreterRelation* rel1 = &getRelation(ramRel1);
        InterpreterRelation* rel2 = &getRelation(ramRel2);
        environment[ramRel1.getName()] = rel2;
        environment[ramRel2.getName()] = rel1;
    }

public:
    Interpreter(RamTranslationUnit& tUnit) : translationUnit(tUnit), counter(0) {}
    virtual ~Interpreter() {
        for (auto& x : environment) {
            delete x.second;
        }
    }

    /** Get translation unit */
    RamTranslationUnit& getTranslationUnit() {
        return translationUnit;
    }

    /** Execute main program */
    void executeMain();

    /* Execute subroutine */
    void executeSubroutine(const RamStatement& stmt, const std::vector<RamDomain>& arguments,
            std::vector<RamDomain>& returnValues, std::vector<bool>& returnErrors);
};

}  // end of namespace souffle
