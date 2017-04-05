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

    // TODO: Check if Clause has to be reordered! (i.e. new one has to be made)
    const AstProgram* program = translationUnit.getProgram();

    //std::cout << "____________________" << std::endl;

    // set up IDB/EDB and the output queries
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
        m_edb.insert(name.str());
      } else {
        m_idb.insert(name.str());
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
        frepeat << "f"; // 'f'*(number of arguments in output query)
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
          std::vector<unsigned int> ordering (clause->getAtoms().size());
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
                // already done
                continue;
              }
              bool foundBound = false;
              name.str(""); name << *currAtom;

              // check if this is the first edb atom met
              if(firstedb < 0 && (m_edb.find(name.str()) != m_edb.end())){
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

              // bound argument found, so based on this SIPS we adorn it
              if(foundBound){
                atomAdded = true;
                std::stringstream atomAdornment;

                // find the adornment pattern
                // std::cout << "BOUNDED:" << boundedArgs << std::endl;
                for(AstArgument* arg : currAtom->getArguments()){
                  std::stringstream argName; argName << *arg;
                  if(boundedArgs.find(argName.str()) != boundedArgs.end()){
                    atomAdornment << "b"; // bounded
                    // std::cout << *currAtom << " with arg " << argName.str() << std::endl;
                  } else {
                    atomAdornment << "f"; // free
                    boundedArgs.insert(argName.str()); // now bounded
                  }
                  // std::cout << "SO FAR: " << atomAdornment.str() << std::endl;
                }

                name.str(""); name << currAtom->getName();
                bool seenBefore = false;

                // check if we've already dealt with this adornment before
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

                clauseAtomAdornments[i] = atomAdornment.str();
                ordering[i] = atomsAdorned;

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

              if(!seenBefore){
                currentPredicates.push_back(AdornedPredicate (name.str(), atomAdornment.str()));
                seenPredicates.insert(AdornedPredicate (name.str(), atomAdornment.str()));
              }

              clauseAtomAdornments[i] = atomAdornment.str();
              ordering[i] = atomsAdorned;

              atoms[i] = nullptr;
              //atoms.erase(atoms.begin() + i);
              atomsAdorned++;
            }
          }
          // std::cout << *clause << std::endl;
          // std::cout << clauseAtomAdornments << std::endl << std::endl;
          // AdornedClause finishedClause (clause, headAdornment, clauseAtomAdornments);
          adornedClauses.push_back(AdornedClause (clause, headAdornment, clauseAtomAdornments, ordering));
          //adornedClauses.push_back(AdornedClause (clause, headAdornment, clauseAtomAdornments));
        }
      }
      //std::cout << adornedClauses << std::endl;
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

  bool MagicSetTransformer::transform(AstTranslationUnit& translationUnit){
    bool changed = true; //TODO: Fix afterwards
    Adornment* adornment = translationUnit.getAnalysis<Adornment>();
    AstProgram* program = translationUnit.getProgram();
    //if(adornment->getRelations().size() != 1){
      // TODO: More than one output
      // NOTE: maybe prepend o[outputnumber]_m_[name]_[adornment]: instead
      //return false;
    //}

    // need to create new IDB - so first work with the current IDB
    // then remove old IDB, add all clauses from new IDB (S)

    // STEPS:
    // For all output relations G:
    // -- Get the adornment S for this clause
    // -- Add to S the set of magic rules for all clauses in S
    // -- For all clauses H :- T in S:
    // -- -- Replace the clause with H :- mag(H), T.
    // -- Add the fact m_G_f...f to S
    // Remove all old idb rules

    // S is the new IDB
    // adornment->getIDB() is the old IDB

    // MAJOOOOOOOOOOOOOOOOOOR TODO TODO TODO FACTS!!!! - think about this!

    std::vector<std::vector<AdornedClause>> allAdornedClauses = adornment->getAdornedClauses();
    std::vector<std::string> outputQueries = adornment->getRelations();
    std::set<std::string> oldidb = adornment->getIDB();
    std::vector<AstClause*> newClauses;

    for(size_t i = 0; i < outputQueries.size(); i++){
      std::string outputQuery = outputQueries[i];
      std::vector<AdornedClause> adornedClauses = allAdornedClauses[i];

      for(AdornedClause adornedClause : adornedClauses){
        AstClause* clause = adornedClause.getClause();
        bool output = false;

        std::string headAdornment = adornedClause.getHeadAdornment();

        std::stringstream relName;
        relName << clause->getHead()->getName();

        if(relName.str().compare(outputQuery) == 0){
          bool allFree = true;
          for(int i = 0; i < headAdornment.size(); i++){
            if(headAdornment[i]!='f'){
              allFree = false;
              break;
            }
          }
          if(allFree){
            // add as output
            output = true;
          }
        }

        relName << "_" << headAdornment;

        AstRelation* adornedRelation;

        if((adornedRelation = program->getRelation(relName.str()))==nullptr){
          std::stringstream tmp; tmp << clause->getHead()->getName();
          AstRelation* originalRelation = program->getRelation(tmp.str());

          AstRelation* newRelation = new AstRelation();
          newRelation->setName(relName.str());

          if(output){
            AstIODirective* newdir = new AstIODirective();
            newdir->setAsOutput();

            // TODO: change this eventually so that it produces the same name as the original
            newRelation->addIODirectives(std::unique_ptr<AstIODirective>(newdir)); // TODO: check this unique ptr stuff
          }

          for(AstAttribute* attr : originalRelation->getAttributes()){
            newRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
          }

          program->appendRelation(std::unique_ptr<AstRelation> (newRelation));
          adornedRelation = newRelation;
        }

        AstClause* newClause = clause->clone();
        newClause->reorderAtoms(adornedClause.getOrdering());
        newClause->getHead()->setName(relName.str());

        // add adornments to names
        std::vector<AstLiteral*> body = newClause->getBodyLiterals();
        std::vector<std::string> bodyAdornment = adornedClause.getBodyAdornment();
        int count = 0;


        // TODO: NEED TO IGNORE ADORNMENT AFTER IN DEBUG-REPORT

        for(size_t i = 0; i < body.size(); i++){
          AstLiteral* lit = body[i];
          // only IDB should be added

          if(dynamic_cast<AstAtom*>(lit)){
            std::stringstream litName; litName << lit->getAtom()->getName();
            if (oldidb.find(litName.str()) != oldidb.end()){
              litName << "_" << bodyAdornment[count];
              AstAtom* atomlit = (AstAtom*) lit; // TODO: fix
              atomlit->setName(litName.str());
              count++;
            }
          }
        }

        // TODO: add the set of magic rules


        newClauses.push_back(newClause);
        adornedRelation->addClause(std::unique_ptr<AstClause> (newClause));
      }

      // int count = 0;
      // for(AstAttribute* attr : originalRelation->getAttributes()){
      //   if(headAdornment[count] == 'b'){
      //     newRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
      //   }
      //   count++;
      // }

      //  std::string originalName = litName.str();
      //  litName.str(""); litName << "m_" << (lit->getAtom()->getName());

      // TODO: "for all clauses H:-T in S, replace with H :- mag(H), T"


    }

    // NOTE: what does std::unique_ptr do?

    // TODO: check

    // remove all old IDB relations
    for(std::string relation : oldidb){
      program->removeRelation(relation);
    }

    return changed;
  }
}
