  #include "AstTransforms.h"
#include "AstTypeAnalysis.h"
#include "AstUtils.h"
#include "AstVisitor.h"
#include "PrecedenceGraph.h"
#include "MagicSet.h"
#include "IODirectives.h"

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

    // set up IDB/EDB and the output queries
    std::vector<AstRelationIdentifier> outputQueries;

    std::vector<std::vector<AdornedClause>> adornedProgram;

    for(AstRelation* rel : program->getRelations()){

      AstRelationIdentifier relName = rel->getName();

      // check if output relation or size is printed
      if(rel->isComputed()){
        outputQueries.push_back(rel->getName());
        m_relations.push_back(rel->getName());
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
        m_edb.insert(relName);
      } else {
        m_idb.insert(relName);
      }
    }

    // TODO: DECIDE IF YOU SHOULD STORE RELATION POINTERS OR ASTRELATIONIDENTIFIERS

    /*----TODO: move this later -------*/
    /* GET LIST OF NEGATED LITERALS */
    std::set<AstRelationIdentifier> negatedLiterals;
    for(AstRelation* rel : program->getRelations()){
      std::vector<AstClause*> clauses = rel->getClauses();
      for(size_t clauseNum = 0; clauseNum < clauses.size(); clauseNum++){
        AstClause* clause = clauses[clauseNum];
        for(AstLiteral* lit : clause->getBodyLiterals()){
          if(dynamic_cast<AstNegation*>(lit)){
            AstRelationIdentifier negatedName = lit->getAtom()->getName(); // check this
            negatedLiterals.insert(negatedName);
          }
        }
      }
    }


    // TODO: ADD THIS!!!!! TO KEEP ALL DEPENDENCIES

    // TODO: CEHCK SETOPS TEST WIHTOUT THIS - AND ALL TESTS REALLY
    // std::set<std::string> newlynegated;
    // for(std::string negatom : negatedliterals){
    //   newlynegated.insert(negatom);
    // }
    //
    //
    // while(newlynegated.size()!=0){
    //   std::set<std::string> xnewnegated;
    //   for(std::string negatom : newlynegated){
    //     for(AstClause* clause : program->getRelation(negatom)->getClauses()){
    //       for(AstAtom* atom : clause->getAtoms()){
    //         std::stringstream newatomname; newatomname.str("");
    //         newatomname << (atom->getName());
    //         if(negatedliterals.find(newatomname.str()) == negatedliterals.end()){
    //           negatedliterals.insert(newatomname.str());
    //           xnewnegated.insert(newatomname.str());
    //         }
    //       }
    //     }
    //   }
    //   newlynegated = std::set<std::string> (xnewnegated);
    // }
    //
    // m_negatedAtoms = negatedliterals;


    // TODO: only keeping atoms - need to check if more should be kept
    for(AstRelationIdentifier negatedName : negatedLiterals){
      for(AstClause* clause : program->getRelation(negatedName)->getClauses()){
        for(AstAtom* atom : clause->getAtoms()){
          negatedLiterals.insert(atom->getName());
        }
      }
    }

    m_negatedAtoms = negatedLiterals;

    /*------------------------------*/



    for(size_t querynum = 0; querynum < outputQueries.size(); querynum++){
      AstRelationIdentifier outputQuery = outputQueries[querynum];
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

        // go through all clauses defining it
        AstRelation* rel = program->getRelation(currPredicate.getName()); // PROBLEM!!! with dfa.dl and co.

        if(rel == nullptr){
            continue; // TODO: THIS STUFFS UP FOR DFA.DL BECAUSE OF XYZ.out AS PREDICATES!!! what do htey do!?!?! check!
        }

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
            AstArgument* lhs = constraint->getLHS();
            AstArgument* rhs = constraint->getRHS();
            if(dynamic_cast<AstConstraint*>(lhs)){
              name.str(""); name << *lhs;
              boundedArgs.insert(name.str());
            }
            if(dynamic_cast<AstConstraint*>(rhs)){
              name.str(""); name << *rhs;
              boundedArgs.insert(name.str());
            }
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
                std::set<std::string> newlyBoundedArgs;

                // std::cout << "BOUNDED:" << boundedArgs << std::endl;
                for(AstArgument* arg : currAtom->getArguments()){
                  std::stringstream argName; argName << *arg;
                  if(boundedArgs.find(argName.str()) != boundedArgs.end()){
                    atomAdornment << "b"; // bounded
                    // std::cout << *currAtom << " with arg " << argName.str() << std::endl;
                  } else {
                    atomAdornment << "f"; // free
                    newlyBoundedArgs.insert(argName.str()); // now bounded
                  }
                  // std::cout << "SO FAR: " << atomAdornment.str() << std::endl;
                }

                for(std::string argx : newlyBoundedArgs){
                  boundedArgs.insert(argx);
                }

                AstRelationIdentifier atomName = currAtom->getName();
                // name.str(""); name << currAtom->getName();
                bool seenBefore = false;

                // check if we've already dealt with this adornment before
                for(AdornedPredicate seenPred : seenPredicates){
                  if( (seenPred.getName() == atomName)
                      && (seenPred.getAdornment().compare(atomAdornment.str()) == 0)){ // TODO: check if correct/better way to do
                        seenBefore = true;
                        break;
                  }
                }

                if(!seenBefore){
                  currentPredicates.push_back(AdornedPredicate (atomName, atomAdornment.str()));
                  seenPredicates.insert(AdornedPredicate (atomName, atomAdornment.str()));
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
              AstRelationIdentifier atomName = currAtom->getName();

              std::set<std::string> newlyBoundedArgs;

              for(AstArgument* arg : currAtom->getArguments()){
                std::stringstream argName; argName << *arg;
                if(boundedArgs.find(argName.str()) != boundedArgs.end()){
                  atomAdornment << "b"; // bounded
                } else {
                  atomAdornment << "f"; // free
                  newlyBoundedArgs.insert(argName.str()); // now bounded
                }
              }

              for(std::string argx : newlyBoundedArgs){
                boundedArgs.insert(argx);
              }

              bool seenBefore = false;
              for(AdornedPredicate seenPred : seenPredicates){
                if( (seenPred.getName() == atomName)
                    && (seenPred.getAdornment().compare(atomAdornment.str()) == 0)){ // TODO: check if correct/better way to do
                      seenBefore = true;
                      break;
                }
              }

              if(!seenBefore){
                currentPredicates.push_back(AdornedPredicate (atomName, atomAdornment.str()));
                seenPredicates.insert(AdornedPredicate (atomName, atomAdornment.str()));
              }

              clauseAtomAdornments[i] = atomAdornment.str();
              ordering[i] = atomsAdorned;

              atoms[i] = nullptr;
              //atoms.erase(atoms.begin() + i);
              atomsAdorned++;
            }
          }
          adornedClauses.push_back(AdornedClause (clause, headAdornment, clauseAtomAdornments, ordering));
        }
      }
      m_adornedClauses.push_back(adornedClauses);
    }
  }

  void Adornment::outputAdornment(std::ostream& os){
    for(size_t i = 0; i < m_adornedClauses.size(); i++){
      std::vector<AdornedClause> clauses = m_adornedClauses[i];
      os << "Output " << i+1 << ": " << m_relations[i] << std::endl;
      for(AdornedClause clause : clauses){
        os << clause << std::endl;
      }
      os << std::endl;
    }
  }

  // AstRelation* backupGetRelation(AstProgram* program, std::string relname){
  //   AstRelation* relation = program->getRelation(relname);
  //   if(relation == nullptr){
  //     return nullptr;
  //     // for(AstRelation* rel : program->getRelations()){
  //     //   std::stringstream check; check.str("");
  //     //   check << rel->getName();
  //     //   if(check.str().compare(relname) == 0){
  //     //     return rel;
  //     //   }
  //     // }
  //   } else {
  //     return relation;
  //   }
  //   // std::cout << "ultimate fail " << relname << std::endl;
  //   return nullptr;
  // }

  std::vector<unsigned int> reorderOrdering(std::vector<unsigned int> order){
    std::vector<unsigned int> neworder (order.size());
    for(size_t i = 0; i < order.size(); i++){
      neworder[order[i]] = i;
    }
    return neworder;
  }

  std::vector<std::string> reorderAdornment(std::vector<std::string> adornment, std::vector<unsigned int> order){
    std::vector<std::string> result (adornment.size());
  //  std::cout << "ORIGINAL: " << adornment << " " << order << std::endl;
    for(size_t i = 0; i < adornment.size(); i++){
      result[order[i]] = adornment[i];
    }
  //  std::cout << "FINAL: " << result << std::endl;
    return result;
  }

  void DEBUGprintProgram(std::unique_ptr<AstProgram> program){
      std::cout << *program << std::endl;
  }

  AstRelationIdentifier createMagicIdentifier(AstRelationIdentifier relationName, size_t outputNumber){
    std::vector<std::string> relationNames = relationName.getNames();
    std::stringstream newMainName; newMainName.str("");
    newMainName << "m" << outputNumber << "_" << relationNames[0]; //<< "_" << adornment; // MAJOR TODO TODO: SHOULD BE AN UNDERSCORE HERE!!!!!!!!!!!!!!!!!!!!!!!!!!
    AstRelationIdentifier newRelationName(newMainName.str()); // TODO: Check valid [0]
    for(size_t i = 1; i < relationNames.size(); i++){
      newRelationName.append(relationNames[i]);
    }
    return newRelationName;
  }

  AstRelationIdentifier createAdornedIdentifier(AstRelationIdentifier relationName, std::string adornment){
    std::vector<std::string> relationNames = relationName.getNames();
    std::stringstream newMainName; newMainName.str("");
    newMainName << relationNames[0] << "_" << adornment;
    AstRelationIdentifier newRelationName(newMainName.str()); // TODO: Check valid [0]
    for(size_t i = 1; i < relationNames.size(); i++){
      newRelationName.append(relationNames[i]);
    }
    return newRelationName;
  }

  AstRelationIdentifier createSubIdentifier(AstRelationIdentifier relationName, size_t start, size_t endpt){
    std::vector<std::string> relationNames = relationName.getNames();
    std::stringstream newMainName; newMainName.str("");
    newMainName << relationNames[0].substr(start, endpt);
    AstRelationIdentifier newRelationName(newMainName.str()); // TODO: Check valid [0]
    for(size_t i = 1; i < relationNames.size(); i++){
      newRelationName.append(relationNames[i]);
    }
    return newRelationName;
  }

  bool MagicSetTransformer::transform(AstTranslationUnit& translationUnit){
    AstProgram* program = translationUnit.getProgram();

    // make EDB and IDB independent
    int edbNum = 0;
    for(AstRelation* rel : program->getRelations()){
      AstRelationIdentifier relName = rel->getName();

      bool is_edb = false;
      bool is_idb = false;
      for(AstClause* clause : rel->getClauses()){
        if(clause->isFact()){
          is_edb = true;
        } else {
          is_idb = true;
        }
        if(is_edb && is_idb){
          break;
        }
      }

      if(is_edb && is_idb){
        AstRelation* newedbrel = new AstRelation ();
        for (AstAttribute* attr : rel->getAttributes()){
          newedbrel->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
        }
        std::stringstream newedbrelname;
        do {
          edbNum++;
          newedbrelname << "newedb" << edbNum;
        } while (program->getRelation(newedbrelname.str())!=nullptr);
        newedbrel->setName(newedbrelname.str());
        program->appendRelation(std::unique_ptr<AstRelation> (newedbrel));
        for(AstClause* clause : rel->getClauses()){
          if(clause->isFact()){
            AstClause* newedbclause = clause->clone();
            newedbclause->getHead()->setName(newedbrelname.str());
            program->appendClause(std::unique_ptr<AstClause> (newedbclause));
          }
        }
        AstClause* newidbclause = new AstClause ();

        // oldname(arg1...argn) :- newname(arg1...argn)
        AstAtom* headatom = new AstAtom (relName);
        AstAtom* bodyatom = new AstAtom (newedbrelname.str());

        size_t numargs = rel->getArity();
        for(size_t j = 0; j < numargs; j++){
          std::stringstream argname; argname.str("");
          argname << "arg" << j;
          headatom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
          bodyatom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
        }

        newidbclause->setHead(std::unique_ptr<AstAtom> (headatom));
        newidbclause->addToBody(std::unique_ptr<AstAtom> (bodyatom));

        program->appendClause(std::unique_ptr<AstClause> (newidbclause));
      }

    }
    // analysis breaks for dfa.dl - get rid of the 'continue' flag and check the failures again
    Adornment* adornment = translationUnit.getAnalysis<Adornment>();
    //adornment->outputAdornment(std::cout);

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

    // MAJOR TODO TODO TODO FACTS!!!! - think about this!

    std::vector<std::vector<AdornedClause>> allAdornedClauses = adornment->getAdornedClauses();
    std::vector<AstRelationIdentifier> outputQueries = adornment->getRelations();
    std::set<AstRelationIdentifier> addAsOutput;
    std::set<AstRelationIdentifier> addAsPrintSize;
    std::vector<AstRelationIdentifier> newQueryNames;
    std::set<AstRelationIdentifier> oldidb = adornment->getIDB();
    std::set<AstRelationIdentifier> newidb;
    std::vector<AstClause*> newClauses;
    std::set<AstRelationIdentifier> negatedAtoms = adornment->getNegatedAtoms();

    for(size_t querynum = 0; querynum < outputQueries.size(); querynum++){
      AstRelationIdentifier outputQuery = outputQueries[querynum];
      std::vector<AdornedClause> adornedClauses = allAdornedClauses[querynum];

      // add a relation for the output query
      AstRelation* outputRelationFree = new AstRelation();
      size_t num_free = program->getRelation(outputQuery)->getArity();
      std::stringstream thefs; thefs.str(""); //thefs << "_";
      for(size_t i = 0; i < num_free; i++){
        thefs << "f";
      }
      // AstRelation* olderRelation = program->getRelation(outputQuery);
      AstRelationIdentifier relNameY = createMagicIdentifier(createAdornedIdentifier(outputQuery, thefs.str()), querynum);
      outputRelationFree->setName(relNameY);
      newQueryNames.push_back(relNameY);
      program->appendRelation(std::unique_ptr<AstRelation> (outputRelationFree));
      AstAtom* newAtomClauseThing = new AstAtom(relNameY);
      AstClause* newAtomClauseThing2 = new AstClause();
      newAtomClauseThing2->setHead(std::unique_ptr<AstAtom> (newAtomClauseThing));
      program->appendClause(std::unique_ptr<AstClause> (newAtomClauseThing2));
      // std::cout << *olderRelation << std::endl;
      // if(olderRelation->isInput()){
      //   for(const AstIODirective* current : olderRelation->getIODirectives()){
      //     std::cout << *current << std::endl;
      //   }
      // }

      for(AdornedClause adornedClause : adornedClauses){
        AstClause* clause = adornedClause.getClause();
        std::string headAdornment = adornedClause.getHeadAdornment();

        // TODO: maybe merge createAdornedIdentifier and createMagicIdentifier
        AstRelationIdentifier origName = clause->getHead()->getName();
        AstRelationIdentifier newRelName = createAdornedIdentifier(origName, headAdornment);
        // std::stringstream relName;
        // relName << clause->getHead()->getName();
        //
        // relName << "_" << headAdornment;

        AstRelation* adornedRelation;

        if((adornedRelation = program->getRelation(newRelName))==nullptr){
          // tmpName = origName, relName = newRelName
          AstRelation* originalRelation = program->getRelation(origName);

          AstRelation* newRelation = new AstRelation();
          newRelation->setName(newRelName);

          for(AstAttribute* attr : originalRelation->getAttributes()){
            newRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
          }
          // TODO: CHECK IF NEEDED ELSEWHERE (espec for output relations)
          // TODO: fix the setup v messy
          if(originalRelation->isInput()){
            IODirectives inputDirectives;
            AstIODirective* newDirective = new AstIODirective();
            inputDirectives.setRelationName(newRelName.getNames()[0]); // TODO: CHECK IF FIRSTN AME
            newDirective->setAsInput();
            for(AstIODirective* current : originalRelation->getIODirectives()){
              if(current->isInput()){
                for(const auto& currentPair : current->getIODirectiveMap()){
                  newDirective->addKVP(currentPair.first, currentPair.second);
                  inputDirectives.set(currentPair.first, currentPair.second);
                }
              }
            }
            if(!inputDirectives.has("IO")){
              inputDirectives.setIOType("file");
              newDirective->addKVP("IO", "file");
            }
            if(inputDirectives.getIOType()=="file" && !inputDirectives.has("filename")){
              inputDirectives.setFileName(origName.getNames()[0] + ".facts"); // TODO: CHECK IF FIRSTN AME
              newDirective->addKVP("filename", origName.getNames()[0] + ".facts");
            }
            newRelation->addIODirectives(std::unique_ptr<AstIODirective>(newDirective));
          }

          program->appendRelation(std::unique_ptr<AstRelation> (newRelation));
          adornedRelation = newRelation;
        }

        AstClause* newClause = clause->clone();
        //std::cout << "BEOFRE: " << adornedClause.getOrdering() << " " << *newClause << " " <<  adornedClause.getBodyAdornment() << std::endl;
        newClause->reorderAtoms(reorderOrdering(adornedClause.getOrdering()));
        //std::cout << "AFTER: " << *newClause << " " <<  adornedClause.getBodyAdornment() << std::endl;

        // for (AstAtom* atom : newClause->getAtoms()){
        //   std::cout << *atom;
        // }
        // std::cout << std::endl;
        newClause->getHead()->setName(newRelName);


        // add adornments to names
        std::vector<AstLiteral*> body = newClause->getBodyLiterals();
        std::vector<std::string> bodyAdornment = adornedClause.getBodyAdornment();

        std::vector<unsigned int> adordering = adornedClause.getOrdering();

        //std::cout << relName.str() << " " << bodyAdornment << std::endl;
        bodyAdornment = reorderAdornment(bodyAdornment, adordering);
        //std::cout << relName.str() << " " << bodyAdornment << std::endl;
        //std::cout << "AFTER: " << *newClause << " " <<  bodyAdornment << std::endl;

        int count = 0;

        // TODO: NEED TO IGNORE ADORNMENT AFTER IN DEBUG-REPORT

        for(size_t i = 0; i < body.size(); i++){
          AstLiteral* lit = body[i];
          // only IDB should be added

          if(dynamic_cast<AstAtom*>(lit)){
            AstRelationIdentifier litName = lit->getAtom()->getName();
            // std::stringstream litName; litName << lit->getAtom()->getName();
            if (oldidb.find(litName) != oldidb.end()){
              AstRelationIdentifier newLitName = createAdornedIdentifier(litName, bodyAdornment[count]);
              AstAtom* atomlit = (AstAtom*) lit; // TODO: fix
              atomlit->setName(newLitName);
              newidb.insert(newLitName);
            }
            count++;
          }
        }

        // [[[todo: function outside this to check if IDB]]]
        // Add the set of magic rules
        for(size_t i = 0; i < body.size(); i++){
          AstLiteral* currentLiteral = body[i];
          count = 0;
          if(dynamic_cast<AstAtom*>(currentLiteral)){
            AstAtom* lit = (AstAtom*) currentLiteral;
            AstRelationIdentifier litName = lit->getAtom()->getName();

            if (newidb.find(litName) != newidb.end()){

              // AstClause* magicClause = newClause->clone();
              AstRelationIdentifier newLitName = createMagicIdentifier(litName, querynum);
              // std::stringstream newLit; newLit << "m" << querynum << "_" << lit->getAtom()->getName();
              if(program->getRelation(newLitName) == nullptr){

                AstRelation* magicRelation = new AstRelation();
                magicRelation->setName(newLitName);

                // put this in a function
                std::string mainLitName = litName.getNames()[0];
                int endpt = mainLitName.size()-1;
                while(endpt >= 0 && mainLitName[endpt] != '_'){
                  endpt--;
                }
                if(endpt == -1){
                  endpt = mainLitName.size();
                }

                AstRelationIdentifier originalRelationName = createSubIdentifier(litName, 0, endpt);
                AstRelation* originalRelation = program->getRelation(originalRelationName);

                std::string currAdornment = bodyAdornment[i];
                int argcount = 0;

                for(AstAttribute* attr : originalRelation->getAttributes()){
                  if(currAdornment[argcount] == 'b'){
                    magicRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
                  }
                  argcount++;
                }
                program->appendRelation(std::unique_ptr<AstRelation> (magicRelation));
              }

              AstClause* magicClause = new AstClause ();
              AstAtom* mclauseHead = new AstAtom (newLitName);

              std::string currAdornment = bodyAdornment[i];

              int argCount = 0;

              for(AstArgument* arg : lit->getArguments()){
                if(currAdornment[argCount] == 'b'){
                  //std::cout << "ARG: " << newLit.str() << " " << *arg << std::endl;
                  mclauseHead->addArgument(std::unique_ptr<AstArgument> (arg->clone()));
                }
                argCount++;
              }

              magicClause->setHead(std::unique_ptr<AstAtom> (mclauseHead));

              // make the body
              // TODO: CHECK WHAT THIS PART DOES AND WHY - not working
              // TODO: FIX FROM THIS POINT!!
              AstRelationIdentifier magPredName = createMagicIdentifier(newClause->getHead()->getName(), querynum);
              std::string mainLitName = magPredName.getNames()[0];
              //std::stringstream magPredName;
              //magPredName << "m" << querynum << "_" << newClause->getHead()->getName();
              int endpt = mainLitName.size()-1;
              while(endpt >= 0 && mainLitName[endpt] != '_'){
                endpt--;
              }
              if(endpt == -1){
                endpt = mainLitName.size();
              }

              // changed stuff here ...
              std::string curradorn = mainLitName.substr(endpt+1, mainLitName.size() - (endpt + 1));

              if(program->getRelation(magPredName) == nullptr){
                AstRelation* freeRelation = new AstRelation();
                freeRelation->setName(magPredName);
                AstRelation* currrel =  program->getRelation(newClause->getHead()->getName());
                std::vector<AstAttribute*> attrs = currrel->getAttributes();
                for(size_t currarg = 0; currarg < currrel->getArity(); currarg++){
                  if(curradorn[currarg] == 'b'){
                    freeRelation->addAttribute(std::unique_ptr<AstAttribute> (attrs[currarg]->clone()));
                  }
                }
                program->appendRelation(std::unique_ptr<AstRelation>(freeRelation));
              }
              AstAtom* addedMagPred = new AstAtom(magPredName);
              argCount = 0;
              for(AstArgument* arg : newClause->getHead()->getArguments()){
                if(headAdornment[argCount] == 'b'){
                  //std::cout << "PART 2 YES: " << newLit.str() << " " << *arg << " " << *newClause->getHead() << std::endl;
                  addedMagPred->addArgument(std::unique_ptr<AstArgument> (arg->clone()));
                } else {
                //  std::cout << "PART 2 FAILED: " << newLit.str() << " " << *arg << " " << *newClause->getHead() << std::endl;
                }
                argCount++;
              }

              magicClause->addToBody(std::unique_ptr<AstAtom> (addedMagPred));
              for(size_t j = 0; j < i; j++){
                magicClause->addToBody(std::unique_ptr<AstLiteral> (body[j]->clone()));
              }

              // std::cout << *magicClause << std::endl;
              // std::stringstream tmpvarx; tmpvarx << magicClause->getBodyLiteral(0)->getAtom()->getName();

              std::vector<AstArgument*> currArguments = magicClause->getHead()->getArguments();
              for(size_t i = 0; i < currArguments.size(); i++){
                AstArgument* arg = currArguments[i];
                std::stringstream tmpvarx; tmpvarx << *arg;
                if(tmpvarx.str().substr(0, 5).compare("abdul")==0){
                  size_t pos;
                  for(pos = 0; pos < tmpvarx.str().size(); pos++){
                    if(tmpvarx.str()[pos] == '_'){
                      break;
                    }
                  }

                  size_t nextpos;
                  for(nextpos = pos+1; nextpos < tmpvarx.str().size(); nextpos++){
                    if(tmpvarx.str()[nextpos] == '_' ){
                      break;
                    }
                  }
                  std::string startstr = tmpvarx.str().substr(pos+1, nextpos-pos-1);

                  std::string res = tmpvarx.str().substr(pos+1, tmpvarx.str().size());
                  // 1 2 ... pos pos+1 ... size()-3 size()-2 size()-1
                  // const char * str = res.substr(1,res.size()-2).c_str();
                  //check if string or num constant

                  if(res[res.size()-1] == 's'){
                    // std::cout << "STRING " << res << std::endl;
                    const char * str = res.substr(0,res.size()-2).c_str();
                    magicClause->addToBody(std::unique_ptr<AstLiteral>(new AstConstraint(BinaryConstraintOp::EQ,
                          std::unique_ptr<AstArgument>(arg->clone()), std::unique_ptr<AstArgument>(new AstStringConstant(translationUnit.getSymbolTable(), str)))));
                  } else {
                    //std::cout << "INT " <<  startstr << " - " << pos << " - " << tmpvarx.str() << " - " << *magicClause << std::endl;
                    magicClause->addToBody(std::unique_ptr<AstLiteral>(new AstConstraint(BinaryConstraintOp::EQ,
                          std::unique_ptr<AstArgument>(arg->clone()), std::unique_ptr<AstArgument>(new AstNumberConstant(stoi(startstr))))));
                  }
                }
              }
              program->appendClause(std::unique_ptr<AstClause> (magicClause));
            }
          }
        }

        // replace with H :- mag(H), T
        size_t numAtoms = newClause->getAtoms().size();
        AstRelationIdentifier newMag = createMagicIdentifier(newClause->getHead()->getAtom()->getName(), querynum);
        AstAtom* newMagAtom = new AstAtom (newMag);
        std::vector<AstArgument*> args = newClause->getHead()->getAtom()->getArguments();

        for(size_t k = 0; k < args.size(); k++){
          if(headAdornment[k] == 'b'){
            newMagAtom->addArgument(std::unique_ptr<AstArgument> (args[k]->clone()));
          }
        }

        newClause->addToBody(std::unique_ptr<AstAtom> (newMagAtom));
        std::vector<unsigned int> newClauseOrder (numAtoms+1);
        for(size_t k = 0; k < numAtoms; k++){
          newClauseOrder[k] = k+1;
        }

        newClauseOrder[numAtoms] = 0;
        newClause->reorderAtoms(reorderOrdering(newClauseOrder));


        // add the clause
        newClauses.push_back(newClause);
        adornedRelation->addClause(std::unique_ptr<AstClause> (newClause));
      }
    }

    // remove all old IDB relations
    for(AstRelationIdentifier relation : oldidb){
      if(program->getRelation(relation)->isOutput()){
        addAsOutput.insert(relation);
      } else if (program->getRelation(relation)->isPrintSize()){
        addAsPrintSize.insert(relation);
      }
      if(negatedAtoms.find(relation) == negatedAtoms.end()){
        program->removeRelation(relation);
      }
      // need AST RELATION NAME here too !! TODO
    }

    // add output relations
    for(size_t i = 0; i < outputQueries.size(); i++){
      AstRelationIdentifier oldname = outputQueries[i];
      AstRelationIdentifier newname = newQueryNames[i];

      size_t prefixpoint = 0;
      std::string mainnewname = newname.getNames()[0];
      while(mainnewname[prefixpoint]!='_'){
        prefixpoint++;
      }

      AstRelationIdentifier newrelationname = createSubIdentifier(mainnewname, prefixpoint+1,mainnewname.size()-(prefixpoint+1));
      AstRelation* adornedRelation = program->getRelation(newrelationname);
      if(adornedRelation==nullptr){
        continue; // TODO: WHY DOES THIS WORK?
      }

      size_t numargs = adornedRelation->getArity();

      AstRelation* outputRelation;
      if(program->getRelation(oldname) != nullptr){
        outputRelation = program->getRelation(oldname);
      } else {
        outputRelation = new AstRelation ();

        for (AstAttribute* attr : adornedRelation->getAttributes()){
          outputRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
        }

        outputRelation->setName(oldname);

        // set as output relation
        AstIODirective* newdir = new AstIODirective();

        if(addAsOutput.find(oldname) != addAsOutput.end()){
          newdir->setAsOutput();
        } else {
          newdir->setAsPrintSize();
        }
        outputRelation->addIODirectives(std::unique_ptr<AstIODirective>(newdir));

        program->appendRelation(std::unique_ptr<AstRelation> (outputRelation));
      }

      // oldname(arg1...argn) :- newname(arg1...argn)
      AstAtom* headatom = new AstAtom (oldname);
      AstAtom* bodyatom = new AstAtom (newrelationname);

      for(size_t j = 0; j < numargs; j++){
        std::stringstream argname; argname.str("");
        argname << "arg" << j;
        headatom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
        bodyatom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
      }

      AstClause* referringClause = new AstClause ();
      referringClause->setHead(std::unique_ptr<AstAtom> (headatom));
      referringClause->addToBody(std::unique_ptr<AstAtom> (bodyatom));

      program->appendClause(std::unique_ptr<AstClause> (referringClause));
    }
    //std::cout << *program << std::endl;
    return true;
  }
} // end of namespace souffle
