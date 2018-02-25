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
    /** Ram Translation Unit */
    RamTranslationUnit& translationUnit;

    /** The type utilized for storing relations */
    typedef std::map<std::string, InterpreterRelation*> relation_map;

    /** The relations manipulated by a ram program */
    relation_map data;

    /** The increment counter utilized by some RAM language constructs */
    int counter;

protected:
    /** Evaluate value */
    RamDomain eval(const RamValue& value, const InterpreterContext& ctxt = InterpreterContext());

    /** Evaluate operation */
    void eval(const RamOperation& op, const InterpreterContext& args = InterpreterContext());

    /** Evaluate conditions */
    bool eval(const RamCondition& cond, const InterpreterContext& ctxt = InterpreterContext());

    /** Evaluate statement */
    void eval(const RamStatement& stmt, std::ostream* profile = nullptr);

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
        auto pos = data.find(id.getName());
        InterpreterRelation* res = nullptr;
        assert(pos == data.end());
        if (!id.isEqRel()) {
            res = new InterpreterRelation(id.getArity());
        } else {
            res = new InterpreterEqRelation(id.getArity());
        }
        data[id.getName()] = res;
    }

    /** Get relation */
    InterpreterRelation& getRelation(const std::string& name) {
        // look up relation
        auto pos = data.find(name);
        assert(pos != data.end());
        return *pos->second;
    }

    /** Get relation */
    inline InterpreterRelation& getRelation(const RamRelation& id) {
        return getRelation(id.getName());
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
        InterpreterRelation &rel = getRelation(id); 
        data.erase(id.getName());
        delete &rel;
    }

public:
    Interpreter(RamTranslationUnit& tUnit) : translationUnit(tUnit), counter(0) {}
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
