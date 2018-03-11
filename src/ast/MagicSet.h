/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file MagicSet.h
 *
 * Define classes and functionality related to the magic set transformation.
 *
 ***********************************************************************/

#pragma once

#include "ast/Transforms.h"
#include "ast/Visitor.h"

#include <set>
#include <string>
#include <vector>

namespace souffle::ast {
class AdornedPredicate {
private:
    RelationIdentifier predicateName;
    std::string adornment;

public:
    AdornedPredicate(RelationIdentifier name, std::string adornment)
            : predicateName(name), adornment(adornment) {}

    ~AdornedPredicate() {}

    RelationIdentifier getName() const {
        return predicateName;
    }

    std::string getAdornment() const {
        return adornment;
    }

    friend std::ostream& operator<<(std::ostream& out, const AdornedPredicate& arg) {
        out << "(" << arg.predicateName << ", " << arg.adornment << ")";
        return out;
    }

    friend bool operator<(const AdornedPredicate& p1, const AdornedPredicate& p2) {
        if (p1.getName() != p2.getName()) {
            return p1.getName() < p2.getName();
        } else {
            return p1.getAdornment() < p2.getAdornment();
        }
    }
};

class AdornedClause {
private:
    Clause* clause;
    std::string headAdornment;
    std::vector<std::string> bodyAdornment;
    std::vector<unsigned int> ordering;

public:
    AdornedClause(Clause* clause, std::string headAdornment, std::vector<std::string> bodyAdornment,
            std::vector<unsigned int> ordering)
            : clause(clause), headAdornment(headAdornment), bodyAdornment(bodyAdornment), ordering(ordering) {
    }

    Clause* getClause() const {
        return clause;
    }

    std::string getHeadAdornment() const {
        return headAdornment;
    }

    std::vector<std::string> getBodyAdornment() const {
        return bodyAdornment;
    }

    std::vector<unsigned int> getOrdering() const {
        return ordering;
    }

    friend std::ostream& operator<<(std::ostream& out, const AdornedClause& arg) {
        size_t currpos = 0;
        bool firstadded = true;
        out << arg.clause->getHead()->getName() << "{" << arg.headAdornment << "} :- ";

        std::vector<Literal*> bodyLiterals = arg.clause->getBodyLiterals();
        for (Literal* lit : bodyLiterals) {
            if (dynamic_cast<Atom*>(lit) == 0) {
                const Atom* corresAtom = lit->getAtom();
                if (corresAtom != nullptr) {
                    if (firstadded) {
                        firstadded = false;
                        out << corresAtom->getName() << "{_}";
                    } else {
                        out << ", " << corresAtom->getName() << "{_}";
                    }
                } else {
                    continue;
                }
            } else {
                if (firstadded) {
                    firstadded = false;
                    out << lit->getAtom()->getName() << "{" << arg.bodyAdornment[currpos] << "}";
                } else {
                    out << ", " << lit->getAtom()->getName() << "{" << arg.bodyAdornment[currpos] << "}";
                }
                currpos++;
            }
        }
        out << ". [order: " << arg.ordering << "]";

        return out;
    }
};

class BindingStore {
private:
    std::map<std::string, std::unique_ptr<Argument>> originalArguments;
    std::map<std::string, std::set<std::string>> varDependencies;
    std::set<std::string> variableBoundComposites;

public:
    Argument* cloneOriginalArgument(const std::string& argName) const {
        return originalArguments.at(argName)->clone();
    }

    const std::set<std::string>& getVariableDependencies(const std::string& argName) const {
        return varDependencies.at(argName);
    }

    void addBinding(std::string newVariableName, const Argument* arg) {
        originalArguments[newVariableName] = std::unique_ptr<Argument>(arg->clone());

        // find the variable dependencies
        std::set<std::string> dependencies;
        visitDepthFirst(*arg, [&](const Variable& var) { dependencies.insert(var.getName()); });
        varDependencies[newVariableName] = dependencies;
    }

    void addVariableBoundComposite(std::string functorName) {
        variableBoundComposites.insert(functorName);
    }

    bool isVariableBoundComposite(std::string functorName) const {
        return (variableBoundComposites.find(functorName) != variableBoundComposites.end());
    }
};

class Adornment : public Analysis {
private:
    std::vector<std::vector<AdornedClause>> adornmentClauses;
    std::vector<RelationIdentifier> adornmentRelations;
    std::set<RelationIdentifier> adornmentEdb;
    std::set<RelationIdentifier> adornmentIdb;
    std::set<RelationIdentifier> negatedAtoms;
    std::set<RelationIdentifier> ignoredAtoms;
    BindingStore bindings;

public:
    static constexpr const char* name = "adorned-clauses";

    ~Adornment() {}

    virtual void run(const TranslationUnit& translationUnit);

    void print(std::ostream& os) const;

    const std::vector<std::vector<AdornedClause>>& getAdornedClauses() const {
        return adornmentClauses;
    }

    const std::vector<RelationIdentifier>& getRelations() const {
        return adornmentRelations;
    }

    const std::set<RelationIdentifier>& getEDB() const {
        return adornmentEdb;
    }

    const std::set<RelationIdentifier>& getIDB() const {
        return adornmentIdb;
    }

    const std::set<RelationIdentifier>& getNegatedAtoms() const {
        return negatedAtoms;
    }

    const std::set<RelationIdentifier>& getIgnoredAtoms() const {
        return ignoredAtoms;
    }

    const BindingStore& getBindings() const {
        return bindings;
    }
};
}  // namespace souffle::ast
