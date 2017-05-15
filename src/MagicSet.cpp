#include "AstTransforms.h"
#include "MagicSet.h"
#include "IODirectives.h"
#include "BinaryConstraintOps.h"

#include <string>
#include <vector>

namespace souffle {
  bool argumentContainsFunctors(AstArgument*);
  bool literalContainsFunctors(AstLiteral*);
  bool atomContainsFunctors(AstAtom*);

  // checks whether the clause contains functors or aggregators
  bool containsFunctors(AstClause* clause){
    if(atomContainsFunctors(clause->getHead())){
      return true;
    }
    for(AstLiteral* lit : clause->getBodyLiterals()){
      if(literalContainsFunctors(lit)){
        return true;
      }
    }
    return false;
  }

  bool isAggRel(AstRelationIdentifier rel){
    // TODO: check if this covers too much (__agg_rel_x defined when aggregations used)
    if(rel.getNames()[0].substr(0, 10).compare("__agg_rel_")==0){
      return true;
    } else {
      return false;
    }
  }

  std::set<AstRelationIdentifier> argumentAddAggregations(AstArgument* arg, std::set<AstRelationIdentifier> ignoredVec){
    std::set<AstRelationIdentifier> retVal = ignoredVec;

    // TODO: check if everything covered
    if(dynamic_cast<AstAggregator*>(arg)){
      AstAggregator* aggregator = dynamic_cast<AstAggregator*>(arg);
      // TODO: double check this section
      for(AstLiteral* lit : aggregator->getBodyLiterals()){
        if(lit->getAtom() != nullptr){
          retVal.insert(lit->getAtom()->getName());
        }
      }
    } else if (dynamic_cast<AstFunctor*>(arg)){
      if(dynamic_cast<AstUnaryFunctor*>(arg)){
        AstUnaryFunctor* func = dynamic_cast<AstUnaryFunctor*>(arg);
        retVal = argumentAddAggregations(func->getOperand(), retVal);
      } else if (dynamic_cast<AstBinaryFunctor*>(arg)){
        AstBinaryFunctor* func = dynamic_cast<AstBinaryFunctor*>(arg);
        retVal = argumentAddAggregations(func->getLHS(), retVal);
        retVal = argumentAddAggregations(func->getRHS(), retVal);
      } else if (dynamic_cast<AstTernaryFunctor*>(arg)){
        AstTernaryFunctor* func = dynamic_cast<AstTernaryFunctor*>(arg);
        retVal = argumentAddAggregations(func->getArg(0), retVal);
        retVal = argumentAddAggregations(func->getArg(1), retVal);
        retVal = argumentAddAggregations(func->getArg(2), retVal);
      }
    } else if (dynamic_cast<AstRecordInit*>(arg)){
      AstRecordInit* rec = dynamic_cast<AstRecordInit*>(arg);
      for(AstArgument* subarg : rec->getArguments()){
        retVal = argumentAddAggregations(subarg, retVal);
      }
    } else if (dynamic_cast<AstTypeCast*>(arg)){
      AstTypeCast* tcast = dynamic_cast<AstTypeCast*>(arg);
      retVal = argumentAddAggregations(tcast->getValue(), retVal);
    }

    return retVal;
  }

  std::set<AstRelationIdentifier> atomAddAggregations(AstAtom* atom, std::set<AstRelationIdentifier> ignoredVec){
    std::set<AstRelationIdentifier> retVal = ignoredVec;
    for(AstArgument* arg : atom->getArguments()){
      retVal = argumentAddAggregations(arg, retVal);
    }
    return retVal;
  }

  std::set<AstRelationIdentifier> addAggregations(AstClause* clause, std::set<AstRelationIdentifier> ignoredVec){
    std::set<AstRelationIdentifier> retVal = ignoredVec;
    retVal = atomAddAggregations(clause->getHead(), retVal);
    for(AstLiteral* lit : clause->getBodyLiterals()){
      if(dynamic_cast<AstAtom*> (lit)){
        retVal = atomAddAggregations((AstAtom*) lit, retVal);
      } else if(dynamic_cast<AstNegation*> (lit)){
        retVal = atomAddAggregations((AstAtom*) (lit->getAtom()), retVal);
      } else {
        AstConstraint* cons = dynamic_cast<AstConstraint*> (lit);
        retVal = argumentAddAggregations(cons->getLHS(), retVal);
        retVal = argumentAddAggregations(cons->getRHS(), retVal);
      }
    }
    return retVal;
  }

