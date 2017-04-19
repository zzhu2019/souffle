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

    //std::cout << "____________________" << std::endl;

    // set up IDB/EDB and the output queries
    std::vector<std::string> outputQueries;

    std::vector<std::vector<AdornedClause>> adornedProgram;

    for(AstRelation* rel : program->getRelations()){

      std::stringstream name; name << rel->getName(); // TODO: check if correct?
      // check if output relation or size is printed
      if(rel->isComputed()){
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

    for(size_t querynum = 0; querynum < outputQueries.size(); querynum++){
      std::string outputQuery = outputQueries[querynum];
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

  std::vector<std::string> reorderAdornment(std::vector<std::string> adornment, std::vector<unsigned int> order){
    std::vector<std::string> result (adornment.size());
    for(size_t i = 0; i < adornment.size(); i++){
      result[i] = adornment[order[i]];
    }
    return result;
  }

  bool MagicSetTransformer::transform(AstTranslationUnit& translationUnit){
    AstProgram* program = translationUnit.getProgram();
    // std::cout << *program << std::endl;
    // std::stringstream adornmentstr;
    // translationUnit.getAnalysis<Adornment>()->outputAdornment(adornmentstr);
    // std::cout << adornmentstr.str();

    // make EDB and IDB independent
    int edbNum = 0;
    for(AstRelation* rel : program->getRelations()){

      std::stringstream relname; relname << rel->getName();

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
        AstAtom* headatom = new AstAtom (relname.str());
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

    Adornment* adornment = translationUnit.getAnalysis<Adornment>();
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
    std::vector<std::string> outputQueries = adornment->getRelations();
    std::set<std::string> addAsOutput;
    std::set<std::string> addAsPrintSize;
    std::vector<std::string> newQueryNames;
    std::set<std::string> oldidb = adornment->getIDB();
    std::set<std::string> newidb;
    std::vector<AstClause*> newClauses;

    for(size_t querynum = 0; querynum < outputQueries.size(); querynum++){
      std::string outputQuery = outputQueries[querynum];
      std::vector<AdornedClause> adornedClauses = allAdornedClauses[querynum];

      // add a relation for the output query
      AstRelation* outputRelationFree = new AstRelation();
      size_t num_free = program->getRelation(outputQuery)->getArity();
      std::stringstream thefs; thefs << "_";
      for(size_t i = 0; i < num_free; i++){
        thefs << "f";
      }
      // AstRelation* olderRelation = program->getRelation(outputQuery);
      std::stringstream relnamey; relnamey << "m" << querynum << "_" << outputQuery << thefs.str();
      outputRelationFree->setName(relnamey.str());
      newQueryNames.push_back(relnamey.str());
      program->appendRelation(std::unique_ptr<AstRelation> (outputRelationFree));
      AstAtom* newAtomClauseThing = new AstAtom(relnamey.str());
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

        std::stringstream relName;
        relName << clause->getHead()->getName();

        relName << "_" << headAdornment;

        AstRelation* adornedRelation;

        if((adornedRelation = program->getRelation(relName.str()))==nullptr){
          std::stringstream tmp; tmp << clause->getHead()->getName();
          AstRelation* originalRelation = program->getRelation(tmp.str());

          AstRelation* newRelation = new AstRelation();
          newRelation->setName(relName.str());

          for(AstAttribute* attr : originalRelation->getAttributes()){
            newRelation->addAttribute(std::unique_ptr<AstAttribute> (attr->clone()));
          }
          // TODO: CHECK IF NEEDED ELSEWHERE (espec for output relations)
          // TODO: fix the setup v messy
          if(originalRelation->isInput()){
            IODirectives inputDirectives;
            AstIODirective* newDirective = new AstIODirective();
            inputDirectives.setRelationName(relName.str());
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
              inputDirectives.setFileName(tmp.str() + ".facts");
              newDirective->addKVP("filename", tmp.str() + ".facts");
            }
            newRelation->addIODirectives(std::unique_ptr<AstIODirective>(newDirective));
          }

          program->appendRelation(std::unique_ptr<AstRelation> (newRelation));
          adornedRelation = newRelation;
        }

        AstClause* newClause = clause->clone();
        //std::cout << "BEOFRE: " << *newClause << " " <<  adornedClause.getBodyAdornment() << std::endl;
        newClause->reorderAtoms(adornedClause.getOrdering());
        newClause->getHead()->setName(relName.str());


        // add adornments to names
        std::vector<AstLiteral*> body = newClause->getBodyLiterals();
        std::vector<std::string> bodyAdornment = adornedClause.getBodyAdornment();

        std::vector<unsigned int> adordering = adornedClause.getOrdering();

        // std::cout << adordering << std::endl;
        bodyAdornment = reorderAdornment(bodyAdornment, adordering);
        //std::cout << "AFTER: " << *newClause << " " <<  bodyAdornment << std::endl;

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
              newidb.insert(litName.str());
            }
            count++;
          }
        }

        // [[[todo: function outside this to check if IDB]]]
        // Add the set of magic rules
        for(size_t i = 0; i < body.size(); i++){
          // TODO: PROBLEM of ungroundedness! how to remove ungrounded stuff...
          AstLiteral* currentLiteral = body[i];
          count = 0;
          if(dynamic_cast<AstAtom*>(currentLiteral)){
            AstAtom* lit = (AstAtom*) currentLiteral;
            std::stringstream litname;
            litname << lit->getAtom()->getName();
            // bool allFree = false;
            // for(size_t currx = 0; currx < bodyAdornment[i].size(); currx )
            // if(i == 0 && body.size() == 1 && allFree){
            //   continue;
            // }

            if (newidb.find(litname.str()) != newidb.end()){

              // AstClause* magicClause = newClause->clone();
              std::stringstream newLit; newLit << "m" << querynum << "_" << lit->getAtom()->getName();
              if(program->getRelation(newLit.str()) == nullptr){

                AstRelation* magicRelation = new AstRelation();
                magicRelation->setName(newLit.str());
                int endpt = litname.str().size()-1;
                while(endpt >= 0 && litname.str()[endpt] != '_'){
                  endpt--;
                }
                if(endpt == -1){
                  endpt = litname.str().size();
                }
                AstRelation* originalRelation = program->getRelation(litname.str().substr(0, endpt)); // check not defined
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
              AstAtom* mclauseHead = new AstAtom (newLit.str());

              std::string currAdornment = bodyAdornment[i];

              int argCount = 0;

              for(AstArgument* arg : lit->getArguments()){
                if(currAdornment[argCount] == 'b'){
                  mclauseHead->addArgument(std::unique_ptr<AstArgument> (arg->clone()));
                }
                argCount++;
              }

              magicClause->setHead(std::unique_ptr<AstAtom> (mclauseHead));

              // make the body
              // TODO: CHECK WHAT THIS PART DOES AND WHY - not working
              // TODO: FIX FROM THIS POINT!!
              argCount = 0;
              std::stringstream magPredName;
              magPredName << "m" << querynum << "_" << newClause->getHead()->getName();
              int endpt = magPredName.str().size()-1;
              while(endpt >= 0 && magPredName.str()[endpt] != '_'){
                endpt--;
              }
              if(endpt == -1){
                endpt = magPredName.str().size();
              }

              // changed stuff here ...
              std::string curradorn = magPredName.str().substr(endpt+1, magPredName.str().size() - (endpt + 1));

              if(program->getRelation(magPredName.str()) == nullptr){
                AstRelation* freeRelation = new AstRelation();
                freeRelation->setName(magPredName.str());
                AstRelation* currrel =  program->getRelation(newClause->getHead()->getName());
                std::vector<AstAttribute*> attrs = currrel->getAttributes();
                for(size_t currarg = 0; currarg < currrel->getArity(); currarg++){
                  if(curradorn[currarg] == 'b'){
                    freeRelation->addAttribute(std::unique_ptr<AstAttribute> (attrs[currarg]->clone()));
                  }
                }
                program->appendRelation(std::unique_ptr<AstRelation>(freeRelation));
              }
              AstAtom* addedMagPred = new AstAtom(magPredName.str());
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
        std::stringstream newMag; newMag << "m" << querynum << "_" << newClause->getHead()->getAtom()->getName();
        AstAtom* newMagAtom = new AstAtom (newMag.str());
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
        newClause->reorderAtoms(newClauseOrder);


        // add the clause
        newClauses.push_back(newClause);
        adornedRelation->addClause(std::unique_ptr<AstClause> (newClause));
      }
    }

    // remove all old IDB relations
    for(std::string relation : oldidb){
      if(program->getRelation(relation)->isOutput()){
        addAsOutput.insert(relation);
      } else if (program->getRelation(relation)->isPrintSize()){
        addAsPrintSize.insert(relation);
      }
      program->removeRelation(relation);
    }

    // add output relations
    for(size_t i = 0; i < outputQueries.size(); i++){
      std::string oldname = outputQueries[i];
      std::string newname = newQueryNames[i];

      size_t prefixpoint = 0;
      while(newname[prefixpoint]!='_'){
        prefixpoint++;
      }

      std::string newrelationname = newname.substr(prefixpoint+1,newname.size()-(prefixpoint+1));
      AstRelation* adornedRelation = program->getRelation(newrelationname);
      if(adornedRelation==nullptr){
        continue; // TODO: WHY DOES THIS WORK?
      }

      size_t numargs = adornedRelation->getArity();


      AstRelation* outputRelation = new AstRelation ();

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
