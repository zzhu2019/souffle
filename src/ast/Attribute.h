/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Attribute.h
 *
 * Defines an attribute for a relation
 *
 ***********************************************************************/

#pragma once

#include "ast/Node.h"
#include "ast/Type.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <ctype.h>

namespace souffle {
class Type;
}

namespace souffle {
namespace ast {

// forward declaration

/**
 *  Intermediate representation of an attribute which stores the name and the type of an attribute
 *
 *  Attribute has the only name attribute
 */
class Attribute : public Node {
    /** Attribute name */
    std::string name;

    /** Type name */
    TypeIdentifier typeName;

public:
    Attribute(const std::string& n, const TypeIdentifier& t, const Type* /*type*/ = nullptr)
            : name(n), typeName(t) {}

    const std::string& getAttributeName() const {
        return name;
    }

    const TypeIdentifier& getTypeName() const {
        return typeName;
    }

    void setTypeName(const TypeIdentifier& name) {
        typeName = name;
    }

    void print(std::ostream& os) const override {
        os << name << ":" << typeName;
    }

    /** Creates a clone if this AST sub-structure */
    Attribute* clone() const override {
        Attribute* res = new Attribute(name, typeName);
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** Mutates this node */
    void apply(const NodeMapper& /*map*/) override {
        // no nested AST nodes
    }

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        return std::vector<const Node*>();
    }

protected:
    /** Implements the node comparison for this node type */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const Attribute*>(&node));
        const Attribute& other = static_cast<const Attribute&>(node);
        return name == other.name && typeName == other.typeName;
    }
};

}  // namespace ast
}  // namespace souffle
