/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Type.h
 *
 * Defines a type, i.e., disjoint supersets of the universe
 *
 ***********************************************************************/

#pragma once

#include "Node.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace souffle {
namespace ast {

/**
 * The type of identifier utilized for referencing types. Type
 * name identifiers are hierarchically qualified names, e.g.
 *
 *          problem.graph.edge
 *
 */
class TypeIdentifier {
    /**
     * The list of names forming this identifier.
     */
    std::vector<std::string> names;

public:
    // -- constructors --

    TypeIdentifier() : names() {}

    TypeIdentifier(const std::string& name) : names(toVector(name)) {}

    TypeIdentifier(const char* name) : TypeIdentifier(std::string(name)) {}

    TypeIdentifier(const TypeIdentifier&) = default;
    TypeIdentifier(TypeIdentifier&&) = default;

    // -- assignment operators --

    TypeIdentifier& operator=(const TypeIdentifier&) = default;
    TypeIdentifier& operator=(TypeIdentifier&&) = default;

    // -- mutators --

    void append(const std::string& name) {
        names.push_back(name);
    }

    void prepend(const std::string& name) {
        names.insert(names.begin(), name);
    }

    // -- getters and setters --

    bool empty() const {
        return names.empty();
    }

    const std::vector<std::string>& getNames() const {
        return names;
    }

    // -- comparison operators --

    bool operator==(const TypeIdentifier& other) const {
        return names == other.names;
    }

    bool operator!=(const TypeIdentifier& other) const {
        return !(*this == other);
    }

    bool operator<(const TypeIdentifier& other) const {
        return std::lexicographical_compare(
                names.begin(), names.end(), other.names.begin(), other.names.end());
    }

    void print(std::ostream& out) const {
        out << join(names, ".");
    }

    friend std::ostream& operator<<(std::ostream& out, const TypeIdentifier& id) {
        id.print(out);
        return out;
    }
};

/**
 * A overloaded operator to add a new prefix to a given relation identifier.
 */
inline TypeIdentifier operator+(const std::string& name, const TypeIdentifier& id) {
    TypeIdentifier res = id;
    res.prepend(name);
    return res;
}

/**
 *  @class Type
 *  @brief An abstract base class for types within the AST.
 *
 */
class Type : public Node {
    /** In the AST each type has to have a name forming a unique identifier */
    TypeIdentifier name;

public:
    /** Creates a new type */
    Type(const TypeIdentifier& name = "") : name(name) {}

    /** Obtains the name of this type */
    const TypeIdentifier& getName() const {
        return name;
    }

    /** Updates the name of this type */
    void setName(const TypeIdentifier& name) {
        this->name = name;
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        return {};
    }

    /** Creates a clone if this AST sub-structure */
    Type* clone() const override = 0;

    /** Mutates this node */
    void apply(const NodeMapper& /*map*/) override {
        // no nested nodes in any type
    }
};

/**
 * A primitive type is named type that can either be a sub-type of
 * the build-in number or symbol type. Primitive types are the most
 * basic building blocks of souffle's type system.
 */
class PrimitiveType : public Type {
    /** Indicates whether it is a number (true) or a symbol (false) */
    bool num;

public:
    /** Creates a new primitive type */
    PrimitiveType(const TypeIdentifier& name, bool num = false) : Type(name), num(num) {}

    /** Tests whether this type is a numeric type */
    bool isNumeric() const {
        return num;
    }

    /** Tests whether this type is a symbolic type */
    bool isSymbolic() const {
        return !num;
    }

    /** Prints a summary of this type to the given stream */
    void print(std::ostream& os) const override {
        os << ".type " << getName() << (num ? "= number" : "");
    }

    /** Creates a clone if this AST sub-structure */
    PrimitiveType* clone() const override {
        return new PrimitiveType(getName(), num);
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const PrimitiveType*>(&node));
        const PrimitiveType& other = static_cast<const PrimitiveType&>(node);
        return getName() == other.getName() && num == other.num;
    }
};

/**
 * A union type combines multiple types into a new super type.
 * Each of the enumerated types become a sub-type of the new
 * union type.
 */
class UnionType : public Type {
    /** The list of types aggregated by this union type */
    std::vector<TypeIdentifier> types;

public:
    /** Creates a new union type */
    UnionType() {}

    /** Obtains a reference to the list element types */
    const std::vector<TypeIdentifier>& getTypes() const {
        return types;
    }

    /** Adds another element type */
    void add(const TypeIdentifier& type) {
        types.push_back(type);
    }

    /** Prints a summary of this type to the given stream */
    void print(std::ostream& os) const override {
        os << ".type " << getName() << " = " << join(types, " | ");
    }

    /** Creates a clone if this AST sub-structure */
    UnionType* clone() const override {
        auto res = new UnionType();
        res->setName(getName());
        res->types = types;
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const UnionType*>(&node));
        const UnionType& other = static_cast<const UnionType&>(node);
        return getName() == other.getName() && types == other.types;
    }
};

/**
 * A record type aggregates a list of fields into a new type.
 * Each record type has a name making it unique. Two record
 * types are unrelated to all other types (they do not have
 * any super or sub types).
 */
class RecordType : public Type {
public:
    /** The type utilized to model a field */
    struct Field {
        std::string name;     // < the field name
        TypeIdentifier type;  // < the field type

        bool operator==(const Field& other) const {
            return this == &other || (name == other.name && type == other.type);
        }
    };

private:
    /** The list of fields constituting this record type */
    std::vector<Field> fields;

public:
    /** Creates a new record type */
    RecordType() {}

    /** Adds a new field to this record type */
    void add(const std::string& name, const TypeIdentifier& type) {
        fields.push_back(Field({name, type}));
    }

    /** Obtains the list of field constituting this record type */
    const std::vector<Field>& getFields() const {
        return fields;
    }

    /** Prints a summary of this type to the given stream */
    void print(std::ostream& os) const override {
        os << ".type " << getName() << " = "
           << "[";
        for (unsigned i = 0; i < fields.size(); i++) {
            if (i != 0) {
                os << ",";
            }
            os << fields[i].name;
            os << ":";
            os << fields[i].type;
        }
        os << "]";
    }

    /** Creates a clone if this AST sub-structure */
    RecordType* clone() const override {
        auto res = new RecordType();
        res->setName(getName());
        res->fields = fields;
        return res;
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const RecordType*>(&node));
        const RecordType& other = static_cast<const RecordType&>(node);
        return getName() == other.getName() && fields == other.fields;
    }
};

}  // namespace ast
}  // namespace souffle
