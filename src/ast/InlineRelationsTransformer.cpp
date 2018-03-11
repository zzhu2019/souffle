#include "ast/Transforms.h"
#include "ast/Visitor.h"

namespace souffle::ast {

template <class T>
class NullableVector {
private:
    std::vector<T> vector;
    bool valid;

public:
    NullableVector() : valid(false) {}
    NullableVector(std::vector<T> vector) : vector(vector), valid(true) {}

    bool isValid() const {
        return valid;
    }

    const std::vector<T>& getVector() const {
        assert(valid && "Accessing invalid vector!");
        return vector;
    }
};

NullableVector<std::vector<ast::Literal*>> getInlinedLiteral(ast::Program&, ast::Literal*);

/**
 * Replace constants in the head of inlined clauses with (constrained) variables.
 */
void normaliseInlinedHeads(ast::Program& program) {
    static int newVarCount = 0;

    // Go through the clauses of all inlined relations
    for (ast::Relation* rel : program.getRelations()) {
        if (!rel->isInline()) {
            break;
        }

        for (ast::Clause* clause : rel->getClauses()) {
            // Set up the new clause with an empty body and no arguments in the head
            auto newClause = std::make_unique<ast::Clause>();
            newClause->setSrcLoc(clause->getSrcLoc());
            auto clauseHead = std::make_unique<ast::Atom>(clause->getHead()->getName());

            // Add in everything in the original body
            for (ast::Literal* lit : clause->getBodyLiterals()) {
                newClause->addToBody(std::unique_ptr<ast::Literal>(lit->clone()));
            }

            // Set up the head arguments in the new clause
            for (ast::Argument* arg : clause->getHead()->getArguments()) {
                if (ast::Constant* constant = dynamic_cast<ast::Constant*>(arg)) {
                    // Found a constant in the head, so replace it with a variable
                    std::stringstream newVar;
                    newVar << "<new_var_" << newVarCount++ << ">";
                    clauseHead->addArgument(std::make_unique<ast::Variable>(newVar.str()));

                    // Add a body constraint to set the variable's value to be the original constant
                    newClause->addToBody(std::make_unique<ast::BinaryConstraint>(BinaryConstraintOp::EQ,
                            std::make_unique<ast::Variable>(newVar.str()),
                            std::unique_ptr<ast::Argument>(constant->clone())));
                } else {
                    // Already a variable
                    clauseHead->addArgument(std::unique_ptr<ast::Argument>(arg->clone()));
                }
            }

            newClause->setHead(std::move(clauseHead));

            // Replace the old clause with this one
            rel->addClause(std::move(newClause));
            program.removeClause(clause);
        }
    }
}

/**
 * Removes all underscores in all atoms of inlined relations
 */
void nameInlinedUnderscores(ast::Program& program) {
    struct M : public ast::NodeMapper {
        const std::set<ast::RelationIdentifier> inlinedRelations;
        bool replaceUnderscores;

        M(std::set<ast::RelationIdentifier> inlinedRelations, bool replaceUnderscores)
                : inlinedRelations(inlinedRelations), replaceUnderscores(replaceUnderscores) {}

        std::unique_ptr<ast::Node> operator()(std::unique_ptr<ast::Node> node) const override {
            static int underscoreCount = 0;

            if (!replaceUnderscores) {
                // Check if we should start replacing underscores for this node's subnodes
                if (ast::Atom* atom = dynamic_cast<ast::Atom*>(node.get())) {
                    if (inlinedRelations.find(atom->getName()) != inlinedRelations.end()) {
                        // Atom associated with an inlined relation, so replace the underscores
                        // in all of its subnodes with named variables.
                        M replace(inlinedRelations, true);
                        node->apply(replace);
                        return node;
                    }
                }
            } else if (dynamic_cast<ast::UnnamedVariable*>(node.get())) {
                // Give a unique name to the underscored variable
                // TODO (azreika): need a more consistent way of handling internally generated variables in
                // general
                std::stringstream newVarName;
                newVarName << "<underscore_" << underscoreCount++ << ">";
                return std::make_unique<ast::Variable>(newVarName.str());
            }

            node->apply(*this);
            return node;
        }
    };

    // Store the names of all relations to be inlined
    std::set<ast::RelationIdentifier> inlinedRelations;
    for (ast::Relation* rel : program.getRelations()) {
        if (rel->isInline()) {
            inlinedRelations.insert(rel->getName());
        }
    }

    // Apply the renaming procedure to the entire program
    M update(inlinedRelations, false);
    program.apply(update);
}

/**
 * Checks if a given clause contains an atom that should be inlined.
 */
bool containsInlinedAtom(const ast::Program& program, const ast::Clause& clause) {
    bool foundInlinedAtom = false;

    visitDepthFirst(clause, [&](const ast::Atom& atom) {
        ast::Relation* rel = program.getRelation(atom.getName());
        if (rel->isInline()) {
            foundInlinedAtom = true;
        }
    });

    return foundInlinedAtom;
}

/**
 * Reduces a vector of substitutions.
 * Returns false only if matched argument pairs are found to be incompatible.
 */
bool reduceSubstitution(std::vector<std::pair<ast::Argument*, ast::Argument*>>& sub) {
    // Type-Checking functions
    auto isConstant = [&](ast::Argument* arg) { return (dynamic_cast<ast::Constant*>(arg)); };
    auto isRecord = [&](ast::Argument* arg) { return (dynamic_cast<ast::RecordInit*>(arg)); };

    // Keep trying to reduce the substitutions until we reach a fixed point.
    // Note that at this point no underscores ('_') or counters ('$') should appear.
    bool done = false;
    while (!done) {
        done = true;

        // Try reducing each pair by one step
        for (size_t i = 0; i < sub.size(); i++) {
            auto currPair = sub[i];
            ast::Argument* lhs = currPair.first;
            ast::Argument* rhs = currPair.second;

            // Start trying to reduce the substitution
            // Note: Can probably go further with this substitution reduction
            if (*lhs == *rhs) {
                // Get rid of redundant `x = x`
                sub.erase(sub.begin() + i);
                done = false;
            } else if (isConstant(lhs) && isConstant(rhs)) {
                // Both are constants but not equal (prev case => !=)
                // Failed to unify!
                return false;
            } else if (isRecord(lhs) && isRecord(rhs)) {
                // Note: we will not deal with the case where only one side is
                // a record and the other is a variable, as variables can be records
                // on a deeper level.
                std::vector<ast::Argument*> lhsArgs = static_cast<ast::RecordInit*>(lhs)->getArguments();
                std::vector<ast::Argument*> rhsArgs = static_cast<ast::RecordInit*>(rhs)->getArguments();

                if (lhsArgs.size() != rhsArgs.size()) {
                    // Records of unequal size can't be equated
                    return false;
                }

                // Equate all corresponding arguments
                for (size_t i = 0; i < lhsArgs.size(); i++) {
                    sub.push_back(std::make_pair(lhsArgs[i], rhsArgs[i]));
                }

                // Get rid of the record equality
                sub.erase(sub.begin() + i);
                done = false;
            } else if ((isRecord(lhs) && isConstant(rhs)) || (isConstant(lhs) && isRecord(rhs))) {
                // A record =/= a constant
                return false;
            }
        }
    }

    return true;
}

/**
 * Returns the nullable vector of substitutions needed to unify the two given atoms.
 * If unification is not successful, the returned vector is marked as invalid.
 * Assumes that the atoms are both of the same relation.
 */
NullableVector<std::pair<ast::Argument*, ast::Argument*>> unifyAtoms(ast::Atom* first, ast::Atom* second) {
    std::vector<std::pair<ast::Argument*, ast::Argument*>> substitution;

    std::vector<ast::Argument*> firstArgs = first->getArguments();
    std::vector<ast::Argument*> secondArgs = second->getArguments();

    // Create the initial unification equalities
    for (size_t i = 0; i < firstArgs.size(); i++) {
        substitution.push_back(std::make_pair(firstArgs[i], secondArgs[i]));
    }

    // Reduce the substitutions
    bool success = reduceSubstitution(substitution);
    if (success) {
        return NullableVector<std::pair<ast::Argument*, ast::Argument*>>(substitution);
    } else {
        // Failed to unify the two atoms
        return NullableVector<std::pair<ast::Argument*, ast::Argument*>>();
    }
}

/**
 * Inlines the given atom based on a given clause.
 * Returns the vector of replacement literals and the necessary constraints.
 * If unification is unsuccessful, the vector of literals is marked as invalid.
 */
std::pair<NullableVector<ast::Literal*>, std::vector<ast::BinaryConstraint*>> inlineBodyLiterals(
        ast::Atom* atom, ast::Clause* atomInlineClause) {
    bool changed = false;
    std::vector<ast::Literal*> addedLits;
    std::vector<ast::BinaryConstraint*> constraints;

    // Rename the variables in the inlined clause to avoid conflicts when unifying multiple atoms
    // - particularly when an inlined relation appears twice in a clause.
    static int inlineCount = 0;

    // Make a temporary clone so we can rename variables without fear
    ast::Clause* atomClause = atomInlineClause->clone();

    struct VariableRenamer : public ast::NodeMapper {
        int varnum;
        VariableRenamer(int varnum) : varnum(varnum) {}
        std::unique_ptr<ast::Node> operator()(std::unique_ptr<ast::Node> node) const override {
            if (ast::Variable* var = dynamic_cast<ast::Variable*>(node.get())) {
                // Rename the variable
                auto newVar = std::unique_ptr<ast::Variable>(var->clone());
                std::stringstream newName;
                newName << "<inlined_" << var->getName() << "_" << varnum << ">";
                newVar->setName(newName.str());
                return std::move(newVar);
            }
            node->apply(*this);
            return node;
        }
    };

    VariableRenamer update(inlineCount);
    atomClause->apply(update);

    inlineCount++;

    // Get the constraints needed to unify the two atoms
    NullableVector<std::pair<ast::Argument*, ast::Argument*>> res = unifyAtoms(atomClause->getHead(), atom);
    if (res.isValid()) {
        changed = true;
        for (std::pair<ast::Argument*, ast::Argument*> pair : res.getVector()) {
            constraints.push_back(new ast::BinaryConstraint(BinaryConstraintOp::EQ,
                    std::unique_ptr<ast::Argument>(pair.first->clone()),
                    std::unique_ptr<ast::Argument>(pair.second->clone())));
        }

        // Add in the body of the current clause of the inlined atom
        for (ast::Literal* lit : atomClause->getBodyLiterals()) {
            addedLits.push_back(lit->clone());
        }
    }

    delete atomClause;

    if (changed) {
        return std::make_pair(NullableVector<ast::Literal*>(addedLits), constraints);
    } else {
        return std::make_pair(NullableVector<ast::Literal*>(), constraints);
    }
}

/**
 * Returns the negated version of a given literal
 */
ast::Literal* negateLiteral(ast::Literal* lit) {
    if (ast::Atom* atom = dynamic_cast<ast::Atom*>(lit)) {
        ast::Negation* neg = new ast::Negation(std::unique_ptr<ast::Atom>(atom->clone()));
        return neg;
    } else if (ast::Negation* neg = dynamic_cast<ast::Negation*>(lit)) {
        ast::Atom* atom = neg->getAtom()->clone();
        return atom;
    } else if (ast::Constraint* cons = dynamic_cast<ast::Constraint*>(lit)) {
        ast::Constraint* newCons = cons->clone();
        newCons->negate();
        return newCons;
    } else {
        assert(false && "Unsupported literal type!");
    }
}

/**
 * Return the negated version of a disjunction of conjunctions.
 * E.g. (a1(x) ^ a2(x) ^ ...) v (b1(x) ^ b2(x) ^ ...) --into-> (!a1(x) ^ !b1(x)) v (!a2(x) ^ !b2(x)) v ...
 */
std::vector<std::vector<ast::Literal*>> combineNegatedLiterals(
        std::vector<std::vector<ast::Literal*>> litGroups) {
    std::vector<std::vector<ast::Literal*>> negation;

    // Corner case: !() = ()
    if (litGroups.empty()) {
        return negation;
    }

    std::vector<ast::Literal*> litGroup = litGroups[0];
    if (litGroups.size() == 1) {
        // !(a1 ^ a2 ^ a3 ^ ...) --into-> !a1 v !a2 v !a3 v ...
        for (size_t i = 0; i < litGroup.size(); i++) {
            std::vector<ast::Literal*> newVec;
            newVec.push_back(negateLiteral(litGroup[i]));
            negation.push_back(newVec);
        }

        // Done!
        return negation;
    }

    // Produce the negated versions of all disjunctions ignoring the first recursively
    std::vector<std::vector<ast::Literal*>> combinedRHS = combineNegatedLiterals(
            std::vector<std::vector<ast::Literal*>>(litGroups.begin() + 1, litGroups.end()));

    // We now just need to add the negation of a single literal from the untouched LHS
    // to every single conjunction on the RHS to finalise creating every possible combination
    for (ast::Literal* lhsLit : litGroup) {
        for (std::vector<ast::Literal*> rhsVec : combinedRHS) {
            std::vector<ast::Literal*> newVec;
            newVec.push_back(negateLiteral(lhsLit));

            for (ast::Literal* lit : rhsVec) {
                newVec.push_back(lit->clone());
            }

            negation.push_back(newVec);
        }
    }

    return negation;
}

/**
 * Forms the bodies that will replace the negation of a given inlined atom.
 * E.g. a(x) <- (a11(x), a12(x)) ; (a21(x), a22(x)) => !a(x) <- (!a11(x), !a21(x)) ; (!a11(x), !a22(x)) ; ...
 * Essentially, produce every combination (m_1 ^ m_2 ^ ...) where m_i is the negation of a literal in the
 * ith rule of a.
 */
std::vector<std::vector<ast::Literal*>> formNegatedLiterals(ast::Program& program, ast::Atom* atom) {
    // Constraints added to unify atoms should not be negated and should be added to
    // all the final rule combinations produced, and so should be stored separately.
    std::vector<std::vector<ast::Literal*>> addedBodyLiterals;
    std::vector<std::vector<ast::BinaryConstraint*>> addedConstraints;

    // Go through every possible clause associated with the given atom
    for (ast::Clause* inClause : program.getRelation(atom->getName())->getClauses()) {
        // Form the replacement clause by inlining based on the current clause
        std::pair<NullableVector<ast::Literal*>, std::vector<ast::BinaryConstraint*>> inlineResult =
                inlineBodyLiterals(atom, inClause);
        NullableVector<ast::Literal*> replacementBodyLiterals = inlineResult.first;
        std::vector<ast::BinaryConstraint*> currConstraints = inlineResult.second;

        if (!replacementBodyLiterals.isValid()) {
            // Failed to unify, so just move on
            continue;
        }

        addedBodyLiterals.push_back(replacementBodyLiterals.getVector());
        addedConstraints.push_back(currConstraints);
    }

    // We now have a list of bodies needed to inline the given atom.
    // We want to inline the negated version, however, which is done using De Morgan's Law.
    std::vector<std::vector<ast::Literal*>> negatedAddedBodyLiterals =
            combineNegatedLiterals(addedBodyLiterals);

    // Add in the necessary constraints to all the body literals
    for (size_t i = 0; i < negatedAddedBodyLiterals.size(); i++) {
        for (std::vector<ast::BinaryConstraint*> constraintGroup : addedConstraints) {
            for (ast::BinaryConstraint* constraint : constraintGroup) {
                negatedAddedBodyLiterals[i].push_back(constraint->clone());
            }
        }
    }

    // Free up the old body literals and constraints
    for (std::vector<ast::Literal*> litGroup : addedBodyLiterals) {
        for (ast::Literal* lit : litGroup) {
            delete lit;
        }
    }
    for (std::vector<ast::BinaryConstraint*> consGroup : addedConstraints) {
        for (ast::Constraint* cons : consGroup) {
            delete cons;
        }
    }

    return negatedAddedBodyLiterals;
}

/**
 * Renames all variables in a given argument uniquely.
 */
void renameVariables(ast::Argument* arg) {
    static int varCount = 0;
    varCount++;

    struct M : public ast::NodeMapper {
        int varnum;
        M(int varnum) : varnum(varnum) {}
        std::unique_ptr<ast::Node> operator()(std::unique_ptr<ast::Node> node) const override {
            if (ast::Variable* var = dynamic_cast<ast::Variable*>(node.get())) {
                auto newVar = std::unique_ptr<ast::Variable>(var->clone());
                std::stringstream newName;
                newName << var->getName() << "-v" << varnum;
                newVar->setName(newName.str());
                return std::move(newVar);
            }
            node->apply(*this);
            return node;
        }
    };

    M update(varCount);
    arg->apply(update);
}

// Performs a given binary op on a list of aggregators recursively.
// E.g. ( <aggr1, aggr2, aggr3, ...>, o > = (aggr1 o (aggr2 o (agg3 o (...))))
ast::Argument* combineAggregators(std::vector<ast::Aggregator*> aggrs, BinaryOp fun) {
    // Due to variable scoping issues with aggregators, we rename all variables uniquely in the
    // added aggregator
    renameVariables(aggrs[0]);

    if (aggrs.size() == 1) {
        return aggrs[0];
    }

    ast::Argument* rhs = combineAggregators(std::vector<ast::Aggregator*>(aggrs.begin() + 1, aggrs.end()), fun);

    ast::Argument* result = new ast::BinaryFunctor(
            fun, std::unique_ptr<ast::Argument>(aggrs[0]), std::unique_ptr<ast::Argument>(rhs));

    return result;
}

/**
 * Returns a vector of arguments that should replace the given argument after one step of inlining.
 * Note: This function is currently generalised to perform any required inlining within aggregators
 * as well, making it simple to extend to this later on if desired (and the semantic check is removed).
 */
NullableVector<ast::Argument*> getInlinedArgument(ast::Program& program, const ast::Argument* arg) {
    bool changed = false;
    std::vector<ast::Argument*> versions;

    // Each argument has to be handled differently - essentially, want to go down to
    // nested aggregators, and inline their bodies if needed.
    if (const ast::Aggregator* aggr = dynamic_cast<const ast::Aggregator*>(arg)) {
        // First try inlining the target expression if necessary
        if (aggr->getTargetExpression() != nullptr) {
            NullableVector<ast::Argument*> argumentVersions =
                    getInlinedArgument(program, aggr->getTargetExpression());

            if (argumentVersions.isValid()) {
                // An element in the target expression can be inlined!
                changed = true;

                // Create a new aggregator per version of the target expression
                for (ast::Argument* newArg : argumentVersions.getVector()) {
                    ast::Aggregator* newAggr = new ast::Aggregator(aggr->getOperator());
                    newAggr->setTargetExpression(std::unique_ptr<ast::Argument>(newArg));
                    for (ast::Literal* lit : aggr->getBodyLiterals()) {
                        newAggr->addBodyLiteral(std::unique_ptr<ast::Literal>(lit->clone()));
                    }
                    versions.push_back(newAggr);
                }
            }
        }

        // Try inlining body arguments if the target expression has not been changed.
        // (At this point we only handle one step of inlining at a time)
        if (!changed) {
            std::vector<ast::Literal*> bodyLiterals = aggr->getBodyLiterals();
            for (size_t i = 0; i < bodyLiterals.size(); i++) {
                ast::Literal* currLit = bodyLiterals[i];

                NullableVector<std::vector<ast::Literal*>> literalVersions =
                        getInlinedLiteral(program, currLit);

                if (literalVersions.isValid()) {
                    // Literal can be inlined!
                    changed = true;

                    ast::Aggregator::Op op = aggr->getOperator();

                    // Create an aggregator (with the same operation) for each possible body
                    std::vector<ast::Aggregator*> aggrVersions;
                    for (std::vector<ast::Literal*> inlineVersions : literalVersions.getVector()) {
                        ast::Aggregator* newAggr = new ast::Aggregator(aggr->getOperator());
                        if (aggr->getTargetExpression() != nullptr) {
                            newAggr->setTargetExpression(
                                    std::unique_ptr<ast::Argument>(aggr->getTargetExpression()->clone()));
                        }

                        // Add in everything except the current literal being replaced
                        for (size_t j = 0; j < bodyLiterals.size(); j++) {
                            if (i != j) {
                                newAggr->addBodyLiteral(
                                        std::unique_ptr<ast::Literal>(bodyLiterals[j]->clone()));
                            }
                        }

                        // Add in everything new that replaces that literal
                        for (ast::Literal* addedLit : inlineVersions) {
                            newAggr->addBodyLiteral(std::unique_ptr<ast::Literal>(addedLit));
                        }

                        aggrVersions.push_back(newAggr);
                    }

                    // Create the actual overall aggregator that ties the replacement aggregators together.
                    if (op == ast::Aggregator::min) {
                        // min x : { a(x) }. <=> min ( min x : { a1(x) }, min x : { a2(x) }, ... )
                        versions.push_back(combineAggregators(aggrVersions, BinaryOp::MIN));
                    } else if (op == ast::Aggregator::max) {
                        // max x : { a(x) }. <=> max ( max x : { a1(x) }, max x : { a2(x) }, ... )
                        versions.push_back(combineAggregators(aggrVersions, BinaryOp::MAX));
                    } else if (op == ast::Aggregator::count) {
                        // count : { a(x) }. <=> sum ( count : { a1(x) }, count : { a2(x) }, ... )
                        versions.push_back(combineAggregators(aggrVersions, BinaryOp::ADD));
                    } else if (op == ast::Aggregator::sum) {
                        // sum x : { a(x) }. <=> sum ( sum x : { a1(x) }, sum x : { a2(x) }, ... )
                        versions.push_back(combineAggregators(aggrVersions, BinaryOp::ADD));
                    } else {
                        assert(false && "Unsupported aggregator type");
                    }
                }

                // Only perform one stage of inlining at a time.
                if (changed) {
                    break;
                }
            }
        }
    } else if (dynamic_cast<const ast::Functor*>(arg)) {
        // Each type of functor (unary, binary, ternary) must be handled differently.
        if (const ast::UnaryFunctor* functor = dynamic_cast<const ast::UnaryFunctor*>(arg)) {
            NullableVector<ast::Argument*> argumentVersions =
                    getInlinedArgument(program, functor->getOperand());
            if (argumentVersions.isValid()) {
                changed = true;
                for (ast::Argument* newArg : argumentVersions.getVector()) {
                    ast::Argument* newFunctor =
                            new ast::UnaryFunctor(functor->getFunction(), std::unique_ptr<ast::Argument>(newArg));
                    versions.push_back(newFunctor);
                }
            }
        } else if (const ast::BinaryFunctor* functor = dynamic_cast<const ast::BinaryFunctor*>(arg)) {
            NullableVector<ast::Argument*> lhsVersions = getInlinedArgument(program, functor->getLHS());
            if (lhsVersions.isValid()) {
                changed = true;
                for (ast::Argument* newLhs : lhsVersions.getVector()) {
                    ast::Argument* newFunctor =
                            new ast::BinaryFunctor(functor->getFunction(), std::unique_ptr<ast::Argument>(newLhs),
                                    std::unique_ptr<ast::Argument>(functor->getRHS()->clone()));
                    versions.push_back(newFunctor);
                }
            } else {
                NullableVector<ast::Argument*> rhsVersions = getInlinedArgument(program, functor->getRHS());
                if (rhsVersions.isValid()) {
                    changed = true;
                    for (ast::Argument* newRhs : rhsVersions.getVector()) {
                        ast::Argument* newFunctor = new ast::BinaryFunctor(functor->getFunction(),
                                std::unique_ptr<ast::Argument>(functor->getLHS()->clone()),
                                std::unique_ptr<ast::Argument>(newRhs));
                        versions.push_back(newFunctor);
                    }
                }
            }
        } else if (const ast::TernaryFunctor* functor = dynamic_cast<const ast::TernaryFunctor*>(arg)) {
            NullableVector<ast::Argument*> leftVersions = getInlinedArgument(program, functor->getArg(0));
            if (leftVersions.isValid()) {
                changed = true;
                for (ast::Argument* newLeft : leftVersions.getVector()) {
                    ast::Argument* newFunctor = new ast::TernaryFunctor(functor->getFunction(),
                            std::unique_ptr<ast::Argument>(newLeft),
                            std::unique_ptr<ast::Argument>(functor->getArg(1)->clone()),
                            std::unique_ptr<ast::Argument>(functor->getArg(2)->clone()));
                    versions.push_back(newFunctor);
                }
            } else {
                NullableVector<ast::Argument*> middleVersions = getInlinedArgument(program, functor->getArg(1));
                if (middleVersions.isValid()) {
                    changed = true;
                    for (ast::Argument* newMiddle : middleVersions.getVector()) {
                        ast::Argument* newFunctor = new ast::TernaryFunctor(functor->getFunction(),
                                std::unique_ptr<ast::Argument>(functor->getArg(0)->clone()),
                                std::unique_ptr<ast::Argument>(newMiddle),
                                std::unique_ptr<ast::Argument>(functor->getArg(2)->clone()));
                        versions.push_back(newFunctor);
                    }
                } else {
                    NullableVector<ast::Argument*> rightVersions =
                            getInlinedArgument(program, functor->getArg(2));
                    if (rightVersions.isValid()) {
                        changed = true;
                        for (ast::Argument* newRight : rightVersions.getVector()) {
                            ast::Argument* newFunctor = new ast::TernaryFunctor(functor->getFunction(),
                                    std::unique_ptr<ast::Argument>(functor->getArg(0)->clone()),
                                    std::unique_ptr<ast::Argument>(functor->getArg(1)->clone()),
                                    std::unique_ptr<ast::Argument>(newRight));
                            versions.push_back(newFunctor);
                        }
                    }
                }
            }
        }
    } else if (const ast::TypeCast* cast = dynamic_cast<const ast::TypeCast*>(arg)) {
        NullableVector<ast::Argument*> argumentVersions = getInlinedArgument(program, cast->getValue());
        if (argumentVersions.isValid()) {
            changed = true;
            for (ast::Argument* newArg : argumentVersions.getVector()) {
                ast::Argument* newTypeCast =
                        new ast::TypeCast(std::unique_ptr<ast::Argument>(newArg), cast->getType());
                versions.push_back(newTypeCast);
            }
        }
    } else if (const ast::RecordInit* record = dynamic_cast<const ast::RecordInit*>(arg)) {
        std::vector<ast::Argument*> recordArguments = record->getArguments();
        for (size_t i = 0; i < recordArguments.size(); i++) {
            ast::Argument* currentRecArg = recordArguments[i];
            NullableVector<ast::Argument*> argumentVersions = getInlinedArgument(program, currentRecArg);
            if (argumentVersions.isValid()) {
                changed = true;
                for (ast::Argument* newArgumentVersion : argumentVersions.getVector()) {
                    ast::RecordInit* newRecordArg = new ast::RecordInit();
                    for (size_t j = 0; j < recordArguments.size(); j++) {
                        if (i == j) {
                            newRecordArg->add(std::unique_ptr<ast::Argument>(newArgumentVersion));
                        } else {
                            newRecordArg->add(std::unique_ptr<ast::Argument>(recordArguments[j]->clone()));
                        }
                    }
                    versions.push_back(newRecordArg);
                }
            }

            // Only perform one stage of inlining at a time.
            if (changed) {
                break;
            }
        }
    }

    if (changed) {
        // Return a valid vector - replacements need to be made!
        return NullableVector<ast::Argument*>(versions);
    } else {
        // Return an invalid vector - no inlining has occurred
        return NullableVector<ast::Argument*>();
    }
}

/**
 * Returns a vector of atoms that should replace the given atom after one step of inlining.
 * Assumes the relation the atom belongs to is not inlined itself.
 */
NullableVector<ast::Atom*> getInlinedAtom(ast::Program& program, ast::Atom& atom) {
    bool changed = false;
    std::vector<ast::Atom*> versions;

    // Try to inline each of the atom's arguments
    std::vector<ast::Argument*> arguments = atom.getArguments();
    for (size_t i = 0; i < arguments.size(); i++) {
        ast::Argument* arg = arguments[i];

        NullableVector<ast::Argument*> argumentVersions = getInlinedArgument(program, arg);

        if (argumentVersions.isValid()) {
            // Argument has replacements
            changed = true;

            // Create a new atom per new version of the argument
            for (ast::Argument* newArgument : argumentVersions.getVector()) {
                ast::Atom* newAtom = atom.clone();
                newAtom->setArgument(i, std::unique_ptr<ast::Argument>(newArgument));
                versions.push_back(newAtom);
            }
        }

        // Only perform one stage of inlining at a time.
        if (changed) {
            break;
        }
    }

    if (changed) {
        // Return a valid vector - replacements need to be made!
        return NullableVector<ast::Atom*>(versions);
    } else {
        // Return an invalid vector - no replacements need to be made
        return NullableVector<ast::Atom*>();
    }
}

/**
 * Tries to perform a single step of inlining on the given literal.
 * Returns a pair of nullable vectors (v, w) such that:
 *    - v is valid if and only if the literal can be directly inlined, whereby it
 *      contains the bodies that replace it
 *    - if v is not valid, then w is valid if and only if the literal cannot be inlined directly,
 *      but contains a subargument that can be. In this case, it will contain the versions
 *      that will replace it.
 *    - If both are invalid, then no more inlining can occur on this literal and we are done.
 */
NullableVector<std::vector<ast::Literal*>> getInlinedLiteral(ast::Program& program, ast::Literal* lit) {
    bool inlined = false;
    bool changed = false;

    std::vector<std::vector<ast::Literal*>> addedBodyLiterals;
    std::vector<ast::Literal*> versions;

    if (ast::Atom* atom = dynamic_cast<ast::Atom*>(lit)) {
        // Check if this atom is meant to be inlined
        ast::Relation* rel = program.getRelation(atom->getName());

        if (rel->isInline()) {
            // We found an atom in the clause that needs to be inlined!
            // The clause needs to be replaced
            inlined = true;

            // N new clauses should be formed, where N is the number of clauses
            // associated with the inlined relation
            for (ast::Clause* inClause : rel->getClauses()) {
                // Form the replacement clause
                std::pair<NullableVector<ast::Literal*>, std::vector<ast::BinaryConstraint*>> inlineResult =
                        inlineBodyLiterals(atom, inClause);
                NullableVector<ast::Literal*> replacementBodyLiterals = inlineResult.first;
                std::vector<ast::BinaryConstraint*> currConstraints = inlineResult.second;

                if (!replacementBodyLiterals.isValid()) {
                    // Failed to unify the atoms! We can skip this one...
                    continue;
                }

                // Unification successful - the returned vector of literals represents one possible body
                // replacement We can add in the unification constraints as part of these literals.
                std::vector<ast::Literal*> bodyResult = replacementBodyLiterals.getVector();

                for (ast::BinaryConstraint* cons : currConstraints) {
                    bodyResult.push_back(cons);
                }

                addedBodyLiterals.push_back(bodyResult);
            }
        } else {
            // Not meant to be inlined, but a subargument may be
            NullableVector<ast::Atom*> atomVersions = getInlinedAtom(program, *atom);
            if (atomVersions.isValid()) {
                // Subnode needs to be inlined, so we have a vector of replacement atoms
                changed = true;
                for (ast::Atom* newAtom : atomVersions.getVector()) {
                    versions.push_back(newAtom);
                }
            }
        }
    } else if (ast::Negation* neg = dynamic_cast<ast::Negation*>(lit)) {
        // For negations, check the corresponding atom
        ast::Atom* atom = neg->getAtom();
        NullableVector<std::vector<ast::Literal*>> atomVersions = getInlinedLiteral(program, atom);

        if (atomVersions.isValid()) {
            // The atom can be inlined
            inlined = true;

            if (atomVersions.getVector().empty()) {
                // No clauses associated with the atom, so just becomes a true literal
                std::vector<ast::Literal*> trueBody;
                // TODO: change this to ast::Boolean
                trueBody.push_back(new ast::BinaryConstraint(BinaryConstraintOp::EQ,
                        std::make_unique<ast::NumberConstant>(1), std::make_unique<ast::NumberConstant>(1)));
                addedBodyLiterals.push_back(trueBody);
            } else {
                // Suppose an atom a(x) is inlined and has the following rules:
                //  - a(x) :- a11(x), a12(x).
                //  - a(x) :- a21(x), a22(x).
                // Then, a(x) <- (a11(x) ^ a12(x)) v (a21(x) ^ a22(x))
                //  => !a(x) <- (!a11(x) v !a12(x)) ^ (!a21(x) v !a22(x))
                //  => !a(x) <- (!a11(x) ^ !a21(x)) v (!a11(x) ^ !a22(x)) v ...
                // Essentially, produce every combination (m_1 ^ m_2 ^ ...) where m_i is a
                // negated literal in the ith rule of a.
                addedBodyLiterals = formNegatedLiterals(program, atom);
            }
        }
    } else if (ast::BinaryConstraint* constraint = dynamic_cast<ast::BinaryConstraint*>(lit)) {
        NullableVector<ast::Argument*> lhsVersions = getInlinedArgument(program, constraint->getLHS());
        if (lhsVersions.isValid()) {
            changed = true;
            for (ast::Argument* newLhs : lhsVersions.getVector()) {
                ast::Literal* newLit = new ast::BinaryConstraint(constraint->getOperator(),
                        std::unique_ptr<ast::Argument>(newLhs),
                        std::unique_ptr<ast::Argument>(constraint->getRHS()->clone()));
                versions.push_back(newLit);
            }
        } else {
            NullableVector<ast::Argument*> rhsVersions = getInlinedArgument(program, constraint->getRHS());
            if (rhsVersions.isValid()) {
                changed = true;
                for (ast::Argument* newRhs : rhsVersions.getVector()) {
                    ast::Literal* newLit = new ast::BinaryConstraint(constraint->getOperator(),
                            std::unique_ptr<ast::Argument>(constraint->getLHS()->clone()),
                            std::unique_ptr<ast::Argument>(newRhs));
                    versions.push_back(newLit);
                }
            }
        }
    }

    if (changed) {
        // Not inlined directly but found replacement literals
        // Rewrite these as single-literal bodies
        for (ast::Literal* version : versions) {
            std::vector<ast::Literal*> newBody;
            newBody.push_back(version);
            addedBodyLiterals.push_back(newBody);
        }
        inlined = true;
    }

    if (inlined) {
        return NullableVector<std::vector<ast::Literal*>>(addedBodyLiterals);
    } else {
        return NullableVector<std::vector<ast::Literal*>>();
    }
}

/**
 * Returns a list of clauses that should replace the given clause after one step of inlining.
 * If no inlining can occur, the list will only contain a clone of the original clause.
 */
std::vector<ast::Clause*> getInlinedClause(ast::Program& program, const ast::Clause& clause) {
    bool changed = false;
    std::vector<ast::Clause*> versions;

    // Try to inline things contained in the arguments of the head first.
    // E.g. `a(x, max y : { b(y) }) :- c(x).`, where b should be inlined.
    ast::Atom* head = clause.getHead();
    NullableVector<ast::Atom*> headVersions = getInlinedAtom(program, *head);
    if (headVersions.isValid()) {
        // The head atom can be inlined!
        changed = true;

        // Produce the new clauses with the replacement head atoms
        for (ast::Atom* newHead : headVersions.getVector()) {
            ast::Clause* newClause = new ast::Clause();
            newClause->setSrcLoc(clause.getSrcLoc());

            newClause->setHead(std::unique_ptr<ast::Atom>(newHead));

            // The body will remain unchanged
            for (ast::Literal* lit : clause.getBodyLiterals()) {
                newClause->addToBody(std::unique_ptr<ast::Literal>(lit->clone()));
            }

            versions.push_back(newClause);
        }
    }

    // Only perform one stage of inlining at a time.
    // If the head atoms did not need inlining, try inlining atoms nested in the body.
    if (!changed) {
        std::vector<ast::Literal*> bodyLiterals = clause.getBodyLiterals();
        for (size_t i = 0; i < bodyLiterals.size(); i++) {
            ast::Literal* currLit = bodyLiterals[i];

            // Three possible cases when trying to inline a literal:
            //  1) The literal itself may be directly inlined. In this case, the atom can be replaced
            //    with multiple different bodies, as the inlined atom may have several rules.
            //  2) Otherwise, the literal itself may not need to be inlined, but a subnode (e.g. an argument)
            //    may need to be inlined. In this case, an altered literal must replace the original.
            //    Again, several possible versions may exist, as the inlined relation may have several rules.
            //  3) The literal does not depend on any inlined relations, and so does not need to be changed.
            NullableVector<std::vector<ast::Literal*>> litVersions = getInlinedLiteral(program, currLit);

            if (litVersions.isValid()) {
                // Case 1 and 2: Inlining has occurred!
                changed = true;

                // The literal may be replaced with several different bodies.
                // Create a new clause for each possible version.
                std::vector<std::vector<ast::Literal*>> bodyVersions = litVersions.getVector();

                // Create the base clause with the current literal removed
                auto baseClause = std::unique_ptr<ast::Clause>(clause.cloneHead());
                for (ast::Literal* oldLit : bodyLiterals) {
                    if (currLit != oldLit) {
                        baseClause->addToBody(std::unique_ptr<ast::Literal>(oldLit->clone()));
                    }
                }

                for (std::vector<ast::Literal*> body : bodyVersions) {
                    ast::Clause* replacementClause = baseClause->clone();

                    // Add in the current set of literals replacing the inlined literal
                    // In Case 2, each body contains exactly one literal
                    for (ast::Literal* newLit : body) {
                        replacementClause->addToBody(std::unique_ptr<ast::Literal>(newLit));
                    }

                    versions.push_back(replacementClause);
                }
            }

            // Only replace at most one literal per iteration
            if (changed) {
                break;
            }
        }
    }

    if (!changed) {
        // Case 3: No inlining changes, so a clone of the original should be returned
        std::vector<ast::Clause*> ret;
        ret.push_back(clause.clone());
        return ret;
    } else {
        // Inlining changes, so return the replacement clauses.
        return versions;
    }
}

bool InlineRelationsTransformer::transform(ast::TranslationUnit& translationUnit) {
    bool changed = false;
    ast::Program& program = *translationUnit.getProgram();

    // Replace constants in the head of inlined clauses with (constrained) variables.
    // This is done to simplify atom unification, particularly when negations are involved.
    normaliseInlinedHeads(program);

    // Remove underscores in inlined atoms in the program to avoid issues during atom unification
    nameInlinedUnderscores(program);

    // Keep trying to inline things until we reach a fixed point.
    // Since we know there are no cyclic dependencies between inlined relations, this will necessarily
    // terminate.
    bool clausesChanged = true;
    while (clausesChanged) {
        std::set<ast::Clause*> clausesToDelete;
        clausesChanged = false;

        // Go through each relation in the program and check if we need to inline any of its clauses
        for (ast::Relation* rel : program.getRelations()) {
            // Skip if the relation is going to be inlined
            if (rel->isInline()) {
                continue;
            }

            // Go through the relation's clauses and try inlining them
            for (ast::Clause* clause : rel->getClauses()) {
                if (containsInlinedAtom(program, *clause)) {
                    // Generate the inlined versions of this clause - the clause will be replaced by these
                    std::vector<ast::Clause*> newClauses = getInlinedClause(program, *clause);

                    // Replace the clause with these equivalent versions
                    clausesToDelete.insert(clause);
                    for (ast::Clause* replacementClause : newClauses) {
                        program.appendClause(std::unique_ptr<ast::Clause>(replacementClause));
                    }

                    // We've changed the program this iteration
                    clausesChanged = true;
                    changed = true;
                }
            }
        }

        // Delete all clauses that were replaced
        for (const ast::Clause* clause : clausesToDelete) {
            program.removeClause(clause);
        }
    }

    return changed;
}

}  // end of namespace souffle
