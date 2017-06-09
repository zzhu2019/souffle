#pragma once

#include "AstTransforms.h"

#include <string>
#include <vector>

namespace souffle {
class AdornedPredicate {
private:
    AstRelationIdentifier predicateName;
    std::string adornment;

public:
    AdornedPredicate(AstRelationIdentifier name, std::string adornment)
            : predicateName(name), adornment(adornment) {}

    ~AdornedPredicate() {}

    AstRelationIdentifier getName() const {
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
        // TODO: think of a better way to compare adorned predicates
        std::stringstream comp1, comp2;
        comp1 << p1.getName() << "+ _ADD_ +" << p1.getAdornment();
        comp2 << p2.getName() << "+ _ADD_ +" << p2.getAdornment();
        return (comp1.str() < comp2.str());
    }
};

class AdornedClause {
private:
    AstClause* clause;
    std::string headAdornment;
    std::vector<std::string> bodyAdornment;
    std::vector<unsigned int> ordering;

public:
    AdornedClause(AstClause* clause, std::string headAdornment, std::vector<std::string> bodyAdornment,
            std::vector<unsigned int> ordering)
            : clause(clause), headAdornment(headAdornment), bodyAdornment(bodyAdornment), ordering(ordering) {
    }

    AstClause* getClause() const {
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

        std::vector<AstLiteral*> bodyLiterals = arg.clause->getBodyLiterals();
        for (AstLiteral* lit : bodyLiterals) {
            if (dynamic_cast<AstAtom*>(lit) == 0) {
                const AstAtom* corresAtom = lit->getAtom();
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

class Adornment : public AstAnalysis {
private:
    // TODO: maybe use a map instead to clean things up
    std::vector<std::vector<AdornedClause>> adornmentClauses;
    std::vector<AstRelationIdentifier> adornmentRelations;
    std::set<AstRelationIdentifier> adornmentEdb;
    std::set<AstRelationIdentifier> adornmentIdb;
    std::set<AstRelationIdentifier> negatedAtoms;
    std::set<AstRelationIdentifier> ignoredAtoms;

public:
    static constexpr const char* name = "adorned-clauses";

    ~Adornment() {}

    virtual void run(const AstTranslationUnit& translationUnit);

    void outputAdornment(std::ostream& os);

    const std::vector<std::vector<AdornedClause>> getAdornedClauses() {
        return adornmentClauses;
    }

    const std::vector<AstRelationIdentifier> getRelations() {
        return adornmentRelations;
    }

    const std::set<AstRelationIdentifier> getEDB() {
        return adornmentEdb;
    }

    const std::set<AstRelationIdentifier> getIDB() {
        return adornmentIdb;
    }

    const std::set<AstRelationIdentifier> getNegatedAtoms() {
        return negatedAtoms;
    }

    const std::set<AstRelationIdentifier> getIgnoredAtoms() {
        return ignoredAtoms;
    }
};
}