  bool hasBoundArgument(AstAtom* atom, std::set<std::string> boundedArgs){
    std::stringstream name; name.str("");
    for(AstArgument* arg : atom->getArguments()){
      name.str(""); name << *arg;
      if(boundedArgs.find(name.str()) != boundedArgs.end()){
        return true; // found a bound argument, so can stop
      }
    }
    return false;
  }

  bool isBoundedArg(AstArgument* lhs, AstArgument* rhs, std::set<std::string> boundedArgs){
    std::stringstream lhs_stream; lhs_stream << *lhs;
    std::stringstream rhs_stream; rhs_stream << *rhs;
    std::string lhs_name = lhs_stream.str();
    std::string rhs_name = rhs_stream.str();

    if(dynamic_cast<AstVariable*> (lhs) && (boundedArgs.find(lhs_name) == boundedArgs.end())){
      if(dynamic_cast<AstVariable*> (rhs) && (boundedArgs.find(rhs_name) != boundedArgs.end())){
        return true;
      } else if(dynamic_cast<AstConstant*> (rhs)){
        return true;
      }
    }
    return false;
  }

  // TODO: CHECK DEPENDENCIES
  std::set<AstRelationIdentifier> addDependencies(const AstProgram* program, std::set<AstRelationIdentifier> relations){
    // TODO much more efficient way to do this... for now leave this...
    int countAdded = 0;
    std::set<AstRelationIdentifier> retVals;
    for(AstRelationIdentifier relName : relations){
      retVals.insert(relName);
      for(AstClause* clause : program->getRelation(relName)->getClauses()){
        for(AstLiteral* lit : clause->getBodyLiterals()){
          if(dynamic_cast<AstAtom*> (lit) || dynamic_cast<AstNegation*> (lit)){
            AstRelationIdentifier addedName = lit->getAtom()->getName();
            if(relations.find(addedName) == relations.end()){
              countAdded++;
            }
            retVals.insert(lit->getAtom()->getName());
          }
        }
      }
    }

    if(countAdded > 0){
      return addDependencies(program, retVals);
    } else {
      return retVals;
    }
  }

