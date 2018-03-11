/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Literal.h
 *
 * Define classes for Literals and its subclasses atoms, negated atoms,
 * and binary relations.
 *
 ***********************************************************************/

#pragma once

#include "BinaryConstraintOps.h"
#include "Node.h"
#include "RelationIdentifier.h"
#include "Util.h"
#include "ast/Argument.h"

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace souffle {
namespace ast {

class Relation;
class Clause;
class Program;
class Atom;

/**
 * Intermediate representation of atoms, binary relations,
 * and negated atoms in the body and head of a clause.
 */
class Literal : public Node {
public:
    Literal() {}

    ~Literal() override = default;

    /** Obtains the atom referenced by this literal - if any */
    virtual const Atom* getAtom() const = 0;

    /** Creates a clone if this AST sub-structure */
    Literal* clone() const override = 0;
};

/**
 * Subclass of Literal that represents the use of a relation
 * either in the head or in the body of a Clause, e.g., parent(x,y).
 * The arguments of the atom can be variables or constants.
 */
class Atom : public Literal {
protected:
    /** Name of the atom */
    RelationIdentifier name;

    /** Arguments of the atom */
    std::vector<std::unique_ptr<Argument>> arguments;

public:
    Atom(const RelationIdentifier& name = RelationIdentifier()) : name(name) {}

    ~Atom() override = default;

    /** Return the name of this atom */
    const RelationIdentifier& getName() const {
        return name;
    }

    /** Return the arity of the atom */
    size_t getArity() const {
        return arguments.size();
    }

    /** Set atom name */
    void setName(const RelationIdentifier& n) {
        name = n;
    }

    /** Returns this class as the referenced atom */
    const Atom* getAtom() const override {
        return this;
    }

    /** Add argument to the atom */
    void addArgument(std::unique_ptr<Argument> arg) {
        arguments.push_back(std::move(arg));
    }

    /** Return the i-th argument of the atom */
    Argument* getArgument(size_t idx) const {
        return arguments[idx].get();
    }

    /** Replace the argument at the given index with the given argument */
    void setArgument(size_t idx, std::unique_ptr<Argument> newArg) {
        arguments[idx].swap(newArg);
    }

    /** Provides access to the list of arguments of this atom */
    std::vector<Argument*> getArguments() const {
        return toPtrVector(arguments);
    }

    /** Return the number of arguments */
    size_t argSize() const {
        return arguments.size();
    }

    /** Output to a given stream */
    void print(std::ostream& os) const override {
        os << getName() << "(";

        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i != 0) {
                os << ",";
            }
            if (arguments[i] != nullptr) {
                arguments[i]->print(os);
            } else {
                os << "_";
            }
        }
        os << ")";
    }

    /** Creates a clone if this AST sub-structure */
    Atom* clone() const override {
        auto res = new Atom(name);
        res->setSrcLoc(getSrcLoc());
        for (const auto& cur : arguments) {
            res->arguments.push_back(std::unique_ptr<Argument>(cur->clone()));
        }
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        for (auto& arg : arguments) {
            arg = map(std::move(arg));
        }
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        std::vector<const Node*> res;
        for (auto& cur : arguments) {
            res.push_back(cur.get());
        }
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Atom*>(&node));
        const Atom& other = static_cast<const Atom&>(node);
        return name == other.name && equal_targets(arguments, other.arguments);
    }
};

/**
 * Subclass of Literal that represents a negated atom, * e.g., !parent(x,y).
 * A Negated atom occurs in a body of clause and cannot occur in a head of a clause.
 */
class Negation : public Literal {
protected:
    /** A pointer to the negated Atom */
    std::unique_ptr<Atom> atom;

public:
    Negation(std::unique_ptr<Atom> a) : atom(std::move(a)) {}

    ~Negation() override = default;

    /** Returns the nested atom as the referenced atom */
    const Atom* getAtom() const override {
        return atom.get();
    }

    /** Return the negated atom */
    Atom* getAtom() {
        return atom.get();
    }

    /** Output to a given stream */
    void print(std::ostream& os) const override {
        os << "!";
        atom->print(os);
    }

