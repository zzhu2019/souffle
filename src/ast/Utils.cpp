/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Utils.cpp
 *
 * A collection of utilities operating on AST constructs.
 *
 ***********************************************************************/

#include "Utils.h"
#include "Constraints.h"
#include "TypeSystem.h"
#include "Util.h"
#include "Visitor.h"

namespace souffle {
namespace ast {

std::vector<const Variable*> getVariables(const Node& root) {
    // simply collect the list of all variables by visiting all variables
    std::vector<const Variable*> vars;
    visitDepthFirst(root, [&](const Variable& var) { vars.push_back(&var); });
    return vars;
}

std::vector<const Variable*> getVariables(const Node* root) {
    return getVariables(*root);
}

const Relation* getAtomRelation(const Atom* atom, const Program* program) {
    return program->getRelation(atom->getName());
}

const Relation* getHeadRelation(const Clause* clause, const Program* program) {
    return getAtomRelation(clause->getHead(), program);
}

std::set<const Relation*> getBodyRelations(const Clause* clause, const Program* program) {
    std::set<const Relation*> bodyRelations;
    for (const auto& lit : clause->getBodyLiterals()) {
        visitDepthFirst(
                *lit, [&](const Atom& atom) { bodyRelations.insert(getAtomRelation(&atom, program)); });
    }
    for (const auto& arg : clause->getHead()->getArguments()) {
        visitDepthFirst(
                *arg, [&](const Atom& atom) { bodyRelations.insert(getAtomRelation(&atom, program)); });
    }
    return bodyRelations;
}

bool hasClauseWithNegatedRelation(const Relation* relation, const Relation* negRelation,
        const Program* program, const Literal*& foundLiteral) {
    for (const Clause* cl : relation->getClauses()) {
        for (const Negation* neg : cl->getNegations()) {
            if (negRelation == getAtomRelation(neg->getAtom(), program)) {
                foundLiteral = neg;
                return true;
            }
        }
    }
    return false;
}

bool hasClauseWithAggregatedRelation(const Relation* relation, const Relation* aggRelation,
        const Program* program, const Literal*& foundLiteral) {
    for (const Clause* cl : relation->getClauses()) {
        bool hasAgg = false;
        visitDepthFirst(*cl, [&](const Aggregator& cur) {
            visitDepthFirst(cur, [&](const Atom& atom) {
                if (aggRelation == getAtomRelation(&atom, program)) {
                    foundLiteral = &atom;
                    hasAgg = true;
                }
            });
        });
        if (hasAgg) {
            return true;
        }
    }
    return false;
}

}  // namespace ast
}  // namespace souffle
