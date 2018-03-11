/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Program.h
 *
 * Define a class that represents a Datalog program consisting of types,
 * relations, and clauses.
 *
 ***********************************************************************/

#pragma once

#include "Component.h"
#include "ComponentModel.h"
#include "ErrorReport.h"
#include "Pragma.h"
#include "Relation.h"
#include "TypeSystem.h"
#include "Util.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace souffle {
class ParserDriver;

namespace ast {

class Argument;
class Atom;
class Clause;
class Literal;
class ProvenanceTransformer;
class Relation;

/**
 *  Intermediate representation of a datalog program
 *          that consists of relations, clauses and types
 */
class Program : public Node {
    // TODO: Check whether this is needed
    friend class ::souffle::ParserDriver;
    friend class ComponentInstantiationTransformer;
    friend class ProvenanceTransformer;

    /** Program types  */
    std::map<TypeIdentifier, std::unique_ptr<Type>> types;

    /** Program relations */
    std::map<RelationIdentifier, std::unique_ptr<Relation>> relations;

    /** The list of clauses provided by the user */
    std::vector<std::unique_ptr<Clause>> clauses;

    /** The list of IO directives provided by the user */
    std::vector<std::unique_ptr<IODirective>> ioDirectives;

    /** Program components */
    std::vector<std::unique_ptr<Component>> components;

    /** Component instantiations */
    std::vector<std::unique_ptr<ComponentInit>> instantiations;

    /** Pragmas */
    std::vector<std::unique_ptr<Pragma>> pragmaDirectives;

    /** a private constructor to restrict creation */
    Program() = default;

public:
    /** Deleted copy constructor since instances can not be copied */
    Program(const Program&) = delete;

    /** A move constructor */
    Program(Program&& other) noexcept;

    /** A programs destructor */
    ~Program() override = default;

    // -- Types ----------------------------------------------------------------

private:
    /** Add the given type to the program. Asserts if a type with the
      same name has already been added.  */
    void addType(std::unique_ptr<Type> type);

public:
    /** Obtains the type with the given name */
    const Type* getType(const TypeIdentifier& name) const;

    /** Gets a list of all types in this program */
    std::vector<const Type*> getTypes() const;

    // -- Relations ------------------------------------------------------------

private:
    /** Add the given relation to the program. Asserts if a relation with the
     * same name has already been added. */
    void addRelation(std::unique_ptr<Relation> r);

    /** Add a clause to the program */
    void addClause(std::unique_ptr<Clause> clause);

    /** Add an IO directive to the program */
    void addIODirective(std::unique_ptr<IODirective> directive);

    /** Add a pragma to the program */
    void addPragma(std::unique_ptr<Pragma> r);

public:
    /** Find and return the relation in the program given its name */
    Relation* getRelation(const RelationIdentifier& name) const;

    /** Get all relations in the program */
    std::vector<Relation*> getRelations() const;

    /** Get all io directives in the program */
    const std::vector<std::unique_ptr<IODirective>>& getIODirectives() const;

    /** Get all pragma directives in the program */
    const std::vector<std::unique_ptr<Pragma>>& getPragmaDirectives() const;

    /** Return the number of relations in the program */
    size_t relationSize() const {
        return relations.size();
    }

    /** appends a new relation to this program -- after parsing */
    void appendRelation(std::unique_ptr<Relation> r);

    /** Remove a relation from the program. */
    void removeRelation(const RelationIdentifier& name);

    /** append a new clause to this program -- after parsing */
    void appendClause(std::unique_ptr<Clause> clause);

    /** Removes a clause from this program */
    void removeClause(const Clause* clause);

    /**
     * Obtains a list of clauses not associated to any relations. In
     * a valid program this list is always empty
     */
    std::vector<Clause*> getOrphanClauses() const {
        return toPtrVector(clauses);
    }

    // -- Components -----------------------------------------------------------

private:
    /** Adds the given component to this program */
    void addComponent(std::unique_ptr<Component> c) {
        components.push_back(std::move(c));
    }

    /** Adds a component instantiation */
    void addInstantiation(std::unique_ptr<ComponentInit> i) {
        instantiations.push_back(std::move(i));
    }

public:
    /** Obtains a list of all comprised components */
    std::vector<Component*> getComponents() const {
        return toPtrVector(components);
    }

    /** Obtains a list of all component instantiations */
    std::vector<ComponentInit*> getComponentInstantiations() const {
        return toPtrVector(instantiations);
    }

    // -- I/O ------------------------------------------------------------------

    /** Output the program to a given output stream */
    void print(std::ostream& os) const override;

    // -- Manipulation ---------------------------------------------------------

    /** Creates a clone if this AST sub-structure */
    Program* clone() const override;

    /** Mutates this node */
    void apply(const NodeMapper& map) override;

public:
    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        std::vector<const Node*> res;
        for (const auto& cur : types) {
            res.push_back(cur.second.get());
        }
        for (const auto& cur : relations) {
            res.push_back(cur.second.get());
        }
        for (const auto& cur : components) {
            res.push_back(cur.get());
        }
        for (const auto& cur : instantiations) {
            res.push_back(cur.get());
        }
        for (const auto& cur : clauses) {
            res.push_back(cur.get());
        }
        for (const auto& cur : pragmaDirectives) {
            res.push_back(cur.get());
        }
        for (const auto& cur : ioDirectives) {
            res.push_back(cur.get());
        }
        return res;
    }

private:
    void finishParsing();

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Program*>(&node));
        const Program& other = static_cast<const Program&>(node);

        // check list sizes
        if (types.size() != other.types.size()) {
            return false;
        }
        if (relations.size() != other.relations.size()) {
            return false;
        }

        // check types
        for (const auto& cur : types) {
            auto pos = other.types.find(cur.first);
            if (pos == other.types.end()) {
                return false;
            }
            if (*cur.second != *pos->second) {
                return false;
            }
        }

        // check relations
        for (const auto& cur : relations) {
            auto pos = other.relations.find(cur.first);
            if (pos == other.relations.end()) {
                return false;
            }
            if (*cur.second != *pos->second) {
                return false;
            }
        }

        // check components
        if (!equal_targets(components, other.components)) {
            return false;
        }
        if (!equal_targets(instantiations, other.instantiations)) {
            return false;
        }
        if (!equal_targets(clauses, other.clauses)) {
            return false;
        }
        if (!equal_targets(ioDirectives, other.ioDirectives)) {
            return false;
        }

        // no different found => programs are equal
        return true;
    }
};

}  // namespace ast
}  // namespace souffle
