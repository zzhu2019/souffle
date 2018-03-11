/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Argument.cpp
 *
 * Define the classes Argument, Variable, and Constant to represent
 * variables and constants in literals. Variable and Constant are
 * sub-classes of class argument.
 *
 ***********************************************************************/

#include "Argument.h"
#include "Literal.h"

namespace souffle {
namespace ast {

std::vector<const Node*> Aggregator::getChildNodes() const {
    auto res = Argument::getChildNodes();
    if (expr) {
        res.push_back(expr.get());
    }
    for (auto& cur : body) {
        res.push_back(cur.get());
    }
    return res;
}

Aggregator* Aggregator::clone() const {
    auto res = new Aggregator(fun);
    res->expr = (expr) ? std::unique_ptr<Argument>(expr->clone()) : nullptr;
    for (const auto& cur : body) {
        res->body.push_back(std::unique_ptr<Literal>(cur->clone()));
    }
    res->setSrcLoc(getSrcLoc());
    return res;
}

void Aggregator::print(std::ostream& os) const {
    switch (fun) {
        case sum:
            os << "sum";
            break;
        case min:
            os << "min";
            break;
        case max:
            os << "max";
            break;
        case count:
            os << "count";
            break;
        default:
            break;
    }

    if (expr) {
        os << " " << *expr;
    }

    os << " : ";
    if (body.size() > 1) {
        os << "{ ";
    }

    os << join(body, ", ", print_deref<std::unique_ptr<Literal>>());

    if (body.size() > 1) {
        os << " }";
    }
}

}  // namespace ast
}  // namespace souffle
