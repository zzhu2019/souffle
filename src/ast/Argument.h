/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Argument.h
 *
 * Define the classes Argument, Variable, and Constant to represent
 * variables and constants in literals. Variable and Constant are
 * sub-classes of class argument.
 *
 ***********************************************************************/

#pragma once

#include "BinaryFunctorOps.h"
#include "ast/Node.h"
#include "SymbolTable.h"
#include "TernaryFunctorOps.h"
#include "ast/Type.h"
#include "TypeSystem.h"
#include "UnaryFunctorOps.h"

#include <array>
#include <list>
#include <memory>
#include <string>

namespace souffle {
namespace ast {

/* Forward declarations */
class Clause;
class Variable;
class Literal;

/**
 * Intermediate representation of an argument
 */
class Argument : public Node {
public:
    ~Argument() override = default;

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        return std::vector<const Node*>();  // type is just cached, not essential
    }

    /** Create clone */
    Argument* clone() const override = 0;
};

/**
 * Subclass of Argument that represents a named variable
 */
class Variable : public Argument {
protected:
    /** Variable name */
    std::string name;

public:
    Variable(const std::string& n) : Argument(), name(n) {}

    /** Updates this variable name */
    void setName(const std::string& name) {
        this->name = name;
    }

    /** @return Variable name */
    const std::string& getName() const {
        return name;
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << name;
    }

    /** Creates a clone if this AST sub-structure */
    Variable* clone() const override {
        Variable* res = new Variable(name);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& /*mapper*/) override {
        // no sub-nodes to consider
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Variable*>(&node));
        const Variable& other = static_cast<const Variable&>(node);
        return name == other.name;
    }
};

/**
 * Subclass of Argument that represents an unnamed variable
 */
class UnnamedVariable : public Argument {
protected:
public:
    UnnamedVariable() : Argument() {}

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "_";
    }

    /** Creates a clone if this AST sub-structure */
    UnnamedVariable* clone() const override {
        UnnamedVariable* res = new UnnamedVariable();
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& /*mapper*/) override {
        // no sub-nodes to consider
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const UnnamedVariable*>(&node));
        return true;
    }
};

/**
 * Subclass of Argument that represents a counter (for projections only)
 */
class Counter : public Argument {
protected:
public:
    Counter() : Argument() {}

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "$";
    }

    /** Creates a clone of this AST sub-structure */
    Counter* clone() const override {
        Counter* res = new Counter();
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& /*mapper*/) override {
        // no sub-nodes to consider within constants
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Counter*>(&node));
        return true;
    }
};

/**
 * Subclass of Argument that represents a datalog constant value
 */
class Constant : public Argument {
protected:
    /** Index of this Constant in the SymbolTable */
    Domain idx;

public:
    Constant(Domain i) : Argument(), idx(i) {}

    /** @return Return the index of this constant in the SymbolTable */
    Domain getIndex() const {
        return idx;
    }

    /** Mutates this node */
    void apply(const NodeMapper& /*mapper*/) override {
        // no sub-nodes to consider within constants
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Constant*>(&node));
        const Constant& other = static_cast<const Constant&>(node);
        return idx == other.idx;
    }
};

/**
 * Subclass of Argument that represents a datalog constant value
 */
class StringConstant : public Constant {
    SymbolTable* symTable;
    StringConstant(SymbolTable* symTable, size_t index) : Constant(index), symTable(symTable) {}

public:
    StringConstant(SymbolTable& symTable, const char* c)
            : Constant(symTable.lookup(c)), symTable(&symTable) {}

    /** @return String representation of this Constant */
    const std::string getConstant() const {
        return symTable->resolve(getIndex());
    }

    /**  Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "\"" << getConstant() << "\"";
    }

    /** Creates a clone if this AST sub-structure */
    StringConstant* clone() const override {
        StringConstant* res = new StringConstant(symTable, getIndex());
        res->setSrcLoc(getSrcLoc());
        return res;
    }
};

/**
 * Subclass of Argument that represents a datalog constant value
 */
