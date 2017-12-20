/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RamTable.h
 *
 * Defines classes Table, TupleBlock, Index, and HashBlock for implementing
 * an ram relational database. A table consists of a linked list of
 * tuple blocks that contain tuples of the table. An index is a hash-index
 * whose hash table is stored in Index. The entry of a hash table entry
 * refer to HashBlocks that are blocks of pointers that point to tuples
 * in tuple blocks with the same hash.
 *
 ***********************************************************************/

#pragma once

#include "IODirectives.h"
#include "ParallelUtils.h"
#include "RamIndex.h"
#include "RamTypes.h"
#include "SymbolMask.h"
#include "SymbolTable.h"
#include "Table.h"
#include "Util.h"

#include <list>
#include <map>
#include <string>
#include <vector>

namespace souffle {

class RamRelation {
    std::string name;
    unsigned arity;
    std::vector<std::string> attributeNames;
    std::vector<std::string> attributeTypeQualifiers;
    SymbolMask mask;
    bool input;
    bool computed;
    bool output;
    bool btree;
    bool brie;
    bool eqrel;

    bool isdata;
    bool istemp;
    IODirectives inputDirectives;
    std::vector<IODirectives> outputDirectives;

public:
    RamRelation()
            : arity(0), mask(arity), input(false), computed(false), output(false), btree(false), brie(false),
              eqrel(false), isdata(false), istemp(false) {}

    RamRelation(const std::string& name, unsigned arity, const bool istemp)
            : RamRelation(name, arity) {
        this->istemp = istemp;
    }

    RamRelation(const std::string& name, unsigned arity,
            std::vector<std::string> attributeNames = {},
            std::vector<std::string> attributeTypeQualifiers = {}, const SymbolMask& mask = SymbolMask(0),
            const bool input = false, const bool computed = false, const bool output = false,
            const bool btree = false, const bool brie = false, const bool eqrel = false,
            const bool isdata = false, const IODirectives inputDirectives = IODirectives(),
            const std::vector<IODirectives> outputDirectives = {}, const bool istemp = false)
            : name(name), arity(arity), attributeNames(attributeNames),
              attributeTypeQualifiers(attributeTypeQualifiers), mask(mask), input(input), computed(computed),
              output(output), btree(btree), brie(brie), eqrel(eqrel), isdata(isdata), istemp(istemp),
              inputDirectives(inputDirectives), outputDirectives(outputDirectives) {
        assert(this->attributeNames.size() == arity || this->attributeNames.empty());
        assert(this->attributeTypeQualifiers.size() == arity || this->attributeTypeQualifiers.empty());
    }

    const std::string& getName() const {
        return name;
    }

    const std::string getArg(uint32_t i) const {
        if (!attributeNames.empty()) {
            return attributeNames[i];
        }
        if (arity == 0) {
            return "";
        }
        return "c" + std::to_string(i);
    }

    const std::string getArgTypeQualifier(uint32_t i) const {
        return (i < attributeTypeQualifiers.size()) ? attributeTypeQualifiers[i] : "";
    }

    const SymbolMask& getSymbolMask() const {
        return mask;
    }

    const bool isInput() const {
        return input;
    }

    const bool isComputed() const {
        return computed;
    }

    const bool isOutput() const {
        return output;
    }

    const bool isBTree() const {
        return btree;
    }

    const bool isBrie() const {
        return brie;
    }

    const bool isEqRel() const {
        return eqrel;
    }

    const bool isTemp() const {
        return istemp;
    }

    const bool isData() const {
        return isdata;
    }

    unsigned getArity() const {
        return arity;
    }

    const IODirectives& getInputDirectives() const {
        return inputDirectives;
    }

    const std::vector<IODirectives>& getOutputDirectives() const {
        return outputDirectives;
    }

    bool operator==(const RamRelation& other) const {
        return name == other.name;
    }

    bool operator!=(const RamRelation& other) const {
        return name != other.name;
    }

    bool operator<(const RamRelation& other) const {
        return name < other.name;
    }

    void print(std::ostream& out) const {
        out << name << "(";
        out << getArg(0);
        for (unsigned i = 1; i < arity; i++) {
            out << ",";
            out << getArg(i);
        }
        out << ")";
    }

    friend std::ostream& operator<<(std::ostream& out, const RamRelation& rel) {
        rel.print(out);
        return out;
    }
};


}  // end of namespace souffle
