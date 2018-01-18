/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file InterpreterInterface.cpp
 *
 * Defines classes that implement the SouffleInterface abstract class
 *
 ***********************************************************************/

#include "InterpreterInterface.h"

namespace souffle {

/**
 * Helper function to convert a tuple to a RamDomain pointer
 */
RamDomain* convertTupleToNums(const tuple& t) {
    RamDomain* newTuple = new RamDomain[t.size()];

    for (size_t i = 0; i < t.size(); i++) {
        newTuple[i] = t[i];
    }

    return newTuple;
}

void InterpreterRelInterface::insert(const tuple& t) {
    relation.insert(convertTupleToNums(t));
}

bool InterpreterRelInterface::contains(const tuple& t) const {
    return relation.exists(convertTupleToNums(t));
}

typename InterpreterRelInterface::iterator InterpreterRelInterface::begin() const {
    return InterpreterRelInterface::iterator(
            new InterpreterRelInterface::iterator_base(id, this, relation.begin()));
}

typename InterpreterRelInterface::iterator InterpreterRelInterface::end() const {
    return InterpreterRelInterface::iterator(
            new InterpreterRelInterface::iterator_base(id, this, relation.end()));
}

const char* InterpreterRelInterface::getAttrType(size_t idx) const {
    assert(0 <= idx && idx < getArity() && "exceeded tuple size");
    return types[idx].c_str();
}

const char* InterpreterRelInterface::getAttrName(size_t idx) const {
    assert(0 <= idx && idx < getArity() && "exceeded tuple size");
    return attrNames[idx].c_str();
}

size_t InterpreterRelInterface::getArity() const {
    return relation.getArity();
}

SymbolTable& InterpreterRelInterface::getSymbolTable() const {
    return symTable;
}

}  // end of namespace souffle