class NumberConstant : public Constant {
public:
    NumberConstant(Domain num) : Constant(num) {}

    /**  Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << idx;
    }

    /** Creates a clone if this AST sub-structure */
    NumberConstant* clone() const override {
        NumberConstant* res = new NumberConstant(getIndex());
        res->setSrcLoc(getSrcLoc());
        return res;
    }
};

/**
 * Subclass of Constant that represents a null-constant (no record)
 */
class NullConstant : public Constant {
public:
    NullConstant() : Constant(0) {}

    /**  Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << '-';
    }

    /** Creates a clone if this AST sub-structure */
    NullConstant* clone() const override {
        NullConstant* res = new NullConstant();
        res->setSrcLoc(getSrcLoc());
        return res;
    }
};

/**
 * A common base class for AST functors
 */
class Functor : public Argument {};

/**
 * Subclass of Argument that represents a unary function
 */
class UnaryFunctor : public Functor {
protected:
    UnaryOp fun;

    std::unique_ptr<Argument> operand;

public:
    UnaryFunctor(UnaryOp fun, std::unique_ptr<Argument> o) : fun(fun), operand(std::move(o)) {}

    ~UnaryFunctor() override = default;

    Argument* getOperand() const {
        return operand.get();
    }

    UnaryOp getFunction() const {
        return fun;
    }

    /** Check if the return value of this functor is a number type. */
    bool isNumerical() const {
        return isNumericUnaryOp(fun);
    }

    /** Check if the return value of this functor is a symbol type. */
    bool isSymbolic() const {
        return isSymbolicUnaryOp(fun);
    }

    /** Check if the argument of this functor is a number type. */
    bool acceptsNumbers() const {
        return unaryOpAcceptsNumbers(fun);
    }

    /** Check if the argument of this functor is a symbol type. */
    bool acceptsSymbols() const {
        return unaryOpAcceptsSymbols(fun);
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << getSymbolForUnaryOp(fun);
        os << "(";
        operand->print(os);
        os << ")";
    }

    /** Creates a clone */
    UnaryFunctor* clone() const override {
        auto res = new UnaryFunctor(fun, std::unique_ptr<Argument>(operand->clone()));
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        operand = map(std::move(operand));
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        auto res = Argument::getChildNodes();
        res.push_back(operand.get());
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const UnaryFunctor*>(&node));
        const UnaryFunctor& other = static_cast<const UnaryFunctor&>(node);
        return fun == other.fun && *operand == *other.operand;
    }
};

/**
 * Subclass of Argument that represents a binary function
 */
class BinaryFunctor : public Functor {
protected:
    BinaryOp fun;                   // binary operator
    std::unique_ptr<Argument> lhs;  // first argument
    std::unique_ptr<Argument> rhs;  // second argument

public:
    BinaryFunctor(BinaryOp fun, std::unique_ptr<Argument> l, std::unique_ptr<Argument> r)
            : fun(fun), lhs(std::move(l)), rhs(std::move(r)) {}

    ~BinaryFunctor() override = default;

    Argument* getLHS() const {
        return lhs.get();
    }

    Argument* getRHS() const {
        return rhs.get();
    }

    BinaryOp getFunction() const {
        return fun;
    }

    /** Check if the return value of this functor is a number type. */
    bool isNumerical() const {
        return isNumericBinaryOp(fun);
    }

    /** Check if the return value of this functor is a symbol type. */
    bool isSymbolic() const {
        return isSymbolicBinaryOp(fun);
    }

    /** Check if the arguments of this functor are number types. */
    bool acceptsNumbers(int arg) const {
        return binaryOpAcceptsNumbers(arg, fun);
    }

    /** Check if the arguments of this functor are symbol types. */
    bool acceptsSymbols(int arg) const {
        return binaryOpAcceptsSymbols(arg, fun);
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        if (fun < BinaryOp::MAX) {
            os << "(";
            lhs->print(os);
            os << getSymbolForBinaryOp(fun);
            rhs->print(os);
            os << ")";
        } else {
            os << getSymbolForBinaryOp(fun);
            os << "(";
            lhs->print(os);
            os << ",";
            rhs->print(os);
            os << ")";
        }
    }

