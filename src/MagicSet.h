#pragma once

#include "AstTransforms.h"
#include "AstTypeAnalysis.h"
#include "AstUtils.h"
#include "AstVisitor.h"
#include "PrecedenceGraph.h"

#include <string>
#include <vector>

namespace souffle {
  class AdornedPredicate {
  private:
    AstRelationIdentifier m_name;
    std::string m_adornment;
  public:
    AdornedPredicate(AstRelationIdentifier name, std::string adornment) : m_name(name), m_adornment(adornment) {}

    ~AdornedPredicate() {}

    AstRelationIdentifier getName() const {
      return m_name;
    }

    std::string getAdornment() const {
      return m_adornment;
    }

    // TODO: Check if correct
    friend std::ostream& operator<<(std::ostream& out, const AdornedPredicate& arg){
      out << "(" <<  arg.m_name << ", " << arg.m_adornment << ")";
      return out;
    }

    friend bool operator< (const AdornedPredicate& p1, const AdornedPredicate& p2){
      //TODO: NEED TO CHECK THIS
      std::stringstream comp1, comp2;
      comp1 << p1.getName() << "_ADD_" << p1.getAdornment();
      comp2 << p2.getName() << "_ADD_" << p2.getAdornment();
      return (comp1.str() < comp2.str());
    }
  };

  class AdornedClause {
  private:
    AstClause* m_clause;
    std::string m_headAdornment;
    std::vector<std::string> m_bodyAdornment;
    std::vector<unsigned int> m_ordering;

  public:
    AdornedClause(AstClause* clause, std::string headAdornment, std::vector<std::string> bodyAdornment, std::vector<unsigned int> ordering)
    : m_clause(clause), m_headAdornment(headAdornment), m_bodyAdornment(bodyAdornment), m_ordering(ordering) {}

    AstClause* getClause() const {
      return m_clause;
    }

    std::string getHeadAdornment() const {
      return m_headAdornment;
    }

    std::vector<std::string> getBodyAdornment() const {
      return m_bodyAdornment;
    }

    std::vector<unsigned int> getOrdering() const {
      return m_ordering;
    }

    friend std::ostream& operator<<(std::ostream& out, const AdornedClause& arg){
    //  std::stringstream str; str << arg.m_clause->getAtom()->getName();

      size_t currpos = 0;
      bool firstadded = true;
      out << arg.m_clause->getHead()->getName() << "{" << arg.m_headAdornment << "} :- ";

      std::vector<AstLiteral*> bodyLiterals = arg.m_clause->getBodyLiterals();
      for(size_t i = 0; i < bodyLiterals.size(); i++){
        AstLiteral* lit = bodyLiterals[i];
        if(dynamic_cast<AstAtom*>(lit) == 0){
          const AstAtom* corresAtom = lit->getAtom();
          if(corresAtom != nullptr){
            if(firstadded){
              firstadded = false;
              out << corresAtom->getName() << "{_}";
            } else {
              out << ", " << corresAtom->getName() << "{_}";
            }
          } else {
            continue;
          }
        } else {
          if(firstadded) {
            firstadded = false;
            out << lit->getAtom()->getName() << "{" << arg.m_bodyAdornment[currpos] << "}";
          } else {
            out << ", " << lit->getAtom()->getName() << "{" << arg.m_bodyAdornment[currpos] << "}";
          }
          currpos++;
        }
      }
      out << ". [order: " << arg.m_ordering << "]";

      return out;
    }
  };

  class Adornment : public AstAnalysis {
  private:
    // TODO: map instead
    std::vector<std::vector<AdornedClause>> m_adornedClauses;
    std::vector<AstRelationIdentifier> m_relations;
    std::set<AstRelationIdentifier> m_edb;
    std::set<AstRelationIdentifier> m_idb;
    std::set<AstRelationIdentifier> m_negatedAtoms;

  public:
    static constexpr const char* name = "adorned-clauses";

    ~Adornment() {}

    virtual void run(const AstTranslationUnit& translationUnit);

    void outputAdornment(std::ostream& os);

    // NOTE: why do these sometimes return a reference instead of the vector
    const std::vector<std::vector<AdornedClause>> getAdornedClauses(){
     return m_adornedClauses;
    }

    const std::vector<AstRelationIdentifier> getRelations(){
      return m_relations;
    }

    const std::set<AstRelationIdentifier> getEDB(){
      return m_edb;
    }

    const std::set<AstRelationIdentifier> getIDB(){
      return m_idb;
    }

    const std::set<AstRelationIdentifier> getNegatedAtoms(){
      return m_negatedAtoms;
    }
  };
}
