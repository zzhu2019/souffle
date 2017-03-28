#include "AstTransforms.h"
#include "AstTypeAnalysis.h"
#include "AstUtils.h"
#include "AstVisitor.h"
#include "PrecedenceGraph.h"
#include "MagicSet.h"

#include <string>
#include <vector>

namespace souffle {
  void Adornment::run(const AstTranslationUnit& translationUnit){
    // Every adorned clause is of the form R^c :- a^c, b^c, etc.
    // Copy over constraints directly (including negated)

    // steps

    // Let P be the set of all adorned predicates (initially empty)
    // Let D' be the set of all adorned clauses (initially empty)
    // Let S be the set of all seen predicate adornments

    // Get the program
    // Get the query
    // Adorn the query based on boundedness, and add it to P and S
    // While P is not empty
    // -- Get the first atom out, call it R^c (remove from P)
    // -- For every clause Q defining R (i.e. all related clauses in EDB):
    // -- -- Adorn Q using R^c
    // -- -- Add the adorned clause to D'
    // -- -- If the body of the adorned clause contains an
    //        unseen predicate adornment, add it to S and P

    // Output: D' [the set of all adorned clauses]

    const AstProgram* program = translationUnit.getProgram();

    // set up IDB/EDB and the output query
    std::set<std::string> edb;
    std::set<std::string> idb;
    // std::string outputQuery;
    std::stringstream frepeat; // 'f'*(number of arguments in output query)

    std::vector<std::string> outputQueries;

    std::vector<std::vector<AdornedClause>> adornedProgram;

    for(AstRelation* rel : program->getRelations()){

      std::stringstream name; name << rel->getName(); // TODO: check if correct?
      // check if output relation
      if(rel->isOutput()){
        // store the output name
        outputQueries.push_back(name.str());
        m_relations.push_back(name.str());
      }

      // store into edb or idb
      bool is_edb = true;
      for(AstClause* clause : rel->getClauses()){
        if(!clause->isFact()){
          is_edb = false; // TODO: check if correct
          break;
        }
      }

      if(is_edb){
        edb.insert(name.str());
      } else {
        idb.insert(name.str());
      }
    }

    for(std::string outputQuery : outputQueries){
      // adornment algorithm
      std::vector<AdornedPredicate> currentPredicates;
      std::set<AdornedPredicate> seenPredicates;
      std::vector<AdornedClause> adornedClauses;

      std::stringstream frepeat;
      size_t arity = program->getRelation(outputQuery)->getArity();
      for(size_t i = 0; i < arity; i++){
        frepeat << "f";
      }

      AdornedPredicate outputPredicate (outputQuery, frepeat.str());
      currentPredicates.push_back(outputPredicate);
      seenPredicates.insert(outputPredicate);

      while(!currentPredicates.empty()){
        // pop out the first element
        AdornedPredicate currPredicate = currentPredicates[0];
        currentPredicates.erase(currentPredicates.begin());

        // std::cout << "Adorning with respect to " << currPredicate << "..." << std::endl;

        // go through all clauses defining it
        AstRelation* rel = program->getRelation(currPredicate.getName());
        for(AstClause* clause : rel->getClauses()){

          if(clause->isFact()){
            continue;
          }

          // TODO: check if ordering correct, and if this is correct C++ vectoring
          std::vector<std::string> clauseAtomAdornments (clause->getAtoms().size());
          std::stringstream name;

          std::set<std::string> boundedArgs;

          // mark all bounded arguments from head adornment
          AstAtom* clauseHead = clause->getHead();
          std::string headAdornment = currPredicate.getAdornment();
          std::vector<AstArgument*> headArguments = clauseHead->getArguments();
          for(size_t argnum = 0; argnum < headArguments.size(); argnum++){
            if(headAdornment[argnum] == 'b'){
              AstArgument* currArg = headArguments[argnum];
              name.str(""); name << *currArg;
              boundedArgs.insert(name.str());
            }
          }

          // mark all bounded arguments from the body
          for(AstConstraint* constraint : clause->getConstraints()){
            name.str(""); name << *constraint->getLHS();
            boundedArgs.insert(name.str());
          }

          std::vector<AstAtom*> atoms = clause->getAtoms();
          int atomsAdorned = 0;
          int atomsTotal = atoms.size();

          while(atomsAdorned < atomsTotal){
            //std::cout << "P: " << currentPredicates << ", Seen: " << seenPredicates << std::endl;
            int firstedb = -1; // index of first edb atom
            bool atomAdded = false;

            for(size_t i = 0; i < atoms.size(); i++){
              AstAtom* currAtom = atoms[i];
              if(currAtom == nullptr){
                continue;
              }
              bool foundBound = false;
              name.str(""); name << *currAtom;

              // check if this is the first edb atom met
              if(firstedb < 0 && (edb.find(name.str()) != edb.end())){
                firstedb = i;
              }

              // check if any of the atom's arguments are bounded
              for(AstArgument* arg : currAtom->getArguments()){
                name.str(""); name << *arg;
                // check if this argument has been bounded
                if(boundedArgs.find(name.str()) != boundedArgs.end()){
                  foundBound = true;
                  break; // we found a bound argument, so we can adorn this
                }
              }

              if(foundBound){
                atomAdded = true;
                std::stringstream atomAdornment;

                // find the adornment pattern
                for(AstArgument* arg : currAtom->getArguments()){
                  std::stringstream argName; argName << *arg;
                  if(boundedArgs.find(argName.str()) != boundedArgs.end()){
                    atomAdornment << "b"; // bounded
                  } else {
                    atomAdornment << "f"; // free
                    boundedArgs.insert(argName.str()); // now bounded
                  }
                }

                name.str(""); name << currAtom->getName();
                bool seenBefore = false;
                for(AdornedPredicate seenPred : seenPredicates){
                  if( (seenPred.getName().compare(name.str()) == 0)
                      && (seenPred.getAdornment().compare(atomAdornment.str()) == 0)){ // TODO: check if correct/better way to do
                        seenBefore = true;
                        break;
                  }
                }

                if(!seenBefore){
                  currentPredicates.push_back(AdornedPredicate (name.str(), atomAdornment.str()));
                  seenPredicates.insert(AdornedPredicate (name.str(), atomAdornment.str()));
                }

                clauseAtomAdornments[i]=atomAdornment.str();

                atoms[i] = nullptr;
                //atoms.erase(atoms.begin() + i);
                atomsAdorned++;
                break;
              }
            }

            if(!atomAdded){
              size_t i = 0;
              if(firstedb >= 0){
                i = firstedb;
              } else {
                for(i = 0; i < atoms.size(); i++){
                  if(atoms[i] != nullptr){
                    break;
                  }
                }
              }

              // TODO: get rid of repetitive code
              std::stringstream atomAdornment;
              AstAtom* currAtom = atoms[i];
              name.str(""); name << currAtom->getName();

              for(AstArgument* arg : currAtom->getArguments()){
                std::stringstream argName; argName << *arg;
                if(boundedArgs.find(argName.str()) != boundedArgs.end()){
                  atomAdornment << "b"; // bounded
                } else {
                  atomAdornment << "f"; // free
                  boundedArgs.insert(argName.str()); // now bounded
                }
              }
              bool seenBefore = false;
              for(AdornedPredicate seenPred : seenPredicates){
                if( (seenPred.getName().compare(name.str()) == 0)
                    && (seenPred.getAdornment().compare(atomAdornment.str()) == 0)){ // TODO: check if correct/better way to do
                      seenBefore = true;
                      break;
                }
              }

              // TODO: FIX THIS. NOT NEEDED FOR EDB.
              if(!seenBefore){
                currentPredicates.push_back(AdornedPredicate (name.str(), atomAdornment.str()));
                seenPredicates.insert(AdornedPredicate (name.str(), atomAdornment.str()));
              }

              clauseAtomAdornments[i] = atomAdornment.str();

              atoms[i] = nullptr;
              //atoms.erase(atoms.begin() + i);
              atomsAdorned++;
            }
          }
          //std::cout << *clause << std::endl;
          //std::cout << clauseAtomAdornments << std::endl << std::endl;
          // AdornedClause finishedClause (clause, headAdornment, clauseAtomAdornments);
          adornedClauses.push_back(AdornedClause (clause, headAdornment, clauseAtomAdornments));
          //adornedClauses.push_back(AdornedClause (clause, headAdornment, clauseAtomAdornments));
        }
      }
      // std::cout << adornedClauses << std::endl;
      m_adornedClauses.push_back(adornedClauses);
    }
  }

  void Adornment::outputAdornment(std::ostream& os){
    // TODO: FIX HOW THIS PRINTS
    for(size_t i = 0; i < m_adornedClauses.size(); i++){
      std::vector<AdornedClause> clauses = m_adornedClauses[i];
      os << "Output " << i+1 << ": " << m_relations[i] << std::endl;
      for(AdornedClause clause : clauses){
        os << clause << std::endl;
      }
      os << std::endl;
    }
  }
}