    /** Creates a clone */
    BinaryFunctor* clone() const override {
        auto res = new BinaryFunctor(
                fun, std::unique_ptr<Argument>(lhs->clone()), std::unique_ptr<Argument>(rhs->clone()));
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
        auto res = Argument::getChildNodes();
        res.push_back(lhs.get());
        res.push_back(rhs.get());
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const BinaryFunctor*>(&node));
        const BinaryFunctor& other = static_cast<const BinaryFunctor&>(node);
        return fun == other.fun && *lhs == *other.lhs && *rhs == *other.rhs;
    }
};

/**
 * @class TernaryFunctor
 * @brief Subclass of Argument that represents a binary functor
 */
class TernaryFunctor : public Functor {
protected:
    TernaryOp fun;
    std::array<std::unique_ptr<Argument>, 3> arg;

public:
    TernaryFunctor(TernaryOp fun, std::unique_ptr<Argument> a1, std::unique_ptr<Argument> a2,
            std::unique_ptr<Argument> a3)
            : fun(fun), arg({{std::move(a1), std::move(a2), std::move(a3)}}) {}

    ~TernaryFunctor() override = default;

    Argument* getArg(int idx) const {
        assert(idx >= 0 && idx < 3 && "wrong argument");
        return arg[idx].get();
    }

    TernaryOp getFunction() const {
        return fun;
    }

    /** Check if the return value of this functor is a number type. */
    bool isNumerical() const {
        return isNumericTernaryOp(fun);
    }

    /** Check if the return value of this functor is a symbol type. */
    bool isSymbolic() const {
        return isSymbolicTernaryOp(fun);
    }

    /** Check if the arguments of this functor are number types. */
    bool acceptsNumbers(int arg) const {
        return ternaryOpAcceptsNumbers(arg, fun);
    }

    /** Check if the arguments of this functor are symbol types. */
    bool acceptsSymbols(int arg) const {
        return ternaryOpAcceptsSymbols(arg, fun);
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << getSymbolForTernaryOp(fun);
        os << "(";
        arg[0]->print(os);
        os << ",";
        arg[1]->print(os);
        os << ",";
        arg[2]->print(os);
        os << ")";
    }

    /** Clone this node  */
    TernaryFunctor* clone() const override {
        auto res = new TernaryFunctor(fun, std::unique_ptr<Argument>(arg[0]->clone()),
                std::unique_ptr<Argument>(arg[1]->clone()), std::unique_ptr<Argument>(arg[2]->clone()));
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        arg[0] = map(std::move(arg[0]));
        arg[1] = map(std::move(arg[1]));
        arg[2] = map(std::move(arg[2]));
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        auto res = Argument::getChildNodes();
        res.push_back(arg[0].get());
        res.push_back(arg[1].get());
        res.push_back(arg[2].get());
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const TernaryFunctor*>(&node));
        const TernaryFunctor& other = static_cast<const TernaryFunctor&>(node);
        return fun == other.fun && *arg[0] == *other.arg[0] && *arg[1] == *other.arg[1] &&
               *arg[2] == *other.arg[2];
    }
};

/**
 * An argument that takes a list of values and combines them into a
 * new record.
 */
class RecordInit : public Argument {
    /** The list of components to be aggregated into a record */
    std::vector<std::unique_ptr<Argument>> args;

public:
    RecordInit() {}

    ~RecordInit() override = default;

    void add(std::unique_ptr<Argument> arg) {
        args.push_back(std::move(arg));
    }

    std::vector<Argument*> getArguments() const {
        return toPtrVector(args);
    }

    void print(std::ostream& os) const override {
        os << "[" << join(args, ",", print_deref<std::unique_ptr<Argument>>()) << "]";
    }