  std::pair<std::string, std::set<std::string>> bindArguments(AstAtom* currAtom, std::set<std::string> boundedArgs){
    std::set<std::string> newlyBoundedArgs;
    std::string atomAdornment = "";
    for(AstArgument* arg : currAtom->getArguments()){
      std::stringstream argName; argName << *arg;
      if(boundedArgs.find(argName.str()) != boundedArgs.end()){
        atomAdornment += "b"; // bounded
      } else {
        atomAdornment += "f"; // free
        newlyBoundedArgs.insert(argName.str()); // now bounded
      }
    }
    std::pair<std::string, std::set<std::string>> result;
    result.first = atomAdornment; result.second = newlyBoundedArgs;
    return result;
  }

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
          is_edb = false;
          break;
        }
      }

      if(is_edb){
        m_edb.insert(relName);
      } else {
        m_idb.insert(relName);
      }
    }

    // find all negated literals
    std::set<AstRelationIdentifier> negatedLiterals;
    for(AstRelation* rel : program->getRelations()){
      for(AstClause* clause : rel->getClauses()){
        for(AstLiteral* lit : clause->getBodyLiterals()){
          if(dynamic_cast<AstNegation*>(lit)){
            negatedLiterals.insert(lit->getAtom()->getName());
          }
        }
      }
    }

    // TODO: check if all dependencies properly added
    negatedLiterals = addDependencies(program, negatedLiterals);
    m_negatedAtoms = negatedLiterals;

    // find atoms that should be ignored
    std::set<AstRelationIdentifier> ignoredAtoms;

    for(AstRelation* rel : program->getRelations()){
      for(AstClause* clause : rel->getClauses()){
        if(containsFunctors(clause)){
          ignoredAtoms.insert(clause->getHead()->getName());
        }
        ignoredAtoms = addAggregations(clause, ignoredAtoms);
      }
    }

    for(size_t querynum = 0; querynum < outputQueries.size(); querynum++){
      AstRelationIdentifier outputQuery = outputQueries[querynum];
      // adornment algorithm
      std::vector<AdornedPredicate> currentPredicates;
      std::set<AdornedPredicate> seenPredicates;
      std::vector<AdornedClause> adornedClauses;

      std::string frepeat = "";
      size_t arity = program->getRelation(outputQuery)->getArity();
      for(size_t i = 0; i < arity; i++){
        frepeat +="f"; // 'f'*(number of arguments in output query)
      }

      AdornedPredicate outputPredicate (outputQuery, frepeat);
      currentPredicates.push_back(outputPredicate);
      seenPredicates.insert(outputPredicate);

      while(!currentPredicates.empty()){
        // pop out the first element
        AdornedPredicate currPredicate = currentPredicates[0];
        currentPredicates.erase(currentPredicates.begin());

        // go through all clauses defining it
        AstRelation* rel = program->getRelation(currPredicate.getName());

        for(AstClause* clause : rel->getClauses()){
          if(clause->isFact()){
            continue;
          }

          // TODO: check if it contains it first before doing this
          // if(containsFunctors(clause)){
          //   ignoredAtoms.insert(clause->getHead()->getName());
          // }
          // ignoredAtoms = addAggregations(clause, ignoredAtoms);

          // TODO: check if ordering correct
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
          std::vector<AstConstraint*> constraints = clause->getConstraints();

          for(size_t i = 0; i < constraints.size(); i++){
            AstConstraint* constraint = constraints[i];
            BinaryConstraintOp op = constraint->getOperator();
            // TODO: check if MATCH works (or if this works at all)
            if(op != BinaryConstraintOp::EQ && op != BinaryConstraintOp::MATCH){
              continue;
            }
            AstArgument* lhs = constraint->getLHS();
            AstArgument* rhs = constraint->getRHS();

            if(isBoundedArg(lhs, rhs, boundedArgs)){
              name.str(""); name << *lhs;
              boundedArgs.insert(name.str());
            }
            if(isBoundedArg(rhs, lhs, boundedArgs)){
              name.str(""); name << *rhs;
              boundedArgs.insert(name.str());
            }
          }

          std::vector<AstAtom*> atoms = clause->getAtoms();
          int atomsAdorned = 0;
          int atomsTotal = atoms.size();

          while(atomsAdorned < atomsTotal){
            int firstedb = -1; // index of first edb atom
            bool atomAdded = false;

            for(size_t i = 0; i < atoms.size(); i++){
              AstAtom* currAtom = atoms[i];
              if(currAtom == nullptr){
                // already done
                continue;
              }

              AstRelationIdentifier atomName = currAtom->getName();

              // check if this is the first edb atom met
              if(firstedb < 0 && contains(m_edb, atomName)){
                firstedb = i;
              }

              if(hasBoundArgument(currAtom, boundedArgs)){
                // bound argument found, so based on this SIPS, we adorn it
                atomAdded = true;

                // find the adornment pattern
                std::pair<std::string, std::set<std::string>> result = bindArguments(currAtom, boundedArgs);
                std::string atomAdornment = result.first;
                std::set<std::string> newlyBoundedArgs = result.second;

                for(std::string newlyAddedArg : newlyBoundedArgs){
                  boundedArgs.insert(newlyAddedArg);
                }

                AstRelationIdentifier atomName = currAtom->getName();
                bool seenBefore = false;

                // check if we've already dealt with this adornment before
                for(AdornedPredicate seenPred : seenPredicates){
                  if((seenPred.getName() == atomName)
                      && (seenPred.getAdornment().compare(atomAdornment) == 0)){ // TODO: check if correct/better way to do
                        seenBefore = true;
                        break;
                  }
                }

                if(!seenBefore){
                  currentPredicates.push_back(AdornedPredicate (atomName, atomAdornment));
                  seenPredicates.insert(AdornedPredicate (atomName, atomAdornment));
                }

                clauseAtomAdornments[i] = atomAdornment;
                ordering[i] = atomsAdorned;

                atoms[i] = nullptr;
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
              AstAtom* currAtom = atoms[i];
              AstRelationIdentifier atomName = currAtom->getName();

              std::pair<std::string, std::set<std::string>> result = bindArguments(currAtom, boundedArgs);
              std::string atomAdornment = result.first;
              std::set<std::string> newlyBoundedArgs = result.second;

              for(std::string argx : newlyBoundedArgs){
                boundedArgs.insert(argx);
              }

              bool seenBefore = false;
              for(AdornedPredicate seenPred : seenPredicates){
                if( (seenPred.getName() == atomName)
                    && (seenPred.getAdornment().compare(atomAdornment) == 0)){ // TODO: check if correct/better way to do
                      seenBefore = true;
                      break;
                }
              }

              if(!seenBefore){
                currentPredicates.push_back(AdornedPredicate (atomName, atomAdornment));
                seenPredicates.insert(AdornedPredicate (atomName, atomAdornment));
              }

              clauseAtomAdornments[i] = atomAdornment;
              ordering[i] = atomsAdorned;

              atoms[i] = nullptr;
              atomsAdorned++;
            }
          }
          adornedClauses.push_back(AdornedClause (clause, headAdornment, clauseAtomAdornments, ordering));
        }
      }
      m_adornedClauses.push_back(adornedClauses);
    }
    ignoredAtoms = addDependencies(program, ignoredAtoms);
    m_ignoredAtoms = ignoredAtoms;
  }

  bool atomContainsFunctors(AstAtom* atom){
    for(AstArgument* arg : atom->getArguments()){
      if(argumentContainsFunctors(arg)){
        return true;
      }
    }
    return false;
  }

  // NOTE: also getting rid of aggregators
  bool argumentContainsFunctors(AstArgument* arg){
    if(dynamic_cast<AstFunctor*> (arg)){
      // functor found!
      return true;
    } else if(dynamic_cast<AstRecordInit*>(arg)){
      AstRecordInit* recordarg = dynamic_cast<AstRecordInit*> (arg);
      for(AstArgument* subarg : recordarg->getArguments()){
        if(argumentContainsFunctors(subarg)){
          return true;
        }
      }
    } else if(dynamic_cast<AstTypeCast*>(arg)){
      AstTypeCast* typearg = dynamic_cast<AstTypeCast*> (arg);
      if(argumentContainsFunctors(typearg->getValue())){
        return true;
      }
    } else if(dynamic_cast<AstAggregator*>(arg)){
		    return true;
    }
    return false;
  }

  bool literalContainsFunctors(AstLiteral* lit){
    if(dynamic_cast<AstAtom*> (lit)){
      if(atomContainsFunctors(dynamic_cast<AstAtom*> (lit))){
        return true;
      }
    } else if(dynamic_cast<AstNegation*> (lit)){
      AstNegation* negLit = dynamic_cast<AstNegation*> (lit);
      if(atomContainsFunctors(negLit->getAtom())){
        return true;
      }
    } else {
      AstConstraint* cons = dynamic_cast<AstConstraint*> (lit);
      if(argumentContainsFunctors(cons->getLHS()) || argumentContainsFunctors(cons->getRHS())){
        return true;
      }
    }
    return false;
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

  void replaceUnderscores(AstProgram* program){
    for(AstRelation* rel : program->getRelations()){
      std::vector<AstClause*> clauses = rel->getClauses();
      for(size_t clauseNum = 0; clauseNum < clauses.size(); clauseNum++){
        AstClause* clause = clauses[clauseNum];
        AstClause* newClause = clause->cloneHead();

        for(AstLiteral* lit : clause->getBodyLiterals()){

          if(dynamic_cast<AstAtom*>(lit)==0){
            newClause->addToBody(std::unique_ptr<AstLiteral> (lit->clone()));
            continue;
          }

          AstAtom* newLit = lit->getAtom()->clone();
          std::vector<AstArgument*> args = newLit->getArguments();
          for(size_t argNum = 0; argNum < args.size(); argNum++){
            AstArgument* currArg = args[argNum];

            if(dynamic_cast<const AstVariable*>(currArg)){
              AstVariable* var = (AstVariable*) currArg;
              if(var->getName().substr(0,10).compare("underscore") == 0){
                newLit->setArgument(argNum, std::unique_ptr<AstArgument> (new AstUnnamedVariable()));
              }
            }
          }
            newClause->addToBody(std::unique_ptr<AstLiteral> (newLit));
        }
        rel->removeClause(clause);
        rel->addClause(std::unique_ptr<AstClause> (newClause));
      }
    }
  }

  std::vector<unsigned int> reorderOrdering(std::vector<unsigned int> order){
    std::vector<unsigned int> neworder (order.size());
    for(size_t i = 0; i < order.size(); i++){
      neworder[order[i]] = i;
    }
    return neworder;
  }

  std::vector<std::string> reorderAdornment(std::vector<std::string> adornment, std::vector<unsigned int> order){
    std::vector<std::string> result (adornment.size());
    for(size_t i = 0; i < adornment.size(); i++){
      result[order[i]] = adornment[i];
    }
    return result;
  }

  void DEBUGprintProgram(std::unique_ptr<AstProgram> program){
      std::cout << *program << std::endl;
  }

  AstRelationIdentifier createMagicIdentifier(AstRelationIdentifier relationName, size_t outputNumber){
    std::vector<std::string> relationNames = relationName.getNames();
    std::stringstream newMainName; newMainName.str("");
    newMainName << "m" << outputNumber << "_" << relationNames[0]; //<< "_" << adornment; // MAJOR TODO: SHOULD BE AN UNDERSCORE HERE!
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

  AstSrcLocation nextSrcLoc(AstSrcLocation orig){
    static int pos = 0;
    pos += 1;

    AstSrcLocation newLoc;
    newLoc.filename = orig.filename + "__MAGIC.dl";
    newLoc.start.line = pos;
    newLoc.end.line = pos;
    newLoc.start.column = 0;
    newLoc.end.column = 1;

    return newLoc;
  }

  void separateDBs(AstProgram* program){
    int edbNum = 0;
    for(AstRelation* relation : program->getRelations()){
      AstRelationIdentifier relName = relation->getName();

      bool is_edb = false;
      bool is_idb = false;

      for(AstClause* clause : relation->getClauses()){
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
        newedbrel->setSrcLoc(nextSrcLoc(relation->getSrcLoc()));

        for (AstAttribute* attr : relation->getAttributes()){
          newedbrel->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
        }

        std::stringstream newEdbName;
        do {
          newEdbName.str(""); // check
          edbNum++;
          newEdbName << "newedb" << edbNum;
        } while (program->getRelation(newEdbName.str())!=nullptr);

        newedbrel->setName(newEdbName.str());
        program->appendRelation(std::unique_ptr<AstRelation> (newedbrel));
        for(AstClause* clause : relation->getClauses()){
          if(clause->isFact()){
            AstClause* newEdbClause = clause->clone(); // TODO: check if should setSrcLoc
            newEdbClause->getHead()->setName(newEdbName.str());
            program->appendClause(std::unique_ptr<AstClause> (newEdbClause));
          }
        }

        AstClause* newIdbClause = new AstClause();
        newIdbClause->setSrcLoc(nextSrcLoc(relation->getSrcLoc()));

        // oldname(arg1...argn) :- newname(arg1...argn)
        AstAtom* headAtom = new AstAtom (relName);
        AstAtom* bodyAtom = new AstAtom (newEdbName.str());

        size_t numargs = relation->getArity();
        for(size_t j = 0; j < numargs; j++){
          std::stringstream argname; argname.str("");
          argname << "arg" << j;
          headAtom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
          bodyAtom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
        }

        newIdbClause->setHead(std::unique_ptr<AstAtom> (headAtom));
        newIdbClause->addToBody(std::unique_ptr<AstAtom> (bodyAtom));

        program->appendClause(std::unique_ptr<AstClause> (newIdbClause));
      }
    }
  }

  int getEndpoint(std::string mainName){
    int endpt = mainName.size()-1;
    while(endpt >= 0 && mainName[endpt] != '_'){
      endpt--;
    }
    if(endpt == -1){
      endpt = mainName.size();
    }
    return endpt;
  }

  bool contains(std::set<AstRelationIdentifier> set, AstRelationIdentifier element){
    if(set.find(element) != set.end()){
      return true;
    }
    return false;
  }

  bool MagicSetTransformer::transform(AstTranslationUnit& translationUnit){
    AstProgram* program = translationUnit.getProgram();
    separateDBs(program);

    // analysis used to break for dfa.dl - get rid of the 'continue' flag and check the failures again

    Adornment* adornment = translationUnit.getAnalysis<Adornment>();

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

    // TODO: make sure facts can be left alone

    // db handling
    std::vector<std::vector<AdornedClause>> allAdornedClauses = adornment->getAdornedClauses();
    std::set<AstRelationIdentifier> negatedAtoms = adornment->getNegatedAtoms();
    std::set<AstRelationIdentifier> ignoredAtoms = adornment->getIgnoredAtoms();
    std::set<AstRelationIdentifier> oldIdb = adornment->getIDB();
    std::set<AstRelationIdentifier> newIdb;
    std::vector<AstRelationIdentifier> newQueryNames;
    std::vector<AstClause*> newClauses;

    // output handling
    std::vector<AstRelationIdentifier> outputQueries = adornment->getRelations();
    std::set<AstRelationIdentifier> addAsOutput;
    std::set<AstRelationIdentifier> addAsPrintSize;
    std::map<AstRelationIdentifier, std::vector<AstIODirective*>> outputDirectives;

    for(size_t querynum = 0; querynum < outputQueries.size(); querynum++){
      AstRelationIdentifier outputQuery = outputQueries[querynum];
      std::vector<AdornedClause> adornedClauses = allAdornedClauses[querynum];

      // add a relation for the output query
      AstRelation* outputRelationFree = new AstRelation();

      size_t num_free = program->getRelation(outputQuery)->getArity();
      std::string thefs = "";
      for(size_t i = 0; i < num_free; i++){
        thefs += "f";
      }

      AstRelationIdentifier magicOutputName = createMagicIdentifier(createAdornedIdentifier(outputQuery, thefs), querynum);
      outputRelationFree->setName(magicOutputName);
      newQueryNames.push_back(magicOutputName);

      program->appendRelation(std::unique_ptr<AstRelation> (outputRelationFree));
      AstClause* finalOutputClause = new AstClause();
      finalOutputClause->setSrcLoc(nextSrcLoc(program->getRelation(outputQuery)->getSrcLoc()));
      finalOutputClause->setHead(std::unique_ptr<AstAtom> (new AstAtom(magicOutputName)));
      program->appendClause(std::unique_ptr<AstClause> (finalOutputClause));

      // if(olderRelation->isInput()){
      //   for(const AstIODirective* current : olderRelation->getIODirectives()){
      //     std::cout << *current << std::endl;
      //   }
      // }

      for(AdornedClause adornedClause : adornedClauses){
        AstClause* clause = adornedClause.getClause(); // TODO: everything following should have this
        AstRelationIdentifier originalName = clause->getHead()->getName();

        if(contains(ignoredAtoms, originalName)){
          continue;
        }

        std::string headAdornment = adornedClause.getHeadAdornment();

        AstRelationIdentifier newRelName = createAdornedIdentifier(originalName, headAdornment);

        AstRelation* adornedRelation;

        if((adornedRelation = program->getRelation(newRelName)) == nullptr){
          AstRelation* originalRelation = program->getRelation(originalName);

          AstRelation* newRelation = new AstRelation();
          newRelation->setSrcLoc(nextSrcLoc(clause->getSrcLoc()));
          newRelation->setName(newRelName);

          for(AstAttribute* attr : originalRelation->getAttributes()){
            newRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
          }
          // TODO: CHECK IF NEEDED ELSEWHERE (espec for output relations)
          // TODO: fix the setup v messy
          // TODO: remove inputDirectives from the following code

          // TODO: why didn't I just use the getIODirectives thing what the heck
          // -- to change the input file if needed bc of defaults
          if(originalRelation->isInput()){
            IODirectives inputDirectives;
            AstIODirective* newDirective = new AstIODirective();
            inputDirectives.setRelationName(newRelName.getNames()[0]); // TODO: CHECK IF FIRSTN AME
            newDirective->addName(newRelName);
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
              inputDirectives.setFileName(originalName.getNames()[0] + ".facts"); // TODO: CHECK IF FIRSTN AME
              newDirective->addKVP("filename", originalName.getNames()[0] + ".facts");
            }

            newRelation->addIODirectives(std::unique_ptr<AstIODirective>(newDirective));
          }

          program->appendRelation(std::unique_ptr<AstRelation> (newRelation));
          adornedRelation = newRelation;
        }

        AstClause* newClause = clause->clone();
        newClause->getHead()->setName(newRelName);

        newClause->reorderAtoms(reorderOrdering(adornedClause.getOrdering()));

        // add adornments to names
        std::vector<AstLiteral*> body = newClause->getBodyLiterals();
        std::vector<std::string> bodyAdornment = adornedClause.getBodyAdornment();

        std::vector<unsigned int> adordering = adornedClause.getOrdering();

        bodyAdornment = reorderAdornment(bodyAdornment, adordering);

        int count = 0;

        // TODO: NEED TO IGNORE ADORNMENT AFTER IN DEBUG-REPORT

        // set the name of each IDB pred in the clause to be the adorned version
        for(size_t i = 0; i < body.size(); i++){
          AstLiteral* lit = body[i];

          // only IDB should be added
          if(dynamic_cast<AstAtom*>(lit)){
            AstRelationIdentifier litName = lit->getAtom()->getName();
            if(contains(oldIdb, litName)){
              // only do this to the IDB
              if(!contains(ignoredAtoms, litName)){
                AstRelationIdentifier newLitName = createAdornedIdentifier(litName, bodyAdornment[count]);
                AstAtom* atomlit = dynamic_cast<AstAtom*>(lit);
                atomlit->setName(newLitName);
                newIdb.insert(newLitName);
              } else {
                newIdb.insert(litName);
              }
            }
            count++; // TODO: check if placement of this line is correct
          }
        }

        // Possible TODO: function outside this to check if IDB
        // Add the set of magic rules
        for(size_t i = 0; i < body.size(); i++){
          AstLiteral* currentLiteral = body[i];
          count = 0;
          if(dynamic_cast<AstAtom*>(currentLiteral)){
            AstAtom* lit = (AstAtom*) currentLiteral;
            AstRelationIdentifier litName = lit->getAtom()->getName();
            if(contains(newIdb, litName) && !contains(ignoredAtoms, litName)){
              // AstClause* magicClause = newClause->clone();
              AstRelationIdentifier newLitName = createMagicIdentifier(litName, querynum);
              if(program->getRelation(newLitName) == nullptr){
                AstRelation* magicRelation = new AstRelation();
                magicRelation->setName(newLitName);

                std::string mainLitName = litName.getNames()[0];
                int endpt = getEndpoint(mainLitName);

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

              AstAtom* magicHead = new AstAtom(newLitName);
              std::string currAdornment = bodyAdornment[i];
              int argCount = 0;

              for(AstArgument* arg : lit->getArguments()){
                if(currAdornment[argCount] == 'b'){
                  magicHead->addArgument(std::unique_ptr<AstArgument> (arg->clone()));
                }
                argCount++;
              }

              AstClause* magicClause = new AstClause();
              magicClause->setSrcLoc(nextSrcLoc(lit->getSrcLoc()));
              magicClause->setHead(std::unique_ptr<AstAtom> (magicHead));

              // make the body
              // TODO: FIX FROM THIS POINT!!
              AstRelationIdentifier magPredName = createMagicIdentifier(newClause->getHead()->getName(), querynum);
              std::string mainLitName = magPredName.getNames()[0];

              int endpt = getEndpoint(mainLitName);

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
                  addedMagPred->addArgument(std::unique_ptr<AstArgument> (arg->clone()));
                }
                argCount++;
              }

              magicClause->addToBody(std::unique_ptr<AstAtom> (addedMagPred));
              for(size_t j = 0; j < i; j++){
                magicClause->addToBody(std::unique_ptr<AstLiteral> (body[j]->clone()));
              }

              std::vector<AstArgument*> currArguments = magicClause->getHead()->getArguments();
              for(size_t i = 0; i < currArguments.size(); i++){
                AstArgument* arg = currArguments[i];
                std::stringstream tmpvar; tmpvar << *arg;
                if(tmpvar.str().substr(0, 5).compare("abdul")==0){
                  size_t pos;
                  for(pos = 0; pos < tmpvar.str().size(); pos++){
                    if(tmpvar.str()[pos] == '_'){
                      break;
                    }
                  }

                  size_t nextpos;
                  for(nextpos = pos+1; nextpos < tmpvar.str().size(); nextpos++){
                    if(tmpvar.str()[nextpos] == '_' ){
                      break;
                    }
                  }
                  std::string startstr = tmpvar.str().substr(pos+1, nextpos-pos-1);

                  // 1 2 ... pos pos+1 ... size()-3 size()-2 size()-1

                  // check if string or num constant
                  std::string res = tmpvar.str().substr(pos+1, tmpvar.str().size());

                  if(res[res.size()-1] == 's'){
                    const char * str = res.substr(0,res.size()-2).c_str();
                    magicClause->addToBody(std::unique_ptr<AstLiteral>(new AstConstraint(BinaryConstraintOp::EQ,
                          std::unique_ptr<AstArgument>(arg->clone()), std::unique_ptr<AstArgument>(new AstStringConstant(translationUnit.getSymbolTable(), str)))));
                  } else {
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
        const AstAtom* newClauseHead = newClause->getHead()->getAtom();
        AstRelationIdentifier newMag = createMagicIdentifier(newClauseHead->getName(), querynum);
        AstAtom* newMagAtom = new AstAtom (newMag);
        std::vector<AstArgument*> args = newClauseHead->getArguments();

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

        newClause->setSrcLoc(nextSrcLoc(newClause->getSrcLoc()));

        // add the clause
        newClauses.push_back(newClause);
        adornedRelation->addClause(std::unique_ptr<AstClause> (newClause));
      }
    }

    // remove all old IDB relations
    for(AstRelationIdentifier relationName : oldIdb){
      AstRelation* relation = program->getRelation(relationName);
      if(relation->isOutput()){
        addAsOutput.insert(relationName);
        std::vector<AstIODirective*> clonedDirectives;
        for(AstIODirective* iodir : relation->getIODirectives()){
          clonedDirectives.push_back(iodir->clone());
        }
        outputDirectives[relationName] = clonedDirectives;
      } else if (relation->isPrintSize()){
        addAsPrintSize.insert(relationName);
        std::vector<AstIODirective*> clonedDirectives;
        for(AstIODirective* iodir : relation->getIODirectives()){
          clonedDirectives.push_back(iodir->clone());
        }
        outputDirectives[relationName] = clonedDirectives;
      }
      if(contains(ignoredAtoms, relationName) || contains(negatedAtoms, relationName)){
        continue;
      }
      if(!isAggRel(relationName)){
        program->removeRelation(relationName);
      }
    }

    // add output relations
    for(size_t i = 0; i < outputQueries.size(); i++){
      AstRelationIdentifier oldName = outputQueries[i];
      AstRelationIdentifier newName = newQueryNames[i];

      size_t prefixpoint = 0;
      std::string mainnewname = newName.getNames()[0];
      while(mainnewname[prefixpoint]!='_'){
        prefixpoint++;
      }

      AstRelationIdentifier newrelationname = createSubIdentifier(newName, prefixpoint+1,mainnewname.size()-(prefixpoint+1));
      AstRelation* adornedRelation = program->getRelation(newrelationname);
      if(adornedRelation==nullptr){
        continue; // TODO: WHY IS THIS HERE?
      }

      size_t numargs = adornedRelation->getArity();

      AstRelation* outputRelation;
      if(program->getRelation(oldName) != nullptr){
        outputRelation = program->getRelation(oldName);
      } else {
        outputRelation = new AstRelation ();
        outputRelation->setSrcLoc(nextSrcLoc(adornedRelation->getSrcLoc()));

        for (AstAttribute* attr : adornedRelation->getAttributes()){
          outputRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
        }

        outputRelation->setName(oldName);

        // set as output relation - TODO: check if needed with the new model!
        AstIODirective* newdir = new AstIODirective();

        if(addAsOutput.find(oldName) != addAsOutput.end()){
          newdir->setAsOutput();
        } else {
          newdir->setAsPrintSize();
        }
        outputRelation->addIODirectives(std::unique_ptr<AstIODirective>(newdir));

        // for(AstIODirective* iodir : originalRelation->getIODirectives()){
        //   newRelation->addIODirectives(std::unique_ptr<AstIODirective> (iodir->clone()));
        // }

        program->appendRelation(std::unique_ptr<AstRelation> (outputRelation));
      }

      // oldname(arg1...argn) :- newname(arg1...argn)
      AstAtom* headatom = new AstAtom (oldName);
      AstAtom* bodyatom = new AstAtom (newrelationname);

      for(size_t j = 0; j < numargs; j++){
        std::stringstream argname; argname.str("");
        argname << "arg" << j;
        headatom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
        bodyatom->addArgument(std::unique_ptr<AstArgument> (new AstVariable (argname.str()) ));
      }

      AstClause* referringClause = new AstClause ();
      referringClause->setSrcLoc(nextSrcLoc(outputRelation->getSrcLoc()));
      referringClause->setHead(std::unique_ptr<AstAtom> (headatom));
      referringClause->addToBody(std::unique_ptr<AstAtom> (bodyatom));

      program->appendClause(std::unique_ptr<AstClause> (referringClause));
    }

    for(std::pair<AstRelationIdentifier, std::vector<AstIODirective*>> iopair : outputDirectives){
      for(AstIODirective* iodir : iopair.second){
        program->getRelation(iopair.first)->addIODirectives(std::unique_ptr<AstIODirective> (iodir));
      }
    }

    replaceUnderscores(program);

    return true;
  }
} // end of namespace souffle
