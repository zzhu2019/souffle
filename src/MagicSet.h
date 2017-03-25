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

  /*class AdornedClause : public AstClause {
  private:
    std::string headAdornment;
    std::vector<std::string> bodyAdornment;
    AstClause* clearClause;

  public:
    const std::string getHeadAdornment(){
      return headAdornment;
    }

    const std::vector<std::string> getBodyAdornment(){
      return bodyAdornment;
    }
  };*/

  class Adornment : public AstAnalysis {
  private:
    //std::vector<AdornedClause*> adornedClauses;

  public:
    static constexpr const char* name = "adorned-clauses";

    ~Adornment() {}

    virtual void run(const AstTranslationUnit& translationUnit);

    // why returning a reference instead of the vector
    // const std::vector<AdornedClause*>& getAdornedClauses(){
    //   return adornedClauses;
    // }
  };
}
