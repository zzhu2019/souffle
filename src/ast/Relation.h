/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Relation.h
 *
 * Defines class Relation that represents relations in a Datalog program.
 * A relation can either be an IDB or EDB relation.
 *
 ***********************************************************************/

#pragma once

#include "ast/Attribute.h"
#include "ast/Clause.h"
#include "ast/IODirective.h"
#include "ast/Node.h"
#include "ast/RelationIdentifier.h"
#include "ast/Type.h"

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <ctype.h>

/** Types of relation qualifiers defined as bits in a word */

/* relation is read from csv file */
#define INPUT_RELATION (0x1)

/* relation is written to csv file */
#define OUTPUT_RELATION (0x2)

/* number of tuples are written to stdout */
#define PRINTSIZE_RELATION (0x4)

/* Rules of a relation defined in a component can be overwritten by sub-component */
#define OVERRIDABLE_RELATION (0x8)

/* Relation is inlined */
#define INLINE_RELATION (0x20)

/* Relation uses a brie data structure */
#define BRIE_RELATION (0x40)

/* Relation uses a btree data structure */
#define BTREE_RELATION (0x80)

/* Relation uses a union relation */
#define EQREL_RELATION (0x100)

/* Relation uses a red-black tree set */
#define RBTSET_RELATION (0x200)

/* Relation uses a hash set */
#define HASHSET_RELATION (0x400)

namespace souffle {
namespace ast {

/*!
 * @class Relation
 * @brief Intermediate representation of a datalog relation
 *
 * A relation has a name, types of its arguments, qualifier type,
 * and dependencies to other relations.
 *
 */
class Relation : public Node {
protected:
    /** Name of relation */
    RelationIdentifier name;

    /** Attributes of the relation */
    std::vector<std::unique_ptr<Attribute>> attributes;

    /** Qualifier of relation (i.e., output or not an output relation) */
    // TODO: Change to a set of qualifiers
    int qualifier;

    /** Clauses associated with this relation. Clauses could be
     * either facts or rules.
     */
    std::vector<std::unique_ptr<Clause>> clauses;

    /** IO directives associated with this relation.
     */
    std::vector<std::unique_ptr<IODirective>> ioDirectives;

public:
    Relation() : qualifier(0) {}

    ~Relation() override = default;

    /** Return the name of the relation */
    const RelationIdentifier& getName() const {
        return name;
    }

    /** Set name for this relation */
    void setName(const RelationIdentifier& n) {
        name = n;
    }

    /** Add a new used type to this relation */
    void addAttribute(std::unique_ptr<Attribute> attr) {
        ASSERT(attr && "Undefined attribute");
        attributes.push_back(std::move(attr));
    }

    /** Return the arity of this relation */
    size_t getArity() const {
        return attributes.size();
    }

    /** Return the declared type at position @p idx */
    Attribute* getAttribute(size_t idx) const {
        return attributes[idx].get();
    }

    /** Obtains a list of the contained attributes */
    std::vector<Attribute*> getAttributes() const {
        return toPtrVector(attributes);
    }

    /** Return qualifier associated with this relation */
    int getQualifier() const {
        return qualifier;
    }

    /** Set qualifier associated with this relation */
    void setQualifier(int q) {
        qualifier = q;
    }

    /** Check whether relation is an output relation */
    bool isOutput() const {
        return (qualifier & OUTPUT_RELATION) != 0;
    }

    /** Check whether relation is an input relation */
    bool isInput() const {
        return (qualifier & INPUT_RELATION) != 0;
    }

    /** Check whether relation is a brie relation */
    bool isBrie() const {
        return (qualifier & BRIE_RELATION) != 0;
    }

    /** Check whether relation is a btree relation */
    bool isBTree() const {
        return (qualifier & BTREE_RELATION) != 0;
    }

    /** Check whether relation is a equivalence relation */
    bool isEqRel() const {
        return (qualifier & EQREL_RELATION) != 0;
    }

    /** Check whether relation is a red-black tree set relation */
    bool isRbtset() const {
        return (qualifier & RBTSET_RELATION) != 0;
    }

    /** Check whether relation is a hash set relation */
    bool isHashset() const {
        return (qualifier & HASHSET_RELATION) != 0;
    }

    /** Check whether relation is an input relation */
    bool isPrintSize() const {
        return (qualifier & PRINTSIZE_RELATION) != 0;
    }

    /** Check whether relation is an output relation */
    bool isComputed() const {
        return isOutput() || isPrintSize();
    }

    /** Check whether relation is an overridable relation */
    bool isOverridable() const {
        return (qualifier & OVERRIDABLE_RELATION) != 0;
    }

    /** Check whether relation is an inlined relation */
    bool isInline() const {
        return (qualifier & INLINE_RELATION) != 0;
    }

    /** Check whether relation has a record in its head */
    bool hasRecordInHead() const {
        for (auto& cur : clauses) {
            for (auto* arg : cur->getHead()->getArguments()) {
                if (dynamic_cast<RecordInit*>(arg)) {
                    return true;
                }
            };
        }
        return false;
    }

