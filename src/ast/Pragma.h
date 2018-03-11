/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Pragma.h
 *
 * Define the class Pragma to update global options based on parameter.
 *
 ***********************************************************************/

#pragma once

#include "Node.h"
#include "Transformer.h"

#include <string>

namespace souffle {
namespace ast {

/**
 * @class Pragma
 * @brief Representation of a global option
 */
class Pragma : public Node {
protected:
    /** An internal function to determine equality to another node */
    virtual bool equal(const Node& node) const {
        assert(dynamic_cast<const Pragma*>(&node));
        const Pragma& other = static_cast<const Pragma&>(node);
        return other.key == key && other.value == value;
    }

public:
    virtual ~Pragma() {}

    Pragma() {}

    Pragma(const std::string& k, const std::string& v) : key(k), value(v) {}

    /** Obtains a list of all embedded child nodes */
    virtual std::vector<const Node*> getChildNodes() const {
        // type is just cached, not essential
        return std::vector<const Node*>();
    }

    /** Creates a clone of this AST sub-structure */
    virtual Pragma* clone() const {
        auto res = new Pragma();
        res->key = key;
        res->value = value;
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** No nested nodes to apply to */
    virtual void apply(const NodeMapper& mapper) {}

    /** Output to a given output stream */
    virtual void print(std::ostream& os) const {
        os << ".pragma " << key << " " << value << "\n";
    }

    std::pair<std::string, std::string> getkvp() const {
        return std::pair<std::string, std::string>(key, value);
    }

    friend std::ostream& operator<<(std::ostream& out, const Pragma& arg) {
        out << arg.key << " \"" << arg.value << "\"";
        return out;
    }

    /** Name of the key */
    std::string key;

    /** Value */
    std::string value;
};

class PragmaChecker : public Transformer {
private:
    virtual bool transform(TranslationUnit&);

public:
    virtual std::string getName() const {
        return "PragmaChecker";
    }
};

}  // namespace ast
}  // namespace souffle
