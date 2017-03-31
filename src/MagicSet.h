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
    std::string m_name;
    std::string m_adornment;
  public:
    AdornedPredicate(std::string name, std::string adornment) : m_name(name), m_adornment(adornment) {}

    ~AdornedPredicate() {}

    std::string getName() const {
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

  public:
    AdornedClause(AstClause* clause, std::string headAdornment, std::vector<std::string> bodyAdornment)
    : m_clause(clause), m_headAdornment(headAdornment), m_bodyAdornment(bodyAdornment) {}

    AstClause* getClause() const {
      return m_clause;
    }

    std::string getHeadAdornment() const {
      return m_headAdornment;
    }

    std::vector<std::string> getBodyAdornment() const {
      return m_bodyAdornment;
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
      out << ".";

      return out;

      // TODO FIX HOW THSI PRINTS (especially m_clause)
      // std::stringstream fullClause; fullClause << *(arg.m_clause);
      // std::string fullClauseString = fullClause.str();
      // fullClauseString.erase(std::remove_if(fullClauseString.begin(), fullClauseString.end(), isspace), fullClauseString.end());
      // out << "(" <<  fullClauseString << ", " << arg.m_headAdornment << ", " << arg.m_bodyAdornment << ")";
      // return out;
    }
  };

  class Adornment : public AstAnalysis {
  private:
    // map instead
    // std::vector<AdornedClause> m_adornedClauses;
    std::vector<std::vector<AdornedClause>> m_adornedClauses;
    std::vector<std::string> m_relations;
    std::set<std::string> m_edb;
    std::set<std::string> m_idb;

  public:
    static constexpr const char* name = "adorned-clauses";

    ~Adornment() {}

    virtual void run(const AstTranslationUnit& translationUnit);

    void outputAdornment(std::ostream& os);

    // NOTE: why returning a reference instead of the vector
    const std::vector<std::vector<AdornedClause>> getAdornedClauses(){
     return m_adornedClauses;
    }
  };
}