    /** Operator overload, calls print if reference is given */
    friend std::ostream& operator<<(std::ostream& os, const Relation& rel) {
        rel.print(os);
        return os;
    }

    /** Operator overload, prints name if pointer is given */
    friend std::ostream& operator<<(std::ostream& os, const Relation* rel) {
        os << rel->getName();
        return os;
    }

    /** Print string representation of the relation to a given output stream */
    void print(std::ostream& os) const override {
        os << ".decl " << this->getName() << "(";
        if (!attributes.empty()) {
            os << attributes[0]->getAttributeName() << ":" << attributes[0]->getTypeName();

            for (size_t i = 1; i < attributes.size(); ++i) {
                os << "," << attributes[i]->getAttributeName() << ":" << attributes[i]->getTypeName();
            }
        }
        os << ") ";
        if (isInput()) {
            os << "input ";
        }
        if (isOutput()) {
            os << "output ";
        }
        if (isPrintSize()) {
            os << "printsize ";
        }
        if (isOverridable()) {
            os << "overridable ";
        }
        if (isInline()) {
            os << "inline ";
        }
        if (isBTree()) {
            os << "btree ";
        }
        if (isBrie()) {
            os << "brie ";
        }
        if (isRbtset()) {
            os << "rbtset ";
        }
        if (isHashset()) {
            os << "hashset ";
        }
        if (isEqRel()) {
            os << "eqrel ";
        }
    }

    /** Creates a clone if this AST sub-structure */
    Relation* clone() const override {
        auto res = new Relation();
        res->name = name;
        res->setSrcLoc(getSrcLoc());
        for (const auto& cur : attributes) {
            res->attributes.push_back(std::unique_ptr<Attribute>(cur->clone()));
        }
        for (const auto& cur : clauses) {
            res->clauses.push_back(std::unique_ptr<Clause>(cur->clone()));
        }
        for (const auto& cur : ioDirectives) {
            res->ioDirectives.push_back(std::unique_ptr<IODirective>(cur->clone()));
        }
        res->qualifier = qualifier;
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        for (auto& cur : attributes) {
            cur = map(std::move(cur));
        }
        for (auto& cur : clauses) {
            cur = map(std::move(cur));
        }
        for (auto& cur : ioDirectives) {
            cur = map(std::move(cur));
        }
    }

    /** Return i-th clause associated with this relation */
    Clause* getClause(size_t idx) const {
        return clauses[idx].get();
    }

    /** Obtains a list of the associated clauses */
    std::vector<Clause*> getClauses() const {
        return toPtrVector(clauses);
    }

    /** Add a clause to the relation */
    void addClause(std::unique_ptr<Clause> clause) {
        ASSERT(clause && "Undefined clause");
        ASSERT(clause->getHead() && "Undefined head of the clause");
        ASSERT(clause->getHead()->getName() == name &&
                "Name of the atom in the head of the clause and the relation do not match");
        clauses.push_back(std::move(clause));
    }

    /** Removes the given clause from this relation */
    bool removeClause(const Clause* clause) {
        for (auto it = clauses.begin(); it != clauses.end(); ++it) {
            if (**it == *clause) {
                clauses.erase(it);
                return true;
            }
        }
        return false;
    }

    /** Return the number of clauses associated with this relation */
    size_t clauseSize() const {
        return clauses.size();
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        std::vector<const Node*> res;
        for (const auto& cur : attributes) {
            res.push_back(cur.get());
        }
        for (const auto& cur : clauses) {
            res.push_back(cur.get());
        }
        for (const auto& cur : ioDirectives) {
            res.push_back(cur.get());
        }
        return res;
    }

    void addIODirectives(std::unique_ptr<IODirective> directive) {
        ASSERT(directive && "Undefined directive");
        // Make sure the old style qualifiers still work.
        if (directive->isInput()) {
            qualifier |= INPUT_RELATION;
        } else if (directive->isOutput()) {
            qualifier |= OUTPUT_RELATION;
        } else if (directive->isPrintSize()) {
            qualifier |= PRINTSIZE_RELATION;
        }
        // Fall back on default behaviour for empty directives.
        if (!directive->getIODirectiveMap().empty()) {
            ioDirectives.push_back(std::move(directive));
        }
    }

    std::vector<IODirective*> getIODirectives() const {
        return toPtrVector(ioDirectives);
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Relation*>(&node));
        const Relation& other = static_cast<const Relation&>(node);
        return name == other.name && equal_targets(attributes, other.attributes) &&
               equal_targets(clauses, other.clauses);
    }
};

struct NameComparison {
    bool operator()(const Relation* x, const Relation* y) const {
        if (x != nullptr && y != nullptr) {
            return x->getName() < y->getName();
        }
        return y != nullptr;
    }
};

typedef std::set<const Relation*, NameComparison> RelationSet;

}  // namespace ast
}  // namespace souffle
