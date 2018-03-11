/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file IODirective.h
 *
 * Define the class IODirective to hold key=value pairs for IO.
 *
 ***********************************************************************/

#pragma once

#include "Node.h"
#include "RelationIdentifier.h"

#include <map>
#include <string>

namespace souffle {
namespace ast {

/**
 * @class IODirective
 * @brief Intermediate representation of an argument of a Literal (e.g., a variable or a constant)
 */
class IODirective : public Node {
public:
    ~IODirective() override = default;

    /** Obtains a list of all embedded child nodes */
    std::vector<const Node*> getChildNodes() const override {
        // type is just cached, not essential
        return std::vector<const Node*>();
    }

    /** Creates a clone of this AST sub-structure */
    IODirective* clone() const override {
        auto res = new IODirective();
        res->names = names;
        res->kvps = kvps;
        res->input = input;
        res->output = output;
        res->printSize = printSize;
        res->setSrcLoc(getSrcLoc());
        return res;
    }

    /** No nested nodes to apply to */
    void apply(const NodeMapper& /*mapper*/) override {}

    /** Output to a given output stream */
    void print(std::ostream& os) const override {
        if (input) {
            os << ".input ";
        }
        if (output) {
            os << ".output ";
        }
        if (printSize) {
            os << ".printsize ";
        }
        os << getNames() << "(";
        bool first = true;
        for (auto& pair : kvps) {
            if (first) {
                first = false;
            } else {
                os << ',';
            }
            os << pair.first << "=\"" << pair.second << "\"";
        }
        os << ')';
    }

    /** Return the name of this kvp map */
    const RelationIdentifier& getName() const {
        return *names.begin();
    }

    /** Return the names of this kvp map */
    const std::set<RelationIdentifier>& getNames() const {
        return names;
    }

    /** Set kvp map name */
    void addName(const RelationIdentifier& name) {
        names.insert(name);
    }
    /** Set kvp map name */
    void setName(const RelationIdentifier& name) {
        names.clear();
        names.insert(name);
    }

    void addKVP(const std::string& key, const std::string& value) {
        kvps[key] = unescape(value);
    }

    const std::map<std::string, std::string>& getIODirectiveMap() {
        return kvps;
    }

    void setAsInput() {
        input = true;
    }
    void setAsOutput() {
        output = true;
    }
    void setAsPrintSize() {
        printSize = true;
    }

    bool isInput() {
        return input;
    }
    bool isOutput() {
        return output;
    }
    bool isPrintSize() {
        return printSize;
    }

protected:
    /** An internal function to determine equality to another node */
    bool equal(const Node& node) const override {
        assert(dynamic_cast<const IODirective*>(&node));
        const IODirective& other = static_cast<const IODirective&>(node);
        return other.names == names && other.input == input && other.kvps == kvps;
    }

    std::string unescape(const std::string& inputString) const {
        std::string unescaped = unescape(inputString, "\\\"", "\"");
        unescaped = unescape(unescaped, "\\t", "\t");
        unescaped = unescape(unescaped, "\\r", "\r");
        unescaped = unescape(unescaped, "\\n", "\n");
        return unescaped;
    }

    std::string unescape(
            const std::string& inputString, const std::string& needle, const std::string replacement) const {
        std::string result = inputString;
        size_t pos = 0;
        while ((pos = result.find(needle, pos)) != std::string::npos) {
            result = result.replace(pos, needle.length(), replacement);
            pos += replacement.length();
        }
        return result;
    }
    /** Name of the kvp */
    std::set<RelationIdentifier> names;

    /** kvp map */
    std::map<std::string, std::string> kvps;
    /** input = true, output = false */
    bool input = false;
    bool output = false;
    bool printSize = false;
};

}  // namespace ast
}  // namespace souffle
