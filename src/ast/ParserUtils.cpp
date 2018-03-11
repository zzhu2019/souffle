/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ParserUtils.cpp
 *
 * Defines class RuleBody to represents rule bodies.
 *
 ***********************************************************************/

#include "ParserUtils.h"
#include "Clause.h"

#include <memory>
#include <vector>

namespace souffle {
namespace ast {

void RuleBody::negate() {
    RuleBody res = getTrue();

    for (const clause& cur : dnf) {
        RuleBody step = getFalse();

        for (const literal& lit : cur) {
            step.dnf.push_back(clause());
            clause& cl = step.dnf.back();
            cl.emplace_back(literal{!lit.negated, std::unique_ptr<Literal>(lit.atom->clone())});
        }

        res.conjunct(std::move(step));
    }

    dnf.swap(res.dnf);
}

void RuleBody::conjunct(RuleBody&& other) {
    std::vector<clause> res;

    for (const auto& clauseA : dnf) {
        for (const auto& clauseB : other.dnf) {
            // create a new clause in result
            clause cur;

            // it is the concatenation of the two clauses
            for (const auto& lit : clauseA) {
                cur.emplace_back(lit.clone());
            }
            for (const auto& lit : clauseB) {
                insert(cur, lit.clone());
            }

            insert(res, std::move(cur));
        }
    }

    // update local dnf
    dnf.swap(res);
}

void RuleBody::disjunct(RuleBody&& other) {
    // append the clauses of the other body to this body
    for (auto& cur : other.dnf) {
        // insert clause
        insert(dnf, std::move(cur));
    }
}

std::vector<Clause*> RuleBody::toClauseBodies() const {
    // collect clause results
    std::vector<Clause*> bodies;
    for (const clause& cur : dnf) {
        bodies.push_back(new Clause());
        Clause& clause = *bodies.back();

        for (const literal& lit : cur) {
            // extract literal
            Literal* base = lit.atom->clone();
            // negate if necessary
            if (lit.negated) {
                // negate
                if (Atom* atom = dynamic_cast<Atom*>(base)) {
                    base = new Negation(std::unique_ptr<Atom>(atom));
                    base->setSrcLoc(atom->getSrcLoc());
                } else if (Constraint* cstr = dynamic_cast<Constraint*>(base)) {
                    cstr->negate();
                }
            }

            // add to result
            clause.addToBody(std::unique_ptr<Literal>(base));
        }
    }

    // done
    return bodies;
}

// -- factory functions --

RuleBody RuleBody::getTrue() {
    RuleBody body;
    body.dnf.push_back(clause());
    return body;
}

RuleBody RuleBody::getFalse() {
    return RuleBody();
}

RuleBody RuleBody::atom(Atom* atom) {
    RuleBody body;
    body.dnf.push_back(clause());
    auto& clause = body.dnf.back();
    clause.push_back(literal());
    clause.back() = literal{false, std::unique_ptr<Atom>(atom)};
    return body;
}

RuleBody RuleBody::constraint(Constraint* constraint) {
    RuleBody body;
    body.dnf.push_back(clause());
    auto& clause = body.dnf.back();
    clause.push_back(literal());
    clause.back() = literal{false, std::unique_ptr<Literal>(constraint)};
    return body;
}

std::ostream& operator<<(std::ostream& out, const RuleBody& body) {
    return out << join(body.dnf, ";", [](std::ostream& out, const RuleBody::clause& cur) {
        out << join(cur, ",", [](std::ostream& out, const RuleBody::literal& l) {
            if (l.negated) {
                out << "!";
            }
            out << *l.atom;
        });
    });
}

bool RuleBody::equal(const literal& a, const literal& b) {
    return a.negated == b.negated && *a.atom == *b.atom;
}

bool RuleBody::equal(const clause& a, const clause& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        bool found = false;
        for (std::size_t j = 0; !found && j < b.size(); ++j) {
            if (equal(a[i], b[j])) {
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool RuleBody::isSubsetOf(const clause& a, const clause& b) {
    if (a.size() > b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        bool found = false;
        for (std::size_t j = 0; !found && j < b.size(); ++j) {
            if (equal(a[i], b[j])) {
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

void RuleBody::insert(clause& cl, literal&& lit) {
    for (const auto& cur : cl) {
        if (equal(cur, lit)) {
            return;
        }
    }
    cl.emplace_back(std::move(lit));
}

void RuleBody::insert(std::vector<clause>& cnf, clause&& cls) {
    for (const auto& cur : cnf) {
        if (isSubsetOf(cur, cls)) {
            return;
        }
    }
    std::vector<clause> res;
    for (auto& cur : cnf) {
        if (!isSubsetOf(cls, cur)) {
            res.emplace_back(std::move(cur));
        }
    }
    res.swap(cnf);
    cnf.emplace_back(std::move(cls));
}

}  // namespace ast
}  // namespace souffle