    /** Creates a clone if this AST sub-structure */
    RecordInit* clone() const override {
        auto res = new RecordInit();
        for (auto& cur : args) {
            res->args.push_back(std::unique_ptr<Argument>(cur->clone()));
        }
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        for (auto& arg : args) {
            arg = map(std::move(arg));
        }
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        auto res = Argument::getChildNodes();
        for (auto& cur : args) {
            res.push_back(cur.get());
        }
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const RecordInit*>(&node));
        const RecordInit& other = static_cast<const RecordInit&>(node);
        return equal_targets(args, other.args);
    }
};

/**
 * An argument capable of casting a value of one type into another.
 */
class TypeCast : public Argument {
    /** The value to be casted */
    std::unique_ptr<Argument> value;

    /** The target type name */
    std::string type;

public:
    TypeCast(std::unique_ptr<Argument> value, const std::string& type)
            : value(std::move(value)), type(type) {}

    void print(std::ostream& os) const override {
        os << *value << " as " << type;
    }

    Argument* getValue() const {
        return value.get();
    }

    const std::string& getType() const {
        return type;
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        auto res = Argument::getChildNodes();
        res.push_back(value.get());
        return res;
    }

    /** Creates a clone if this AST sub-structure */
    TypeCast* clone() const override {
        auto res = new TypeCast(std::unique_ptr<Argument>(value->clone()), type);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        value = map(std::move(value));
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const TypeCast*>(&node));
        const TypeCast& other = static_cast<const TypeCast&>(node);
        return type == other.type && *value == *other.value;
    }
};

/**
 * An argument aggregating a value from a sub-query.
 */
class Aggregator : public Argument {
public:
    /**
     * The kind of utilized aggregation operator.
     * Note: lower-case is utilized due to a collision with
     *  constants in the parser.
     */
    enum Op { min, max, count, sum };

private:
    /** The aggregation operator of this aggregation step */
    Op fun;

    /** The expression to be aggregated */
    std::unique_ptr<Argument> expr;

    /** A list of body-literals forming a sub-query which's result is projected and aggregated */
    std::vector<std::unique_ptr<Literal>> body;

public:
    /** Creates a new aggregation node */
    Aggregator(Op fun) : fun(fun), expr(nullptr) {}

    /** Destructor */
    ~Aggregator() override = default;

    // -- getters and setters --

    Op getOperator() const {
        return fun;
    }

    void setTargetExpression(std::unique_ptr<Argument> arg) {
        expr = std::move(arg);
    }

    const Argument* getTargetExpression() const {
        return expr.get();
    }

    std::vector<Literal*> getBodyLiterals() const {
        return toPtrVector(body);
    }

    void clearBodyLiterals() {
        body.clear();
    }

    void addBodyLiteral(std::unique_ptr<Literal> lit) {
        body.push_back(std::move(lit));
    }

    // -- others --

    /** Prints this instance in a parse-able format */
    void print(std::ostream& os) const override;

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override;

    /** Creates a clone if this AST sub-structure */
    Aggregator* clone() const override;

    /** Mutates this node */
    void apply(const NodeMapper& map) override {
        if (expr) {
            expr = map(std::move(expr));
        }
        for (auto& cur : body) {
            cur = map(std::move(cur));
        }
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Aggregator*>(&node));
        const Aggregator& other = static_cast<const Aggregator&>(node);
        return fun == other.fun && equal_ptr(expr, other.expr) && equal_targets(body, other.body);
    }
};

/**
 * An argument taking its value from an argument of a RAM subroutine
 */
class SubroutineArgument : public Argument {
private:
    /** Index of argument in argument list*/
    size_t number;

public:
    SubroutineArgument(size_t n) : Argument(), number(n) {}

    /** Return argument number */
    size_t getNumber() const {
        return number;
    }

    /** Print argument to the given output stream */
    void print(std::ostream& os) const override {
        os << "arg_" << number;
    }

    /** Creates a clone if this AST sub-structure */
    SubroutineArgument* clone() const override {
        SubroutineArgument* res = new SubroutineArgument(number);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& /*mapper*/) override {
        // no sub-nodes to consider
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const SubroutineArgument*>(&node));
        const SubroutineArgument& other = static_cast<const SubroutineArgument&>(node);
        return number == other.number;
    }
};

}  // namespace ast
}  // namespace souffle
