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

void InterpreterInterface::iterator_base::operator++() {
    ++it;
}

tuple& InterpreterInterface::iterator_base::operator*() {
    tup.rewind();

    // construct the tuple to return
    for (size_t i = 0; i < ramRelationInterface->getArity(); i++) {
        if (*(ramRelationInterface->getAttrType(i)) == 's') {
            std::string s = ramRelationInterface->getSymbolTable().resolve((*it)[i]);
            tup << s;
        } else {
            tup << (*it)[i];
        }
    }

    tup.rewind();

    return tup;
}

InterpreterInterface::iterator_base* InterpreterInterface::iterator_base::clone() const {
    return new InterpreterInterface::iterator_base(getId(), ramRelationInterface, it);
}

bool InterpreterInterface::iterator_base::equal(const Relation::iterator_base& o) const {
    try {
        auto iter = dynamic_cast<const InterpreterInterface::iterator_base&>(o);
        return ramRelationInterface == iter.ramRelationInterface && it == iter.it;
    } catch (const std::bad_cast& e) {
        return false;
    }
}

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

void InterpreterInterface::insert(const tuple& t) {
    ramRelation.insert(convertTupleToNums(t));
}

bool InterpreterInterface::contains(const tuple& t) const {
    return ramRelation.exists(convertTupleToNums(t));
}

typename InterpreterInterface::iterator InterpreterInterface::begin() const {
    return InterpreterInterface::iterator(
            new InterpreterInterface::iterator_base(id, this, ramRelation.begin()));
}

typename InterpreterInterface::iterator InterpreterInterface::end() const {
    return InterpreterInterface::iterator(
            new InterpreterInterface::iterator_base(id, this, ramRelation.end()));
}

std::size_t InterpreterInterface::size() {
    return ramRelation.size();
}

bool InterpreterInterface::isOutput() const {
    // TODO
    return true;
}

bool InterpreterInterface::isInput() const {
    // TODO
    return true;
}

std::string InterpreterInterface::getName() const {
    return name;
}

const char* InterpreterInterface::getAttrType(size_t idx) const {
    assert(0 <= idx && idx < getArity() && "exceeded tuple size");
    return types[idx].c_str();
}

const char* InterpreterInterface::getAttrName(size_t idx) const {
    assert(0 <= idx && idx < getArity() && "exceeded tuple size");
    return attrNames[idx].c_str();
}

size_t InterpreterInterface::getArity() const {
    return ramRelation.getArity();
}

SymbolTable& InterpreterInterface::getSymbolTable() const {
    return symTable;
}

}  // end of namespace souffle
