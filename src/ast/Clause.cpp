/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Clause.cpp
 *
 * Implements methods of class Clause that represents rules including facts,
 * predicates, and queries in a Datalog program.
 *
 ***********************************************************************/

#include "Clause.h"
#include "Literal.h"
#include "Program.h"
#include "Relation.h"
#include "Visitor.h"

#include <set>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <string.h>

namespace souffle {
namespace ast {

/*
 * Clause
 */

/* Add literal to body */
void Clause::addToBody(std::unique_ptr<Literal> l) {
    if (dynamic_cast<Atom*>(l.get())) {
        atoms.push_back(std::unique_ptr<Atom>(static_cast<Atom*>(l.release())));
    } else if (dynamic_cast<Negation*>(l.get())) {
        negations.push_back(std::unique_ptr<Negation>(static_cast<Negation*>(l.release())));
    } else if (dynamic_cast<Constraint*>(l.get())) {
        constraints.push_back(std::unique_ptr<Constraint>(static_cast<Constraint*>(l.release())));
    } else {
        assert(false && "Unsupported literal type!");
    }
}

/* Set the head of clause to h */
void Clause::setHead(std::unique_ptr<Atom> h) {
    ASSERT(!head && "Head is already set");
    head = std::move(h);
}

Literal* Clause::getBodyLiteral(size_t idx) const {
    if (idx < atoms.size()) {
        return atoms[idx].get();
    }
    idx -= atoms.size();
    if (idx < negations.size()) {
        return negations[idx].get();
    }
    idx -= negations.size();
    return constraints[idx].get();
}

std::vector<Literal*> Clause::getBodyLiterals() const {
    std::vector<Literal*> res;
    for (auto& cur : atoms) {
        res.push_back(cur.get());
    }
    for (auto& cur : negations) {
        res.push_back(cur.get());
    }
    for (auto& cur : constraints) {
        res.push_back(cur.get());
    }
    return res;
}

bool Clause::isFact() const {
    // there must be a head
    if (head == nullptr) {
        return false;
    }
    // there must not be any body clauses
    if (getBodySize() != 0) {
        return false;
    }
    // and there are no aggregates
    bool hasAggregates = false;
    visitDepthFirst(*head, [&](const Aggregator& cur) { hasAggregates = true; });
    return !hasAggregates;
}

void Clause::print(std::ostream& os) const {
    if (head != nullptr) {
        head->print(os);
    }
    if (getBodySize() > 0) {
        os << " :- \n   ";
        os << join(getBodyLiterals(), ",\n   ", print_deref<Literal*>());
    }
    os << ".";
    if (getExecutionPlan()) {
        getExecutionPlan()->print(os);
    }
}

void Clause::reorderAtoms(const std::vector<unsigned int>& newOrder) {
    // Validate given order
    assert(newOrder.size() == atoms.size());
    std::vector<unsigned int> nopOrder;
    for (unsigned int i = 0; i < atoms.size(); i++) {
        nopOrder.push_back(i);
    }
    assert(std::is_permutation(nopOrder.begin(), nopOrder.end(), newOrder.begin()));

    // Reorder atoms
    std::vector<std::unique_ptr<Atom>> oldAtoms(atoms.size());
    atoms.swap(oldAtoms);
    for (unsigned int i = 0; i < newOrder.size(); i++) {
        atoms[i].swap(oldAtoms[newOrder[i]]);
    }
}

}  // namespace ast
}  // namespace souffle