    /** Creates a clone if this AST sub-structure */
    Negation* clone() const override {
        Negation* res = new Negation(std::unique_ptr<Atom>(atom->clone()));
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        atom = map(std::move(atom));
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        return {atom.get()};
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Negation*>(&node));
        const Negation& other = static_cast<const Negation&>(node);
        return *atom == *other.atom;
    }
};

/**
 * Subclass of Literal that represents a logical constraint
 */
class Constraint : public Literal {
public:
    ~Constraint() override = default;

    const Atom* getAtom() const override {
        // This kind of literal has no nested atom
        return nullptr;
    }

    /** Negates the constraint */
    virtual void negate() = 0;

    Constraint* clone() const override = 0;
};

/**
 * Subclass of Constraint that represents a constant 'true'
 * or 'false' value.
 */
class BooleanConstraint : public Constraint {
protected:
    bool truthValue;

public:
    BooleanConstraint(bool truthValue) : truthValue(truthValue) {}

    ~BooleanConstraint() override = default;

    bool isTrue() const {
        return truthValue;
    }

    void negate() override {
        truthValue = !truthValue;
    }

    void print(std::ostream& os) const override {
        os << (truthValue ? "true" : "false");
    }

    BooleanConstraint* clone() const override {
        BooleanConstraint* res = new BooleanConstraint(truthValue);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** No nested nodes to apply to */
    void apply(const NodeMapper& /*mapper*/) override {}

    /** No nested child nodes */
    std::vector<const Node*> getChildNodes() const override {
        return std::vector<const Node*>();
    }

protected:
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const BooleanConstraint*>(&node));
        const BooleanConstraint& other = static_cast<const BooleanConstraint&>(node);
        return truthValue == other.truthValue;
    }
};

/**
 * Subclass of Constraint that represents a binary constraint
 * e.g., x = y.
 */
class BinaryConstraint : public Constraint {
protected:
    /** The operator in this relation */
    BinaryConstraintOp operation;

    /** Left-hand side argument of a binary operation */
    std::unique_ptr<Argument> lhs;

    /** Right-hand side argument of a binary operation */
    std::unique_ptr<Argument> rhs;

public:
    BinaryConstraint(BinaryConstraintOp o, std::unique_ptr<Argument> ls, std::unique_ptr<Argument> rs)
            : operation(o), lhs(std::move(ls)), rhs(std::move(rs)) {}

    BinaryConstraint(const std::string& op, std::unique_ptr<Argument> ls, std::unique_ptr<Argument> rs)
            : operation(toBinaryConstraintOp(op)), lhs(std::move(ls)), rhs(std::move(rs)) {}

    ~BinaryConstraint() override = default;

    /** Return LHS argument */
    Argument* getLHS() const {
        return lhs.get();
    }

    /** Return RHS argument */
    Argument* getRHS() const {
        return rhs.get();
    }

    /** Return binary operator */
    BinaryConstraintOp getOperator() const {
        return operation;
    }

    /** Update the binary operator */
    void setOperator(BinaryConstraintOp op) {
        operation = op;
    }

    /** Negates the constraint */
    void negate() override {
        setOperator(souffle::negatedConstraintOp(operation));
    }

    /** Check whether constraint is a numeric constraint */
    const bool isNumerical() const {
        return isNumericBinaryConstraintOp(operation);
    }

    /** Check whether constraint is a symbolic constraint */
    const bool isSymbolic() const {
        return isSymbolicBinaryConstraintOp(operation);
    }

    /** Output the constraint to a given stream */
    void print(std::ostream& os) const override {
        lhs->print(os);
        os << " " << toBinaryConstraintSymbol(operation) << " ";
        rhs->print(os);
    }

    /** Creates a clone if this AST sub-structure */
    BinaryConstraint* clone() const override {
        BinaryConstraint* res = new BinaryConstraint(
                operation, std::unique_ptr<Argument>(lhs->clone()), std::unique_ptr<Argument>(rhs->clone()));
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        lhs = map(std::move(lhs));
        rhs = map(std::move(rhs));
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        return {lhs.get(), rhs.get()};
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const BinaryConstraint*>(&node));
        const BinaryConstraint& other = static_cast<const BinaryConstraint&>(node);
        return operation == other.operation && *lhs == *other.lhs && *rhs == *other.rhs;
    }
};

}  // namespace ast
}  // namespace souffle
