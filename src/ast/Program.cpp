/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Program.h
 *
 * Implement methods for class Program that represents a Datalog
 * program consisting of types, relations, and clauses.
 *
 ***********************************************************************/

#include "Program.h"
#include "Clause.h"
#include "Component.h"
#include "ErrorReport.h"
#include "GraphUtils.h"
#include "Relation.h"
#include "TypeAnalysis.h"
#include "Util.h"
#include "Utils.h"
#include "Visitor.h"

#include <list>
#include <sstream>

#include <stdarg.h>
#include <stdlib.h>

namespace souffle {
namespace ast {

/*
 * Program
 */

Program::Program(Program&& other) noexcept {
    types.swap(other.types);
    relations.swap(other.relations);
    clauses.swap(other.clauses);
    ioDirectives.swap(other.ioDirectives);
    components.swap(other.components);
    instantiations.swap(other.instantiations);
}

/* Add a new type to the program */
void Program::addType(std::unique_ptr<Type> type) {
    auto& cur = types[type->getName()];
    assert(!cur && "Redefinition of type!");
    cur = std::move(type);
}

const Type* Program::getType(const TypeIdentifier& name) const {
    auto pos = types.find(name);
    return (pos == types.end()) ? nullptr : pos->second.get();
}

std::vector<const Type*> Program::getTypes() const {
    std::vector<const Type*> res;
    for (const auto& cur : types) {
        res.push_back(cur.second.get());
    }
    return res;
}

/* Add a relation to the program */
void Program::addRelation(std::unique_ptr<Relation> r) {
    const auto& name = r->getName();
    assert(relations.find(name) == relations.end() && "Redefinition of relation!");
    relations[name] = std::move(r);
}

void Program::appendRelation(std::unique_ptr<Relation> r) {
    // get relation
    std::unique_ptr<Relation>& rel = relations[r->getName()];
    assert(!rel && "Adding pre-existing relation!");

    // add relation
    rel = std::move(r);
}

/* Remove a relation from the program */
void Program::removeRelation(const RelationIdentifier& name) {
    /* Remove relation from map */
    relations.erase(relations.find(name));
}

void Program::appendClause(std::unique_ptr<Clause> clause) {
    // get relation
    std::unique_ptr<Relation>& r = relations[clause->getHead()->getName()];
    assert(r && "Trying to append to unknown relation!");

    // delegate call
    r->addClause(std::move(clause));
}

void Program::removeClause(const Clause* clause) {
    // get relation
    auto pos = relations.find(clause->getHead()->getName());
    if (pos == relations.end()) {
        return;
    }

    // delegate call
    pos->second->removeClause(clause);
}

Relation* Program::getRelation(const RelationIdentifier& name) const {
    auto pos = relations.find(name);
    return (pos == relations.end()) ? nullptr : pos->second.get();
}

/* Add a clause to the program */
void Program::addClause(std::unique_ptr<Clause> clause) {
    ASSERT(clause && "NULL clause");
    clauses.push_back(std::move(clause));
}

/* Add a clause to the program */
void Program::addIODirective(std::unique_ptr<IODirective> directive) {
    ASSERT(directive && "NULL IO directive");
    ioDirectives.push_back(std::move(directive));
}

/* Add a pragma to the program */
void Program::addPragma(std::unique_ptr<Pragma> pragma) {
    ASSERT(pragma && "NULL IO directive");
    pragmaDirectives.push_back(std::move(pragma));
}

/* Put all pragma directives of the program into a list */
const std::vector<std::unique_ptr<Pragma>>& Program::getPragmaDirectives() const {
    return pragmaDirectives;
}

/* Put all relations of the program into a list */
std::vector<Relation*> Program::getRelations() const {
    std::vector<Relation*> res;
    for (const auto& rel : relations) {
        res.push_back(rel.second.get());
    }
    return res;
}
/* Put all io directives of the program into a list */
const std::vector<std::unique_ptr<IODirective>>& Program::getIODirectives() const {
    return ioDirectives;
}

/* Print program in textual format */
void Program::print(std::ostream& os) const {
    /* Print types */
    os << "// ----- Types -----\n";
    for (const auto& cur : types) {
        os << *cur.second << "\n";
    }

    /* Print components */
    if (!components.empty()) {
        os << "\n// ----- Components -----\n";
        for (const auto& cur : components) {
            os << *cur << "\n";
        }
    }

    /* Print instantiations */
    if (!instantiations.empty()) {
        os << "\n";
        for (const auto& cur : instantiations) {
            os << *cur << "\n";
        }
    }

    /* Print relations */
    os << "\n// ----- Relations -----\n";
    for (const auto& cur : relations) {
        const std::unique_ptr<Relation>& rel = cur.second;
        os << "\n\n// -- " << rel->getName() << " --\n";
        os << *rel << "\n\n";
        for (const auto clause : rel->getClauses()) {
            os << *clause << "\n\n";
        }
        for (const auto ioDirective : rel->getIODirectives()) {
            os << *ioDirective << "\n\n";
        }
    }

    if (!clauses.empty()) {
        os << "\n// ----- Orphan Clauses -----\n";
        os << join(clauses, "\n\n", print_deref<std::unique_ptr<Clause>>()) << "\n";
    }

    if (!ioDirectives.empty()) {
        os << "\n// ----- Orphan IO directives -----\n";
        os << join(ioDirectives, "\n\n", print_deref<std::unique_ptr<IODirective>>()) << "\n";
    }

    if (!pragmaDirectives.empty()) {
        os << "\n// ----- Pragma -----\n";
        for (const auto& cur : pragmaDirectives) {
            os << *cur << "\n";
        }
    }
}

Program* Program::clone() const {
    // create copy
    auto res = new Program();

    // move types
    for (const auto& cur : types) {
        res->types.insert(std::make_pair(cur.first, std::unique_ptr<Type>(cur.second->clone())));
    }

    // move relations
    for (const auto& cur : relations) {
        res->relations.insert(std::make_pair(cur.first, std::unique_ptr<Relation>(cur.second->clone())));
    }

    // move components
    for (const auto& cur : components) {
        res->components.push_back(std::unique_ptr<Component>(cur->clone()));
    }

    // move component instantiations
    for (const auto& cur : instantiations) {
        res->instantiations.push_back(std::unique_ptr<ComponentInit>(cur->clone()));
    }

    // move ioDirectives
    for (const auto& cur : ioDirectives) {
        res->ioDirectives.push_back(std::unique_ptr<IODirective>(cur->clone()));
    }

    ErrorReport errors;

    res->finishParsing();

    // done
    return res;
}

/** Mutates this node */
void Program::apply(const NodeMapper& map) {
    for (auto& cur : types) {
        cur.second = map(std::move(cur.second));
    }
    for (auto& cur : relations) {
        cur.second = map(std::move(cur.second));
    }
    for (auto& cur : components) {
        cur = map(std::move(cur));
    }
    for (auto& cur : instantiations) {
        cur = map(std::move(cur));
    }
    for (auto& cur : pragmaDirectives) {
        cur = map(std::move(cur));
    }
    for (auto& cur : ioDirectives) {
        cur = map(std::move(cur));
    }
}

void Program::finishParsing() {
    // unbound clauses with no relation defined
    std::vector<std::unique_ptr<Clause>> unbound;

    // add clauses
    for (auto& cur : clauses) {
        auto pos = relations.find(cur->getHead()->getName());
        if (pos != relations.end()) {
            pos->second->addClause(std::move(cur));
        } else {
            unbound.push_back(std::move(cur));
        }
    }
    // remember the remaining orphan clauses
    clauses.clear();
    clauses.swap(unbound);

    // unbound directives with no relation defined
    std::vector<std::unique_ptr<IODirective>> unboundDirectives;

    // add IO directives
    for (auto& cur : ioDirectives) {
        auto pos = relations.find(cur->getName());
        if (pos != relations.end()) {
            pos->second->addIODirectives(std::move(cur));
        } else {
            unboundDirectives.push_back(std::move(cur));
        }
    }
    // remember the remaining orphan directives
    ioDirectives.clear();
    ioDirectives.swap(unboundDirectives);
}

}  // namespace ast
}  // namespace souffle
