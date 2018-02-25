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

#include "InterpreterRelation.h"
#include "InterpreterContext.h"
#include "RamTranslationUnit.h"
#include "SymbolTable.h"

#include <ostream>
#include <vector>
#include <map>
#include <memory>

namespace souffle {

class InterpreterProgInterface;

/**
 * Interpreter executing a RAM translation unit
 */

class Interpreter {
    friend InterpreterProgInterface;

private:
    /** Ram Translation Unit */
    RamTranslationUnit& translationUnit;

    /** The type utilized for storing relations */
    typedef std::map<std::string, InterpreterRelation*> relation_map;

    /** The relations manipulated by a ram program */
    relation_map data;

    /** The increment counter utilized by some RAM language constructs */
    std::atomic<int> counter;

protected:
    /** Evaluate value */
    RamDomain eval(const RamValue& value, const InterpreterContext& ctxt = InterpreterContext());

    /** Evaluate operation */
    void eval(const RamOperation& op, const InterpreterContext& args = InterpreterContext());

    /** Evaluate conditions */
    bool eval(const RamCondition& cond, const InterpreterContext& ctxt = InterpreterContext());

    /** Evaluate statement */
    void eval(const RamStatement& stmt, std::ostream* profile = nullptr);

    /** Get symbol table
     */
    SymbolTable& getSymbolTable() {
        return translationUnit.getSymbolTable();
    }

    /**
     * Obtains the current value of the internal counter.
     */
    int getCounter() const {
        return counter;
    }

    /**
     * Increments the internal counter and obtains the
     * old value.
     */
    int incCounter() {
        return counter++;
    }

    /** Get relation */
    InterpreterRelation& getRelation(const RamRelation& id) {
        InterpreterRelation* res = nullptr;
        auto pos = data.find(id.getName());
        if (pos != data.end()) {
            res = (pos->second);
        } else {
            if (!id.isEqRel()) {
                res = new InterpreterRelation(id.getArity());
            } else {
                res = new InterpreterEqRelation(id.getArity());
            }
            data[id.getName()] = res;
        }
        // return result
        return *res;
    }

    /** Get relation  */
    const InterpreterRelation& getRelation(const RamRelation& id) const {
        // look up relation
        auto pos = data.find(id.getName());
        assert(pos != data.end());

        // cache result
        return *pos->second;
    }

    /** Get relation */
    const InterpreterRelation& getRelation(const std::string& name) const {
        auto pos = data.find(name);
        assert(pos != data.end());
        return *pos->second;
    }

    /** Get relation map */
    relation_map& getRelationMap() const {
        return const_cast<relation_map&>(data);
    }

    /** Check for relation */
    bool hasRelation(const std::string& name) const {
        return data.find(name) != data.end();
    }

    /** Drop relation */
    void dropRelation(const RamRelation& id) {
        data.erase(id.getName());
    }

public:
    Interpreter(RamTranslationUnit& tUnit) : translationUnit(tUnit)  {}
    virtual ~Interpreter() { 
        for (auto& x : data) {
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
