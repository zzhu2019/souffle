/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file AstTransforms.cpp
 *
 * Implementation of AST transformation passes.
 *
 ***********************************************************************/

#include "AstTransforms.h"
#include "AstTypeAnalysis.h"
#include "AstUtils.h"
#include "AstVisitor.h"
#include "MagicSet.h"
#include "PrecedenceGraph.h"

namespace souffle {

void ResolveAliasesTransformer::resolveAliases(AstProgram& program) {
    // get all clauses
    std::vector<const AstClause*> clauses;
    visitDepthFirst(program, [&](const AstRelation& rel) {
        for (const auto& cur : rel.getClauses()) {
            clauses.push_back(cur);
        }
    });

    // clean all clauses
    for (const AstClause* cur : clauses) {
        // -- Step 1 --
        // get rid of aliases
        std::unique_ptr<AstClause> noAlias = resolveAliases(*cur);

        // clean up equalities
        std::unique_ptr<AstClause> cleaned = removeTrivialEquality(*noAlias);

        // -- Step 2 --
        // restore simple terms in atoms
        removeComplexTermsInAtoms(*cleaned);

        // exchange rule
        program.removeClause(cur);
        program.appendClause(std::move(cleaned));
    }
}

namespace {

/**
 * A utility class for the unification process required to eliminate
 * aliases. A substitution maps variables to terms and can be applied
 * as a transformation to AstArguments.
 */
class Substitution {
    // the type of map for storing mappings internally
    //   - variables are identified by their name (!)
    typedef std::map<std::string, std::unique_ptr<AstArgument>> map_t;

    /** The mapping of variables to terms (see type def above) */
    map_t map;

public:
    // -- Ctors / Dtors --

    Substitution(){};

    Substitution(const std::string& var, const AstArgument* arg) {
        map.insert(std::make_pair(var, std::unique_ptr<AstArgument>(arg->clone())));
    }

    virtual ~Substitution() = default;

    /**
     * Applies this substitution to the given argument and
     * returns a pointer to the modified argument.
     *
     * @param node the node to be transformed
     * @return a pointer to the modified or replaced node
     */
    virtual std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const {
        // create a substitution mapper
        struct M : public AstNodeMapper {
            const map_t& map;

            M(const map_t& map) : map(map) {}

            using AstNodeMapper::operator();

            std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
                // see whether it is a variable to be substituted
                if (auto var = dynamic_cast<AstVariable*>(node.get())) {
                    auto pos = map.find(var->getName());
                    if (pos != map.end()) {
                        return std::unique_ptr<AstNode>(pos->second->clone());
                    }
                }

                // otherwise - apply mapper recursively
                node->apply(*this);
                return node;
            }
        };

        // apply mapper
        return M(map)(std::move(node));
    }

    /**
     * A generic, type consistent wrapper of the transformation
     * operation above.
     */
    template <typename T>
    std::unique_ptr<T> operator()(std::unique_ptr<T> node) const {
        std::unique_ptr<AstNode> resPtr =
                (*this)(std::unique_ptr<AstNode>(static_cast<AstNode*>(node.release())));
        assert(dynamic_cast<T*>(resPtr.get()) && "Invalid node type mapping.");
        return std::unique_ptr<T>(dynamic_cast<T*>(resPtr.release()));
    }

    /**
     * Appends the given substitution to this substitution such that
     * this substitution has the same effect as applying this following
     * the given substitution in sequence.
     */
    void append(const Substitution& s) {
        // apply substitution on all current mappings
        for (auto& cur : map) {
            cur.second = s(std::move(cur.second));
        }

        // append uncovered variables to the end
        for (const auto& cur : s.map) {
            auto pos = map.find(cur.first);
            if (pos != map.end()) {
                continue;
            }
            map.insert(std::make_pair(cur.first, std::unique_ptr<AstArgument>(cur.second->clone())));
        }
    }

    /** A print function (for debugging) */
    void print(std::ostream& out) const {
        out << "{" << join(map, ",",
                              [](std::ostream& out,
                                      const std::pair<const std::string, std::unique_ptr<AstArgument>>& cur) {
                                  out << cur.first << " -> " << *cur.second;
                              })
            << "}";
    }

    friend std::ostream& operator<<(std::ostream& out, const Substitution& s) __attribute__((unused)) {
        s.print(out);
        return out;
    }
};

/**
 * An equality constraint between to AstArguments utilized by the
 * unification algorithm required by the alias resolution.
 */
struct Equation {
    /** The two terms to be equivalent */
    std::unique_ptr<AstArgument> lhs;
    std::unique_ptr<AstArgument> rhs;

    Equation(const AstArgument& lhs, const AstArgument& rhs)
            : lhs(std::unique_ptr<AstArgument>(lhs.clone())), rhs(std::unique_ptr<AstArgument>(rhs.clone())) {
    }

    Equation(const AstArgument* lhs, const AstArgument* rhs)
            : lhs(std::unique_ptr<AstArgument>(lhs->clone())),
              rhs(std::unique_ptr<AstArgument>(rhs->clone())) {}

    Equation(const Equation& other)
            : lhs(std::unique_ptr<AstArgument>(other.lhs->clone())),
              rhs(std::unique_ptr<AstArgument>(other.rhs->clone())) {}

    Equation(Equation&& other) : lhs(std::move(other.lhs)), rhs(std::move(other.rhs)) {}

    ~Equation() = default;

    /**
     * Applies the given substitution to both sides of the equation.
     */
    void apply(const Substitution& s) {
        lhs = s(std::move(lhs));
        rhs = s(std::move(rhs));
    }

    /** Enables equations to be printed (for debugging) */
    void print(std::ostream& out) const {
        out << *lhs << " = " << *rhs;
    }

    friend std::ostream& operator<<(std::ostream& out, const Equation& e) __attribute__((unused)) {
        e.print(out);
        return out;
    }
};
}  // namespace

std::unique_ptr<AstClause> ResolveAliasesTransformer::resolveAliases(const AstClause& clause) {
    /**
     * This alias analysis utilizes unification over the equality
     * constraints in clauses.
     */

    // -- utilities --

    // tests whether something is a ungrounded variable
    auto isVar = [&](const AstArgument& arg) { return dynamic_cast<const AstVariable*>(&arg); };

    // tests whether something is a record
    auto isRec = [&](const AstArgument& arg) { return dynamic_cast<const AstRecordInit*>(&arg); };

    // tests whether a value a occurs in a term b
    auto occurs = [](const AstArgument& a, const AstArgument& b) {
        bool res = false;
        visitDepthFirst(b, [&](const AstArgument& cur) { res = res || cur == a; });
        return res;
    };

    // I) extract equations
    std::vector<Equation> equations;
    visitDepthFirst(clause, [&](const AstConstraint& rel) {
        if (rel.getOperator() == BinaryConstraintOp::EQ) {
            equations.push_back(Equation(rel.getLHS(), rel.getRHS()));
        }
    });

    // II) compute unifying substitution
    Substitution substitution;

    // a utility for processing newly identified mappings
    auto newMapping = [&](const std::string& var, const AstArgument* term) {
        // found a new substitution
        Substitution newMapping(var, term);

        // apply substitution to all remaining equations
        for (auto& cur : equations) {
            cur.apply(newMapping);
        }

        // add mapping v -> t to substitution
        substitution.append(newMapping);
    };

    while (!equations.empty()) {
        // get next equation to compute
        Equation cur = equations.back();
        equations.pop_back();

        // shortcuts for left/right
        const AstArgument& a = *cur.lhs;
        const AstArgument& b = *cur.rhs;

        // #1:   t = t   => skip
        if (a == b) {
            continue;
        }

        // #2:   [..] = [..]   => decompose
        if (isRec(a) && isRec(b)) {
            // get arguments
            const auto& args_a = static_cast<const AstRecordInit&>(a).getArguments();
            const auto& args_b = static_cast<const AstRecordInit&>(b).getArguments();

            // make sure sizes are identical
            assert(args_a.size() == args_b.size());

            // create new equalities
            for (size_t i = 0; i < args_a.size(); ++i) {
                equations.push_back(Equation(args_a[i], args_b[i]));
            }
            continue;
        }

        // neither is a variable
        if (!isVar(a) && !isVar(b)) {
            continue;  // => nothing to do
        }

        // both are variables
        if (isVar(a) && isVar(b)) {
            // a new mapping is found
            auto& var = static_cast<const AstVariable&>(a);
            newMapping(var.getName(), &b);
            continue;
        }

        // #3:   t = v   => swap
        if (!isVar(a)) {
            equations.push_back(Equation(b, a));
            continue;
        }

        // now we know a is a variable
        assert(isVar(a));

        // we have   v = t
        const AstVariable& v = static_cast<const AstVariable&>(a);
        const AstArgument& t = b;

        // #4:   v occurs in t
        if (occurs(v, t)) {
            continue;
        }

        assert(!occurs(v, t));

        // add new maplet
        newMapping(v.getName(), &t);
    }

    // III) compute resulting clause
    return substitution(std::unique_ptr<AstClause>(clause.clone()));
}

std::unique_ptr<AstClause> ResolveAliasesTransformer::removeTrivialEquality(const AstClause& clause) {
    // finally: remove t = t constraints
    std::unique_ptr<AstClause> res(clause.cloneHead());
    for (AstLiteral* cur : clause.getBodyLiterals()) {
        // filter out t = t
        if (AstConstraint* rel = dynamic_cast<AstConstraint*>(cur)) {
            if (rel->getOperator() == BinaryConstraintOp::EQ) {
                if (*rel->getLHS() == *rel->getRHS()) {
                    continue;  // skip this one
                }
            }
        }
        res->addToBody(std::unique_ptr<AstLiteral>(cur->clone()));
    }

    // done
    return res;
}

void ResolveAliasesTransformer::removeComplexTermsInAtoms(AstClause& clause) {
    // restore temporary variables for expressions in atoms

    // get list of atoms
    std::vector<AstAtom*> atoms;
    for (AstLiteral* cur : clause.getBodyLiterals()) {
        if (AstAtom* arg = dynamic_cast<AstAtom*>(cur)) {
            atoms.push_back(arg);
        }
    }

    // find all binary operations in atoms
    std::vector<const AstArgument*> terms;
    for (const AstAtom* cur : atoms) {
        for (const AstArgument* arg : cur->getArguments()) {
            // only interested in functions
            if (!(dynamic_cast<const AstFunctor*>(arg))) {
                continue;
            }
            // add this one if not yet registered
            if (!any_of(terms, [&](const AstArgument* cur) { return *cur == *arg; })) {
                terms.push_back(arg);
            }
        }
    }

    // substitute them with variables (a real map would compare pointers)
    typedef std::vector<std::pair<std::unique_ptr<AstArgument>, std::unique_ptr<AstVariable>>>
            substitution_map;
    substitution_map map;

    int var_counter = 0;
    for (const AstArgument* arg : terms) {
        map.push_back(std::make_pair(std::unique_ptr<AstArgument>(arg->clone()),
                std::unique_ptr<AstVariable>(new AstVariable(" _tmp_" + toString(var_counter++)))));
    }

    // apply mapping to replace terms with variables
    struct Update : public AstNodeMapper {
        const substitution_map& map;
        Update(const substitution_map& map) : map(map) {}
        std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
            // check whether node needs to be replaced
            for (const auto& cur : map) {
                if (*cur.first == *node) {
                    return std::unique_ptr<AstNode>(cur.second->clone());
                }
            }
            // continue recursively
            node->apply(*this);
            return node;
        }
    };

    Update update(map);

    // update atoms
    for (AstAtom* atom : atoms) {
        atom->apply(update);
    }

    // add variable constraints to clause
    for (const auto& cur : map) {
        clause.addToBody(std::unique_ptr<AstLiteral>(
                new AstConstraint(BinaryConstraintOp::EQ, std::unique_ptr<AstArgument>(cur.second->clone()),
                        std::unique_ptr<AstArgument>(cur.first->clone()))));
    }
}

bool RemoveRelationCopiesTransformer::removeRelationCopies(AstProgram& program) {
    typedef std::map<AstRelationIdentifier, AstRelationIdentifier> alias_map;

    // tests whether something is a variable
    auto isVar = [&](const AstArgument& arg) { return dynamic_cast<const AstVariable*>(&arg); };

    // tests whether something is a record
    auto isRec = [&](const AstArgument& arg) { return dynamic_cast<const AstRecordInit*>(&arg); };

    // collect aliases
    alias_map isDirectAliasOf;

    // search for relations only defined by a single rule ..
    for (AstRelation* rel : program.getRelations()) {
        if (!rel->isComputed() && rel->getClauses().size() == 1u) {
            // .. of shape r(x,y,..) :- s(x,y,..)
            AstClause* cl = rel->getClause(0);
            if (!cl->isFact() && cl->getBodySize() == 1u && cl->getAtoms().size() == 1u) {
                AstAtom* atom = cl->getAtoms()[0];
                if (equal_targets(cl->getHead()->getArguments(), atom->getArguments())) {
                    // we have a match but have to check that all arguments are either
                    // variables or records containing variables
                    bool onlyVars = true;
                    auto args = cl->getHead()->getArguments();
                    while (!args.empty()) {
                        const auto& cur = args.back();
                        args.pop_back();
                        if (!isVar(*cur)) {
                            if (isRec(*cur)) {
                                // records are decomposed and their arguments are checked
                                const auto& rec_args = static_cast<const AstRecordInit&>(*cur).getArguments();
                                for (size_t i = 0; i < rec_args.size(); ++i) {
                                    args.push_back(rec_args[i]);
                                }
                            } else {
                                onlyVars = false;
                                break;
                            }
                        }
                    }
                    if (onlyVars) {
                        // all arguments are either variables or records containing variables
                        isDirectAliasOf[cl->getHead()->getName()] = atom->getName();
                    }
                }
            }
        }
    }

    // map each relation to its ultimate alias (could be transitive)
    alias_map isAliasOf;

    // track any copy cycles; cyclic rules are effectively empty
    std::set<AstRelationIdentifier> cycle_reps;

    for (std::pair<AstRelationIdentifier, AstRelationIdentifier> cur : isDirectAliasOf) {
        // compute replacement

        std::set<AstRelationIdentifier> visited;
        visited.insert(cur.first);
        visited.insert(cur.second);

        auto pos = isDirectAliasOf.find(cur.second);
        while (pos != isDirectAliasOf.end()) {
            if (visited.count(pos->second)) {
                cycle_reps.insert(cur.second);
                break;
            }
            cur.second = pos->second;
            pos = isDirectAliasOf.find(cur.second);
        }
        isAliasOf[cur.first] = cur.second;
    }

    if (isAliasOf.empty()) {
        return false;
    }

    // replace usage of relations according to alias map
    visitDepthFirst(program, [&](const AstAtom& atom) {
        auto pos = isAliasOf.find(atom.getName());
        if (pos != isAliasOf.end()) {
            const_cast<AstAtom&>(atom).setName(pos->second);
        }
    });

    // break remaining cycles
    for (const auto& rep : cycle_reps) {
        auto rel = program.getRelation(rep);
        rel->removeClause(rel->getClause(0));
    }

    // remove unused relations
    for (const auto& cur : isAliasOf) {
        if (!cycle_reps.count(cur.first)) {
            program.removeRelation(program.getRelation(cur.first)->getName());
        }
    }

    return true;
}

bool UniqueAggregationVariablesTransformer::transform(AstTranslationUnit& translationUnit) {
    bool changed = false;

    // make variables in aggregates unique
    int aggNumber = 0;
    visitDepthFirstPostOrder(*translationUnit.getProgram(), [&](const AstAggregator& agg) {

        // only applicable for aggregates with target expression
        if (!agg.getTargetExpression()) {
            return;
        }

        // get all variables in the target expression
        std::set<std::string> names;
        visitDepthFirst(
                *agg.getTargetExpression(), [&](const AstVariable& var) { names.insert(var.getName()); });

        // rename them
        visitDepthFirst(agg, [&](const AstVariable& var) {
            auto pos = names.find(var.getName());
            if (pos == names.end()) {
                return;
            }
            const_cast<AstVariable&>(var).setName(" " + var.getName() + toString(aggNumber));
            changed = true;
        });

        // increment aggregation number
        aggNumber++;
    });
    return changed;
}

bool MaterializeAggregationQueriesTransformer::materializeAggregationQueries(
        AstTranslationUnit& translationUnit) {
    bool changed = false;

    AstProgram& program = *translationUnit.getProgram();
    const TypeEnvironment& env = translationUnit.getAnalysis<TypeEnvironmentAnalysis>()->getTypeEnvironment();

    // if an aggregator has a body consisting of more than an atom => create new relation
    int counter = 0;
    visitDepthFirst(program, [&](const AstClause& clause) {
        visitDepthFirstPostOrder(clause, [&](const AstAggregator& agg) {
            // check whether a materialization is required
            if (!needsMaterializedRelation(agg)) {
                return;
            }

            changed = true;

            // for more body literals: create a new relation
            std::set<std::string> vars;
            visitDepthFirst(agg, [&](const AstVariable& var) { vars.insert(var.getName()); });

            // -- create a new clause --

            auto relName = "__agg_rel_" + toString(counter++);
            while (program.getRelation(relName) != nullptr) {
                relName = "__agg_rel_" + toString(counter++);
            }

            AstAtom* head = new AstAtom();
            head->setName(relName);
            std::vector<bool> symbolArguments;
            for (const auto& cur : vars) {
                head->addArgument(std::unique_ptr<AstArgument>(new AstVariable(cur)));
            }

            AstClause* aggClause = new AstClause();
            aggClause->setHead(std::unique_ptr<AstAtom>(head));
            for (const auto& cur : agg.getBodyLiterals()) {
                aggClause->addToBody(std::unique_ptr<AstLiteral>(cur->clone()));
            }

            // instantiate unnamed variables in count operations
            if (agg.getOperator() == AstAggregator::count) {
                int count = 0;
                for (const auto& cur : aggClause->getBodyLiterals()) {
                    cur->apply(
                            makeLambdaMapper([&](std::unique_ptr<AstNode> node) -> std::unique_ptr<AstNode> {
                                // check whether it is a unnamed variable
                                AstUnnamedVariable* var = dynamic_cast<AstUnnamedVariable*>(node.get());
                                if (!var) {
                                    return node;
                                }

                                // replace by variable
                                auto name = " _" + toString(count++);
                                auto res = new AstVariable(name);

                                // extend head
                                head->addArgument(std::unique_ptr<AstArgument>(res->clone()));

                                // return replacement
                                return std::unique_ptr<AstNode>(res);
                            }));
                }
            }

            // -- build relation --

            AstRelation* rel = new AstRelation();
            rel->setName(relName);
            // add attributes
            std::map<const AstArgument*, TypeSet> argTypes =
                    TypeAnalysis::analyseTypes(env, *aggClause, &program);
            for (const auto& cur : head->getArguments()) {
                rel->addAttribute(std::unique_ptr<AstAttribute>(new AstAttribute(
                        toString(*cur), (isNumberType(argTypes[cur])) ? "number" : "symbol")));
            }

            rel->addClause(std::unique_ptr<AstClause>(aggClause));
            program.appendRelation(std::unique_ptr<AstRelation>(rel));

            // -- update aggregate --
            AstAtom* aggAtom = head->clone();

            // count the usage of variables in the clause
            // outside of aggregates. Note that the visitor
            // is exhaustive hence double counting occurs
            // which needs to be deducted for variables inside
            // the aggregators and variables in the expression
            // of aggregate need to be added. Counter is zero
            // if the variable is local to the aggregate
            std::map<std::string, int> varCtr;
            visitDepthFirst(clause, [&](const AstArgument& arg) {
                if (const AstAggregator* a = dynamic_cast<const AstAggregator*>(&arg)) {
                    visitDepthFirst(arg, [&](const AstVariable& var) { varCtr[var.getName()]--; });
                    if (a->getTargetExpression() != nullptr) {
                        visitDepthFirst(*a->getTargetExpression(),
                                [&](const AstVariable& var) { varCtr[var.getName()]++; });
                    }
                } else {
                    visitDepthFirst(arg, [&](const AstVariable& var) { varCtr[var.getName()]++; });
                }
            });
            for (size_t i = 0; i < aggAtom->getArity(); i++) {
                if (AstVariable* var = dynamic_cast<AstVariable*>(aggAtom->getArgument(i))) {
                    // replace local variable by underscore if local
                    if (varCtr[var->getName()] == 0) {
                        aggAtom->setArgument(i, std::unique_ptr<AstArgument>(new AstUnnamedVariable()));
                    }
                }
            }
            const_cast<AstAggregator&>(agg).clearBodyLiterals();
            const_cast<AstAggregator&>(agg).addBodyLiteral(std::unique_ptr<AstLiteral>(aggAtom));
        });
    });

    return changed;
}

bool MaterializeAggregationQueriesTransformer::needsMaterializedRelation(const AstAggregator& agg) {
    // everything with more than 1 body literal => materialize
    if (agg.getBodyLiterals().size() > 1) {
        return true;
    }

    // inspect remaining atom more closely
    const AstAtom* atom = dynamic_cast<const AstAtom*>(agg.getBodyLiterals()[0]);
    if(!atom) {
      return false;
    }
    assert(atom && "Body of aggregate is not containing an atom!");

    // if the same variable occurs several times => materialize
    bool duplicates = false;
    std::set<std::string> vars;
    visitDepthFirst(*atom,
            [&](const AstVariable& var) { duplicates = duplicates | !vars.insert(var.getName()).second; });

    // if there are duplicates a materialization is required
    if (duplicates) {
        return true;
    }

    // for all others the materialization can be skipped
    return false;
}

bool EvaluateConstantAggregatesTransformer::transform(AstTranslationUnit& translationUnit) {
  // TODO: need some way of checking if things have changed

  // TODO: commenting
  struct M : public AstNodeMapper {
    M() {}
    std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
      if(AstAggregator* aggr = dynamic_cast<AstAggregator*>(node.get())) {
        AstAggregator::Op op = aggr->getOperator();
        if(op == AstAggregator::min || op == AstAggregator::max) {
          const AstArgument* arg = aggr->getTargetExpression();
          if (arg != nullptr && dynamic_cast<const AstConstant*>(arg)) {
            return std::unique_ptr<AstNode>(arg->clone());
          }
        } else if (op == AstAggregator::count) {
          // TODO: check correctness of this part!!! Mainly:
          //        - does this check correctly that there is no body atom?
          //        - should it always be 1? could be 0? in inlining's case, yes
          if (!dynamic_cast<const AstAtom*>(aggr->getBodyLiterals()[0])) {
            return std::unique_ptr<AstNode>(new AstNumberConstant(1));
          }
        } else if (op == AstAggregator::sum) {
          const AstArgument* arg = aggr->getTargetExpression();
          if (arg != nullptr && dynamic_cast<const AstConstant*>(arg)) {
            return std::unique_ptr<AstNode>(arg->clone());
          }
        } else {
          // TODO: correctness?
          assert(false && "unhandled aggregate operator type");
        }
      }

      node->apply(*this);
      return node;
    }
  };

  M update;
  translationUnit.getProgram()->apply(update);

  return true;
}

bool RemoveEmptyRelationsTransformer::removeEmptyRelations(AstTranslationUnit& translationUnit) {
    AstProgram& program = *translationUnit.getProgram();
    bool changed = false;
    for (auto rel : program.getRelations()) {
        if (rel->clauseSize() == 0 && !rel->isInput()) {
            removeEmptyRelationUses(translationUnit, rel);

            bool usedInAggregate = false;
            visitDepthFirst(program, [&](const AstAggregator& agg) {
                for (const auto lit : agg.getBodyLiterals()) {
                    visitDepthFirst(*lit, [&](const AstAtom& atom) {
                        if (getAtomRelation(&atom, &program) == rel) {
                            usedInAggregate = true;
                        }
                    });
                }
            });

            if (!usedInAggregate && !rel->isComputed()) {
                program.removeRelation(rel->getName());
            }
            changed = true;
        }
    }
    return changed;
}

void RemoveEmptyRelationsTransformer::removeEmptyRelationUses(
        AstTranslationUnit& translationUnit, AstRelation* emptyRelation) {
    AstProgram& program = *translationUnit.getProgram();

    //
    // (1) drop rules from the program that have empty relations in their bodies.
    // (2) drop negations of empty relations
    //
    // get all clauses
    std::vector<const AstClause*> clauses;
    visitDepthFirst(program, [&](const AstClause& cur) { clauses.push_back(&cur); });

    // clean all clauses
    for (const AstClause* cl : clauses) {
        // check for an atom whose relation is the empty relation

        bool removed = false;
        ;
        for (AstLiteral* lit : cl->getBodyLiterals()) {
            if (AstAtom* arg = dynamic_cast<AstAtom*>(lit)) {
                if (getAtomRelation(arg, &program) == emptyRelation) {
                    program.removeClause(cl);
                    removed = true;
                    break;
                }
            }
        }

        if (!removed) {
            // check whether a negation with empty relations exists

            bool rewrite = false;
            for (AstLiteral* lit : cl->getBodyLiterals()) {
                if (AstNegation* neg = dynamic_cast<AstNegation*>(lit)) {
                    if (getAtomRelation(neg->getAtom(), &program) == emptyRelation) {
                        rewrite = true;
                        break;
                    }
                }
            }

            if (rewrite) {
                // clone clause without negation for empty relations

                auto res = std::unique_ptr<AstClause>(cl->cloneHead());

                for (AstLiteral* lit : cl->getBodyLiterals()) {
                    if (AstNegation* neg = dynamic_cast<AstNegation*>(lit)) {
                        if (getAtomRelation(neg->getAtom(), &program) != emptyRelation) {
                            res->addToBody(std::unique_ptr<AstLiteral>(lit->clone()));
                        }
                    } else {
                        res->addToBody(std::unique_ptr<AstLiteral>(lit->clone()));
                    }
                }

                program.removeClause(cl);
                program.appendClause(std::move(res));
            }
        }
    }
}

bool RemoveRedundantRelationsTransformer::transform(AstTranslationUnit& translationUnit) {
    bool changed = false;
    RedundantRelations* redundantRelationsAnalysis = translationUnit.getAnalysis<RedundantRelations>();
    const std::set<const AstRelation*>& redundantRelations =
            redundantRelationsAnalysis->getRedundantRelations();
    if (!redundantRelations.empty()) {
        for (auto rel : redundantRelations) {
            translationUnit.getProgram()->removeRelation(rel->getName());
            changed = true;
        }
    }
    return changed;
}

// TODO: Get rid of these
bool isConstant(AstArgument* arg) {
  return (dynamic_cast<AstConstant*>(arg));
}

bool isRecord(AstArgument* arg) {
  return (dynamic_cast<AstRecordInit*>(arg));
}

bool isFunctor(AstArgument* arg) {
  return (dynamic_cast<AstFunctor*>(arg));
}

bool isCounter(AstArgument* arg) {
  return (dynamic_cast<AstCounter*>(arg));
}

// TODO LIST:
/*
1 - underscores -> variables
2 - alpha reductions
3 - get rid of inner arguments for aggregators
*/

// TODO: not really Martelli-Montanari algo anymore?
// Reduces a vector of substitutions based on Martelli-Montanari algorithm
bool reduceSubstitution(std::vector<std::pair<AstArgument*, AstArgument*>>& sub) {
  // Type-Checking functions
  // TODO: is [&] needed?
  //auto isConstant = [&](AstArgument* arg) {return (dynamic_cast<AstConstant*>(arg));};
  //auto isFunctor = [&](AstArgument* arg) {return (dynamic_cast<AstFunctor*>(arg));};
  // auto isVariable = [&](AstArgument* arg) {return ((dynamic_cast<AstVariable*>(arg)) || isFunctor(arg));};

  // Keep trying to reduce the substitutions until we reach a fixed point
  bool done = false;
  while (!done) {
    done = true;

    // Try reducing each pair
    for(int i = 0; i < sub.size(); i++) {
      auto currPair = sub[i];
      AstArgument* lhs = currPair.first;
      AstArgument* rhs = currPair.second;

      // TODO: see doubles_new.dl file for what's left
      // AstArgument Cases:
      //  0 - AstVariable
      //  1 - AstUnnamedVariable (_) -> change all to variables to reduce this case -> need to rename everything
      //  2 - AstCounter ($) -> find out what this does --> UHH??
      //  3 - AstConstant -> as straightforward as variables
      //  4 - AstFunctor -> can this just be direct? or more thought into this?
      //  5 - AstRecordInit -> this should just be pairwise equality right? think about this...
      //  6 - AstTypeCast -> find out what this does ;; SIDE NOTE: no idea how this looks actually but should work immediately
      //  7 - AstAggregator -> complicated! think about this - perhaps equate arguments on a lower level

      // TODO: throw this error properly
      assert(!isCounter(lhs) && !isCounter(rhs) && "ERROR: CONTAINS `$`");

      if (*lhs == *rhs) {
        // get rid of `x = x`
        sub.erase(sub.begin() + i);
        done = false;
      } else if (isConstant(lhs) && isConstant(rhs)) {
        // both are constants but not equal (prev case => !=)
        // failed to unify!
        return false;
      } else if (isRecord(lhs) && isRecord(rhs)) {
        // handle equated records
        // note: we will not deal with the case where only one side is
        // a record and the other is a variable
        // TODO: think about the case where record = constant, etc.
        std::vector<AstArgument*> lhsArgs = static_cast<AstRecordInit*>(lhs)->getArguments();
        std::vector<AstArgument*> rhsArgs = static_cast<AstRecordInit*>(rhs)->getArguments();
        if (lhsArgs.size() != rhsArgs.size()) {
          return false;
        }
        for (int i = 0; i < lhsArgs.size(); i++) {
          sub.push_back(std::make_pair(lhsArgs[i], rhsArgs[i]));
        }
        sub.erase(sub.begin() + i);
        done = false;
      } else if ((isRecord(lhs) && isConstant(rhs))||(isConstant(lhs) && isRecord(rhs))) {
        // a record =/= a constant
        // TODO: is this correct? is this even possible?
        return false;
      }

      // // TODO: see doubles.dl file for what's left
      // // AstArgument Cases:
      // //  0 - AstVariable [x]
      // //  1 - AstUnnamedVariable (_) -> change all to variables to reduce this case -> need to rename everything
      // //  2 - AstCounter ($) -> find out what this does --> UHH??
      // //  3 - AstConstant -> as straightforward as variables [x]
      // //  4 - AstFunctor -> can this just be direct? or more thought into this? [x]
      // //  5 - AstRecordInit -> this should just be pairwise equality right? think about this... [x]
      // //  6 - AstTypeCast -> find out what this does ;; SIDE NOTE: no idea how this looks actually but should work immediately [x]
      // //  7 - AstAggregator -> complicated! think about this - perhaps equate arguments on a lower level
      //
      // // Unify the arguments
      // // If returnCode = -1, then failed to unify
      // // If return code = 0, then unified and theres no change
      // // If return code = 1, then unified and there was a change
      // // int returnCode = unifyArguments(firstTerm, secondTerm, sub);
      //
      // // TODO: change to LHS and RHS
      // if (*firstTerm == *secondTerm) {
      //   // get rid of `x = x`
      //   sub.erase(sub.begin() + i);
      //   done = false;
      // } else if (isRecord(firstTerm) && isRecord(secondTerm)) {
      //   std::vector<AstArgument*> firstRecArgs = static_cast<AstRecordInit*>(firstTerm)->getArguments();
      //   std::vector<AstArgument*> secondRecArgs = static_cast<AstRecordInit*>(secondTerm)->getArguments();
      //   if (firstRecArgs.size() != secondRecArgs.size()) {
      //     return false;
      //   }
      //   for (int i = 0; i < firstRecArgs.size(); i++) {
      //     sub.push_back(std::make_pair(firstRecArgs[i], secondRecArgs[i]));
      //   }
      //   sub.erase(sub.begin() + i);
      //   done = false;
      // } else if (isConstant(firstTerm) && isConstant(secondTerm)) {
        // // both are constants but non-equal (prev case => !=)
        // // failed to unify!
        // return false;
      // } else if (!isVariable(firstTerm) && isVariable(secondTerm)) {
      //   // rewrite `t=x` as `x=t`
      //   sub[i] = std::make_pair(secondTerm, firstTerm);
      //   done = false;
      // } else if (isVariable(firstTerm) && (isVariable(secondTerm) || isConstant(secondTerm))) {
      //   // // variable elimination when repeated
      //   // for(int j = 0; j < sub.size(); j++) {
      //   //   // TODO: functions here too!
      //   //   if(j == i) {
      //   //     continue;
      //   //   }
      //   //
      //   //   if(isVariable(sub[j].first) && (*sub[j].first == *firstTerm)){
      //   //     if(*sub[j].second == *secondTerm) {
      //   //       sub.erase(sub.begin() + i);
      //   //     } else {
      //   //       sub[j].first = secondTerm;
      //   //     }
      //   //     done = false;
      //   //   } else if(isVariable(sub[j].second) && (*sub[j].second == *firstTerm)) {
      //   //     if(*sub[j].first == *secondTerm) {
      //   //       sub.erase(sub.begin() + i);
      //   //     } else {
      //   //       sub[j].second = secondTerm;
      //   //     }
      //   //     done = false;
      //   //   }
      //   // }
      // }
    }
  }

  return true;
}

// Returns the pair (v, p), where p indicates whether unification has failed,
// and v is the vector of substitutions to unify the two given atoms if successful.
// Assumes that the atoms are both of the same relation.
std::pair<std::vector<std::pair<AstArgument*, AstArgument*>>, bool> unifyAtoms(AstAtom* first, AstAtom* second) {
  std::pair<std::vector<std::pair<AstArgument*, AstArgument*>>, bool> result;
  std::vector<std::pair<AstArgument*, AstArgument*>> substitution;

  std::vector<AstArgument*> firstArgs = first->getArguments();
  std::vector<AstArgument*> secondArgs = second->getArguments();

  // Create the initial unification equalities
  for(int i = 0; i < firstArgs.size(); i++) {
    substitution.push_back(std::make_pair(firstArgs[i], secondArgs[i]));
  }

  // Reduce the substitutions
  bool success = reduceSubstitution(substitution);

  result = make_pair(substitution, success);
  return result;
}

std::pair<std::vector<AstLiteral*>, bool> inlineBodyLiterals(AstAtom* atom, AstClause* atomClause) {
  bool changed = false;
  std::vector<AstLiteral*> addedLits;

  // Get the constraints needed to unify the two atoms
  std::pair<std::vector<std::pair<AstArgument*, AstArgument*>>, bool> res = unifyAtoms(atomClause->getHead(), atom);
  if(res.second) {
    changed = true;
    for(auto pair : res.first) {
      addedLits.push_back(new AstConstraint(BinaryConstraintOp::EQ,
              std::unique_ptr<AstArgument>(pair.first->clone()),
              std::unique_ptr<AstArgument>(pair.second->clone())));
    }

    // Add in the body of the current clause of the inlined atom
    for(AstLiteral* lit : atomClause->getBodyLiterals()) {
      addedLits.push_back(lit->clone());
    }
  }

  return std::make_pair(addedLits, changed);
}

// Returns an equivalent clause after inlining the given atom in the clause wrt atomClause
// TODO: fix up
AstClause* inlineClause(const AstClause& clause, AstAtom* atom, AstClause* atomClause) {
  // TODO: what if variable names are non-unique across atoms being unified? - \alpha reduction time later!
  //      -- see apply function?

  // Get the constraints needed to unify the two atoms
  std::pair<std::vector<std::pair<AstArgument*, AstArgument*>>, bool> res = unifyAtoms(atomClause->getHead(), atom);

  // NOTE: DEBUGGING
  //std::cout << "NEW UNIFICATION: " << *atomClause->getHead() << " " << *atom << std::endl;
  //for(auto argPair : res.first) {
  //  std::cout << *argPair.first << " " << *argPair.second << "\t";
  //}
  //std::cout << std::endl;

  if(!res.second) {
    // Could not unify!
    // TODO: how to proceed? means an error has occurred and this formed clause is b a d
    //std::cout << "BROKEN UNIFICATION ^ ^ ^" << std::endl;
    return nullptr;
  }

  // Start creating the replacement clause
  AstClause* replacementClause = clause.cloneHead();

  // Add in all the literals except the atom to be inlined
  for(AstLiteral* lit : clause.getBodyLiterals()) {
    if(atom != lit) {
      replacementClause->addToBody(std::unique_ptr<AstLiteral>(lit->clone()));
    }
  }

  // Add in the body of the current clause of the inlined atom
  for(AstLiteral* lit : atomClause->getBodyLiterals()) {
    replacementClause->addToBody(std::unique_ptr<AstLiteral>(lit->clone()));
  }

  // Add in the substitutions as constraints to unify
  // TODO: need alpha reduction to make sure everything doesn't break
  for(auto pair : res.first) {
    AstConstraint* subCons = new AstConstraint(BinaryConstraintOp::EQ,
            std::unique_ptr<AstArgument>(pair.first->clone()),
            std::unique_ptr<AstArgument>(pair.second->clone()));

    replacementClause->addToBody(std::unique_ptr<AstLiteral>(subCons));
  }

  return replacementClause;
}

// Renames all arguments contained in inlined relations to prevent conflict
void renameInlinedArguments(AstProgram& program) {
  // construct mapping to rename arguments
  struct VariableRenamer : public AstNodeMapper {
      VariableRenamer() {}
      std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
          if (dynamic_cast<AstLiteral*>(node.get())){
            // Apply to the literal's arguments
            node->apply(*this);
            return node;
          } else if(AstVariable* var = dynamic_cast<AstVariable*>(node.get())){
            // Rename the variable
            AstVariable* newVar = var->clone();
            newVar->setName("<inlined_" + var->getName() + ">");
            return std::unique_ptr<AstNode>(newVar);
          }
          return node;
      }
  };

  // TODO: fix this so that it fits the proper style?
  VariableRenamer renameVar;
  for(AstRelation* rel : program.getRelations()) {
    if(rel->isInline()) {
      for(AstClause* clause : rel->getClauses()) {
        clause->apply(renameVar);
      }
    }
  }
}

int underscoreCount = 0;

// Removes all underscores in inlined relations
void removeInlinedUnderscores(AstProgram& program) {
  // struct UpdateUnderscores : public AstNodeMapper;
  //
  // // construct mapping to name underscores
  // struct UnderscoreNamer : public AstNodeMapper {
  //     AstProgram& program;
  //     UpdateUnderscores* updater;
  //     UnderscoreNamer(AstProgram& program, UpdateUnderscores* updater) : program(program), updater(updater) {}
  //     std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
  //         if(AstUnnamedVariable* var = dynamic_cast<AstUnnamedVariable*>(node.get())){
  //           // Give a name to the underscord variable
  //           std::stringstream newVarName;
  //           newVarName << "<underscore_" << underscoreCount++ << ">";
  //           AstVariable* newVar = new AstVariable(newVarName.str());
  //           return std::unique_ptr<AstNode>(newVar);
  //         }
  //         return node->apply(*updater);
  //     }
  // };

  // // TODO: fix this so that it fits the proper style?
  // struct UpdateUnderscores : public AstNodeMapper {
  //   AstProgram& program;
  //   UpdateUnderscores(AstProgram& program) : program(program) {}
  //   std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
  //     if(AstLiteral* lit = dynamic_cast<AstLiteral*>(node.get())) {
  //       if(lit->getAtom() != nullptr) {
  //         if(program.getRelation(lit->getAtom()->getName())->isInline()){
  //           UnderscoreNamer m(program);
  //           node->apply(m);
  //           return node;
  //         }
  //       }
  //     }
  //
  //     node->apply(*this);
  //     return node;
  //   }
  // };

  // TODO: NEED to only change the name of inlined stuff
  struct UnderscoreNamer : public AstNodeMapper {
    UnderscoreNamer() {}
    std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
      static int underscoreCount = 0;
      // std::cout << " performing replacement on " << *node << std::endl;
      if (AstUnnamedVariable* var = dynamic_cast<AstUnnamedVariable*>(node.get())) {
        // Give a name to the underscored variable
        std::stringstream newVarName;
        newVarName << "<underscore_" << underscoreCount++ << ">";
        AstVariable* newVar = new AstVariable(newVarName.str());
        // TODO: check if should be applied beforehand
        return std::unique_ptr<AstNode>(newVar);
      }
      node->apply(*this);
      return node;
    }
  };

  UnderscoreNamer update;

  // TODO: want to visit every atom and apply this to it
  visitDepthFirst(program, [&](const AstAtom& a) {
    // TODO: THIS IS HORRIBLE - GETTING RID OF THE CONST QUALIFIER
    // TODO: but need a way to do this without having a const to begin with...
    AstAtom* atom = (AstAtom*)((void *) &a);
    atom->apply(update);
  });
}

AstClause* replaceClauseLiteral(const AstClause& clause, int index, AstLiteral* lit) {
  AstClause* newClause = clause.clone();
  auto bodyLiterals = clause.getBodyLiterals();
  for(int i = 0; i < bodyLiterals.size(); i++) {
    if(i == index) {
      newClause->addToBody(std::unique_ptr<AstLiteral>(lit));
    } else {
      newClause->addToBody(std::unique_ptr<AstLiteral>(bodyLiterals[i]->clone()));
    }
  }
  return newClause;
}

AstLiteral* negateLiteral(AstLiteral* lit) {
  if(AstAtom* atom = dynamic_cast<AstAtom*>(lit)) {
    AstNegation* neg = new AstNegation(std::unique_ptr<AstAtom>(atom->clone()));
    return neg;
  } else if (AstNegation* neg = dynamic_cast<AstNegation*>(lit)) {
    AstAtom* atom = neg->getAtom()->clone();
    return atom;
  } else if (AstConstraint* cons = dynamic_cast<AstConstraint*>(lit)) {
    AstConstraint* newCons = cons->clone();
    newCons->negate();
    return newCons;
  } else {
    assert(false && "this should've never been reached");
    return lit; // TODO: THIS SHOULD NEVER BE REACHED
  }
}

  std::vector<std::vector<AstLiteral*>> combineNegatedLiterals(std::vector<std::vector<AstLiteral*>> litGroups) {
    std::vector<std::vector<AstLiteral*>> res;

    std::vector<AstLiteral*> litGroup = litGroups[0];
    if(litGroups.size() == 1) {
      // TODO: better code style for vector instantiation? is this even correct vector usage?
      for(int i = 0; i < litGroup.size(); i++) {
        std::vector<AstLiteral*> currVec;
        currVec.push_back(negateLiteral(litGroup[i]));
        res.push_back(currVec);
        // delete litGroup[i]; TODO: should i?
      }
      return res;
    }

    std::vector<std::vector<AstLiteral*>> combinedRHS = combineNegatedLiterals(std::vector<std::vector<AstLiteral*>>(litGroups.begin()+1, litGroups.end()));
    for(AstLiteral* lhsLit : litGroup) {
      for(std::vector<AstLiteral*> rhsVec : combinedRHS) {
        std::vector<AstLiteral*> currVec;
        currVec.push_back(negateLiteral(lhsLit));

        for(AstLiteral* lit : rhsVec) {
          currVec.push_back(lit);
        }

        res.push_back(currVec);
      }
    }

    return res;
  }

std::pair<std::vector<AstArgument*>, bool> getInlinedArgument(AstProgram& program, const AstArgument* arg);
std::pair<std::vector<AstAtom*>, bool> getInlinedAtom(AstProgram& program, AstAtom& atom);
std::pair<std::vector<AstLiteral*>, bool> getInlinedLiteral(AstProgram& program, AstLiteral& literal);

// TODO: NON-ATOM LITERALS ARE NOT HANLDED CORRECTLY - CHECK THROUGH THIS (e.g. negations)
// ARGUMENTS: <<<inline body literals versions to add>, <inlined?>> <<replacement literal versions> <replacements exist?>>>
std::pair<std::pair<std::vector<std::vector<AstLiteral*>>, bool>, std::pair<std::vector<AstLiteral*>, bool>> getInlinedLiteral(AstProgram& program, AstLiteral* lit) {
  bool inlined = false;
  bool changed = false;
  std::vector<std::vector<AstLiteral*>> addedBodyLiterals;
  std::vector<AstLiteral*> versions;

  if(AstAtom* atom = dynamic_cast<AstAtom*>(lit)) {
    // atom

    // Check if this atom is meant to be inlined
    AstRelation* rel = program.getRelation(atom->getName());
    if (rel->isInline()) {
      // We found an atom in the clause that needs to be inlined!
      // The clause needs to be replaced
      inlined = true;

      // N new clauses should be formed, where N is the number of clauses
      // associated with the inlined relation
      for (AstClause* inClause : rel->getClauses()) {
        // Form the replacement clause
        std::pair<std::vector<AstLiteral*>, bool> replacementBodyLiterals = inlineBodyLiterals(atom, inClause);
        if(!replacementBodyLiterals.second) {
          // Failed to unify
          continue;
        }
        addedBodyLiterals.push_back(replacementBodyLiterals.first);
      }
    } else {
      // check if its arguments need to be changed
      std::pair<std::vector<AstAtom*>, bool> atomVersions = getInlinedAtom(program, *atom);
      if(atomVersions.second) {
        changed = true;
        for(AstAtom* newAtom : atomVersions.first) {
          versions.push_back(newAtom);
        }
      }
    }
  } else if (AstNegation* neg = dynamic_cast<AstNegation*>(lit)) {
    // negation

    // negated atoms that are supposed to be inlined are dealt with earlier
    // TODO: DO THAT HANDLING

    // check if its arguments need to be changed
    AstAtom* atom = neg->getAtom();
    auto atomVersions = getInlinedLiteral(program, atom);
    if(atomVersions.first.second) {
      inlined = true;

      // c(x) :- b(x), !a(x) ==> c(x) :- b(x), !a1(x), !a2(x).
      // TODO: check the unified atoms whether you can `and` them together without problems
      addedBodyLiterals = combineNegatedLiterals(atomVersions.first.first);
    } else if (atomVersions.second.second) {
      changed = true;
      for(AstLiteral* nextLit : atomVersions.second.first) {
        // TODO: is this astliteral return correct?
        AstAtom* newAtom = dynamic_cast<AstAtom*>(nextLit);
        assert("atom expected" && newAtom); // TODO: fix this up
        AstLiteral* newNeg = new AstNegation(std::unique_ptr<AstAtom>(newAtom));
        versions.push_back(newNeg);
      }
    }
  } else {
    // constraint
    AstConstraint* constraint = dynamic_cast<AstConstraint*>(lit);
    std::pair<std::vector<AstArgument*>, bool> lhsVersions = getInlinedArgument(program, constraint->getLHS());
    if(lhsVersions.second) {
      changed = true;
      for(AstArgument* newLhs : lhsVersions.first) {
        AstLiteral* newLit = new AstConstraint(constraint->getOperator(), std::unique_ptr<AstArgument>(newLhs), std::unique_ptr<AstArgument>(constraint->getRHS()->clone()));
        versions.push_back(newLit);
      }
    } else {
      std::pair<std::vector<AstArgument*>, bool> rhsVersions = getInlinedArgument(program, constraint->getRHS());
      if(rhsVersions.second) {
        changed = true;
        for(AstArgument* newRhs : rhsVersions.first) {
          AstLiteral* newLit = new AstConstraint(constraint->getOperator(), std::unique_ptr<AstArgument>(constraint->getLHS()->clone()), std::unique_ptr<AstArgument>(newRhs));
          versions.push_back(newLit);
        }
      }
    }
  }

  return std::make_pair(std::make_pair(addedBodyLiterals, inlined), std::make_pair(versions, changed));
}

AstArgument* combineAggregators(std::vector<AstArgument*> aggrs, BinaryOp fun) {
  if(aggrs.size() == 1) {
    return aggrs[0];
  }

  AstArgument* rhs = combineAggregators(std::vector<AstArgument*>(aggrs.begin() + 1, aggrs.end()), fun);
  AstArgument* result = new AstBinaryFunctor(fun, std::unique_ptr<AstArgument>(aggrs[0]), std::unique_ptr<AstArgument> (rhs));

  return result;
}

// TODO: this function
// Handle aggregators
std::pair<std::vector<AstArgument*>, bool> getInlinedArgument(AstProgram& program, const AstArgument* arg) {
  bool changed = false;
  std::vector<AstArgument*> versions;

  if(const AstAggregator* aggr = dynamic_cast<const AstAggregator*>(arg)){
    // first check the argument
    // TODO: how to handle this correctly? what does `sum aggr1 : aggr2` even mean?
    if(aggr->getTargetExpression() != nullptr) {
      std::pair<std::vector<AstArgument*>, bool> argumentVersions = getInlinedArgument(program, aggr->getTargetExpression());
      if(argumentVersions.second) {
        changed = true;
        // TODO: add in the cases for MAX, MIN, COUNT, SUM (all aggr types)
        for(AstArgument* newArg : argumentVersions.first) {
          AstAggregator* newAggr = new AstAggregator(aggr->getOperator());
          newAggr->setTargetExpression(std::unique_ptr<AstArgument>(newArg));
          for(AstLiteral* lit : aggr->getBodyLiterals()) {
            newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(lit->clone()));
          }
          versions.push_back(newAggr);
        }
      }
    }

    // handle the body arguments
    // TODO: only handle one change at a time for now?
    if(!changed) {
      std::vector<AstLiteral*> bodyLiterals = aggr->getBodyLiterals();
      for(int i = 0; i < bodyLiterals.size(); i++) {
        AstLiteral* currLit = bodyLiterals[i];
        auto literalVersions = getInlinedLiteral(program, currLit);

        // literal is inlined
        if(literalVersions.first.second) {
          changed = true;
          AstAggregator::Op op = aggr->getOperator();

          std::vector<AstArgument*> aggrVersions;

          for(std::vector<AstLiteral*> inlineVersions : literalVersions.first.first) {
            AstAggregator* newAggr = new AstAggregator(aggr->getOperator());
            if(aggr->getTargetExpression() != nullptr) {
              newAggr->setTargetExpression(std::unique_ptr<AstArgument>(aggr->getTargetExpression()->clone()));
            }

            // Add in everything except the current literal
            for(int j = 0; j < bodyLiterals.size(); j++) {
              if(i != j) {
                newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(bodyLiterals[j]->clone()));
              }
            }

            // Add in everything new
            for(AstLiteral* addedLit : inlineVersions) {
              newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(addedLit));
            }

            aggrVersions.push_back(newAggr);
          }

          // create the actual overall aggregator
          if(op == AstAggregator::min) {
            // min x : { a(x) }. <=> min ( min x : { a1(x) }, min x : { a2(x) }, ... )
            versions.push_back(combineAggregators(aggrVersions, BinaryOp::MIN));
          } else if (op == AstAggregator::max) {
            // max x : { a(x) }. <=> max ( max x : { a1(x) }, max x : { a2(x) }, ... )
            versions.push_back(combineAggregators(aggrVersions, BinaryOp::MAX));
          } else if (op == AstAggregator::count) {
            // count : { a(x) }. <=> sum ( count : { a1(x) }, count : { a2(x) }, ... )
            versions.push_back(combineAggregators(aggrVersions, BinaryOp::ADD));
          } else if (op == AstAggregator::sum) {
            // sum x : { a(x) }. <=> sum ( sum x : { a1(x) }, sum x : { a2(x) }, ... )
            versions.push_back(combineAggregators(aggrVersions, BinaryOp::ADD));
          } else {
            assert(false && "unhandled aggregator operator type");
          }

        } else if (literalVersions.second.second) {
          changed = true;
          for(AstLiteral* newLit : literalVersions.second.first) {
            AstAggregator* newAggr = new AstAggregator(aggr->getOperator());
            if(aggr->getTargetExpression() != nullptr) {
              newAggr->setTargetExpression(std::unique_ptr<AstArgument>(aggr->getTargetExpression()->clone()));
            }
            for(int j = 0; j < bodyLiterals.size(); j++) {
              if(i == j) {
                newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(newLit));
              } else {
                newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(bodyLiterals[j]->clone()));
              }
            }
            versions.push_back(newAggr);
          }
        }

        // TODO: one at a time for now?
        if(changed) {
          break;
        }
      }
    }
  } else if (dynamic_cast<const AstFunctor*>(arg)) {
    if(const AstUnaryFunctor* functor = dynamic_cast<const AstUnaryFunctor*>(arg)) {
      std::pair<std::vector<AstArgument*>, bool> argumentVersions = getInlinedArgument(program, functor->getOperand());
      if (argumentVersions.second) {
        changed = true;
        for(AstArgument* newArg : argumentVersions.first) {
          AstArgument* newFunctor = new AstUnaryFunctor(functor->getFunction(), std::unique_ptr<AstArgument>(newArg));
          versions.push_back(newFunctor);
        }
      }
    } else if (const AstBinaryFunctor* functor = dynamic_cast<const AstBinaryFunctor*>(arg)) {
      std::pair<std::vector<AstArgument*>, bool> lhsVersions = getInlinedArgument(program, functor->getLHS());
      if(lhsVersions.second) {
        changed = true;
        for(AstArgument* newLhs : lhsVersions.first) {
          AstArgument* newFunctor = new AstBinaryFunctor(functor->getFunction(), std::unique_ptr<AstArgument>(newLhs), std::unique_ptr<AstArgument>(functor->getRHS()));
          versions.push_back(newFunctor);
        }
      } else {
        std::pair<std::vector<AstArgument*>, bool> rhsVersions = getInlinedArgument(program, functor->getRHS());
        if(rhsVersions.second) {
          changed = true;
          for(AstArgument* newRhs : rhsVersions.first) {
            AstArgument* newFunctor = new AstBinaryFunctor(functor->getFunction(), std::unique_ptr<AstArgument>(functor->getLHS()), std::unique_ptr<AstArgument>(newRhs));
            versions.push_back(newFunctor);
          }
        }
      }
    } else if (const AstTernaryFunctor* functor = dynamic_cast<const AstTernaryFunctor*>(arg)) {
      std::pair<std::vector<AstArgument*>, bool> leftVersions = getInlinedArgument(program, functor->getArg(0));
      if(leftVersions.second) {
        changed = true;
        for(AstArgument* newLeft : leftVersions.first) {
          AstArgument* newFunctor = new AstTernaryFunctor(functor->getFunction(), std::unique_ptr<AstArgument>(newLeft), std::unique_ptr<AstArgument>(functor->getArg(1)->clone()), std::unique_ptr<AstArgument>(functor->getArg(2)->clone()));
          versions.push_back(newFunctor);
        }
      } else {
        std::pair<std::vector<AstArgument*>, bool> middleVersions = getInlinedArgument(program, functor->getArg(1));
        if(middleVersions.second) {
          changed = true;
          for(AstArgument* newMiddle : middleVersions.first) {
            AstArgument* newFunctor = new AstTernaryFunctor(functor->getFunction(), std::unique_ptr<AstArgument>(functor->getArg(0)->clone()), std::unique_ptr<AstArgument>(newMiddle), std::unique_ptr<AstArgument>(functor->getArg(2)->clone()));
            versions.push_back(newFunctor);
          }
        } else {
          std::pair<std::vector<AstArgument*>, bool> rightVersions = getInlinedArgument(program, functor->getArg(1));
          if(rightVersions.second) {
            changed = true;
            for(AstArgument* newRight : rightVersions.first) {
              AstArgument* newFunctor = new AstTernaryFunctor(functor->getFunction(), std::unique_ptr<AstArgument>(functor->getArg(0)->clone()), std::unique_ptr<AstArgument>(functor->getArg(1)->clone()), std::unique_ptr<AstArgument>(newRight));
              versions.push_back(newFunctor);
            }
          }
        }
      }
    }
  } else if (const AstTypeCast* cast = dynamic_cast<const AstTypeCast*>(arg)) {
    std::pair<std::vector<AstArgument*>, bool> argumentVersions = getInlinedArgument(program, cast->getValue());
    if(argumentVersions.second) {
      changed = true;
      for(AstArgument* newArg : argumentVersions.first) {
        AstArgument* newTypeCast = new AstTypeCast(std::unique_ptr<AstArgument>(newArg), cast->getType());
        versions.push_back(newTypeCast);
      }
    }
  } else if (const AstRecordInit* record = dynamic_cast<const AstRecordInit*>(arg)) {
    std::vector<AstArgument*> recordArguments = record->getArguments();
    for(int i = 0; i < recordArguments.size(); i++) {
      AstArgument* currentRecArg = recordArguments[i];
      std::pair<std::vector<AstArgument*>, bool> argumentVersions = getInlinedArgument(program, currentRecArg);
      if(argumentVersions.second) {
        changed = true;
        for(AstArgument* newArgumentVersion : argumentVersions.first) {
          AstRecordInit* newRecordArg = new AstRecordInit();
          for(int j = 0; j < recordArguments.size(); j++) {
            if (i == j) {
              newRecordArg->add(std::unique_ptr<AstArgument>(newArgumentVersion));
            } else {
              newRecordArg->add(std::unique_ptr<AstArgument>(recordArguments[j]->clone()));
            }
          }
          versions.push_back(newRecordArg);
        }
      }

      // TODO: for now, only one change at a time
      if(changed) {
        break;
      }
    }
  }

  return std::make_pair(versions, changed);
}

std::pair<std::vector<AstAtom*>, bool> getInlinedAtom(AstProgram& program, AstAtom& atom) {
  bool changed = false;
  std::vector<AstAtom*> versions;

  std::vector<AstArgument*> arguments = atom.getArguments();
  for(int i = 0; i < arguments.size(); i++) {
    AstArgument* arg = arguments[i];
    std::pair<std::vector<AstArgument*>, bool> argumentVersions = getInlinedArgument(program, arg);
    if(argumentVersions.second) {
      changed = true;
      for(AstArgument* newArgument : argumentVersions.first) {
        AstAtom* newAtom = atom.clone();
        newAtom->setArgument(i, std::unique_ptr<AstArgument>(newArgument));
        versions.push_back(newAtom);
      }
    }

    // TODO: for now, only one change at a time
    if(changed) {
      break;
    }
  }

  return std::make_pair(versions, changed);
}

std::pair<std::vector<AstClause*>, bool> getInlinedClause(AstProgram& program, const AstClause& clause) {
  bool changed = false;
  std::vector<AstClause*> versions;

  // Fix arguments in the head
  AstAtom* head = clause.getHead();
  auto headVersions = getInlinedAtom(program, *head);
  if(headVersions.second) {
    changed = true;
    for(AstAtom* newHead : headVersions.first) {
      AstClause* newClause = new AstClause();
      newClause->setSrcLoc(clause.getSrcLoc());
      newClause->setHead(std::unique_ptr<AstAtom>(newHead));
      for(AstLiteral* lit : clause.getBodyLiterals()) {
        newClause->addToBody(std::unique_ptr<AstLiteral>(lit->clone()));
      }
      versions.push_back(newClause);
    }
  }


  // Go through the atoms in the clause and inline the necessary atoms
  // TODO: this only does atoms... fix this up
  if(!changed) {
    auto bodyLiterals = clause.getBodyLiterals();
    for(int i = 0; i < bodyLiterals.size(); i++) {
      AstLiteral* currLit = bodyLiterals[i];
      auto litVersions = getInlinedLiteral(program, currLit);
      if(litVersions.first.second) {
        changed = true;
        for(auto body : litVersions.first.first) {
          AstClause* replacementClause = clause.cloneHead();
          for(AstLiteral* oldLit : bodyLiterals) {
            if(currLit != oldLit) {
              replacementClause->addToBody(std::unique_ptr<AstLiteral>(oldLit->clone()));
            }
          }

          for(AstLiteral* newLit : body) {
            replacementClause->addToBody(std::unique_ptr<AstLiteral>(newLit));
          }

          versions.push_back(replacementClause);
        }
      } else if (litVersions.second.second) {
        changed = true;
        for(AstLiteral* newLit : litVersions.second.first) {
          AstClause* replacementClause = clause.cloneHead();
          for(AstLiteral* oldLit : bodyLiterals) {
            if(currLit != oldLit) {
              replacementClause->addToBody(std::unique_ptr<AstLiteral>(oldLit->clone()));
            }
          }
          replacementClause->addToBody(std::unique_ptr<AstLiteral>(newLit));

          versions.push_back(replacementClause);
        }
      }

      if(changed) {
        // To avoid confusion, only replace one atom per iteration
        // TODO: change this up later to make Smarter (tm)
        break;
      }
    }
}

  return std::make_pair(versions, changed);
}

// TODO: handle negation -> maybe create a new relation <negated_a> :- !a(x,y).
// TODO: won't be grounded though! so should be careful using it!
// TODO: handle constraints
bool InlineRelationsTransformer::transform(AstTranslationUnit& translationUnit) {
  bool changed = false;

  AstProgram& program = *translationUnit.getProgram();

  renameInlinedArguments(program);

  removeInlinedUnderscores(program);

  // Keep trying to inline until we reach a fixed point
  // TODO: why is this important?
  bool clausesChanged = true;
  while(clausesChanged) {
    std::vector<const AstClause*> clausesToDelete;
    clausesChanged = false;

    // Go through each clause in the program and check if we need to inline anything
    visitDepthFirst(program, [&](const AstClause& clause) {
      // Skip if the clause is meant to be inlined
      // TODO: should this really be done? is it more efficient if kept?
      if(program.getRelation(clause.getHead()->getName())->isInline()) {
        return;
      }

      // Get the inlined versions of this clause
      std::pair<std::vector<AstClause*>, bool> newClauses = getInlinedClause(program, clause);

      if(newClauses.second) {
        // Clause was replaced with equivalent versions, so delete it!
        clausesToDelete.push_back(&clause);

        // Add in the new clauses
        for(AstClause* replacementClause : newClauses.first) {
          //std::cout << "NEW CLAUSE:\n" << *replacementClause << std::endl;
          //std::cout << "OLD CLAUSE:\n" << clause << std::endl;
          program.appendClause(std::unique_ptr<AstClause>(replacementClause));
        }

        clausesChanged = true;
        changed = true;
      }
    });

    // Delete all clauses that were replaced
    // TODO: double check positioning of clause deletion
    for(const AstClause* clause : clausesToDelete) {
      program.removeClause(clause);
    }
  }

  // // handle aggregators
  // clausesChanged = true;
  // visitDepthFirst(program, [&](AstAggregator& agg) {
  //
  // });
  //
  // struct AggregateInliner : public AstNodeMapper {
  //   AstProgram& program;
  //
  //   AggregateInliner(AstProgram& program) : program(program) {}
  //   std::unique_ptr<AstNode> operator()(std::unique_ptr<AstNode> node) const override {
  //     // apply down to the base case first
  //     node->apply(*this);
  //
  //     if(AstAggregator* aggr = dynamic_cast<AstAggregator*>(node.get())) {
  //       AstAggregator* newAggr = new AstAggregator(aggr->getOperator());
  //       if(aggr->getTargetExpression() != nullptr) {
  //         newAggr->setTargetExpression(std::unique_ptr<AstArgument>(aggr->getTargetExpression()->clone()));
  //       }
  //
  //       for(AstLiteral* lit : aggr->getBodyLiterals()) {
  //         if(auto atom = dynamic_cast<AstAtom*>(lit)) {
  //           if(program.getRelation(atom->getName())->isInline()) {
  //
  //           } else {
  //             newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(atom->clone()));
  //           }
  //         } else {
  //           newAggr->addBodyLiteral(std::unique_ptr<AstLiteral>(lit->clone()));
  //         }
  //       }
  //     }
  //     return node;
  //   }
  // };


  std::cout << program << std::endl;
  return changed;
}

bool NormaliseConstraintsTransformer::transform(AstTranslationUnit& translationUnit) {
    bool changed = false;

    // set a prefix for variables bound by magic-set for identification later
    // prepended by + to avoid conflict with user-defined variables
    const std::string boundPrefix = "+abdul";

    AstProgram* program = translationUnit.getProgram();
    std::vector<AstRelation*> relations = program->getRelations();

    int constantCount = 0;    // number of constants seen so far
    int underscoreCount = 0;  // number of underscores seen so far

    // go through the relations, replacing constants and underscores with constraints and variables
    for (AstRelation* rel : relations) {
        std::vector<AstClause*> clauses = rel->getClauses();
        for (AstClause* clause : clauses) {
            // create the replacement clause
            AstClause* newClause = clause->cloneHead();

            for (AstLiteral* lit : clause->getBodyLiterals()) {
                // if not an atom, just add it immediately
                if (dynamic_cast<AstAtom*>(lit) == 0) {
                    newClause->addToBody(std::unique_ptr<AstLiteral>(lit->clone()));
                    continue;
                }

                AstAtom* newAtom = lit->getAtom()->clone();
                std::vector<AstArgument*> args = newAtom->getArguments();
                for (size_t argNum = 0; argNum < args.size(); argNum++) {
                    AstArgument* currArg = args[argNum];

                    // check if the argument is constant
                    if (dynamic_cast<const AstConstant*>(currArg)) {
                        // constant found
                        changed = true;

                        // create new variable name (with appropriate suffix)
                        std::stringstream newVariableName;
                        std::stringstream currArgName;
                        currArgName << *currArg;

                        if (dynamic_cast<AstNumberConstant*>(currArg)) {
                            newVariableName << boundPrefix << constantCount << "_" << currArgName.str()
                                            << "_n";
                        } else {
                            newVariableName << boundPrefix << constantCount << "_"
                                            << currArgName.str().substr(1, currArgName.str().size() - 2)
                                            << "_s";
                        }

                        AstArgument* variable = new AstVariable(newVariableName.str());
                        AstArgument* constant = currArg->clone();

                        constantCount++;

                        // update argument to be the variable created
                        newAtom->setArgument(argNum, std::unique_ptr<AstArgument>(variable));

                        // add in constraint (+abdulX = constant)
                        newClause->addToBody(std::unique_ptr<AstLiteral>(new AstConstraint(
                                BinaryConstraintOp::EQ, std::unique_ptr<AstArgument>(variable->clone()),
                                std::unique_ptr<AstArgument>(constant))));
                    } else if (dynamic_cast<const AstUnnamedVariable*>(currArg)) {
                        // underscore found
                        changed = true;

                        // create new variable name for the underscore (with appropriate suffix)
                        std::stringstream newVariableName;
                        newVariableName.str("");
                        newVariableName << "+underscore" << underscoreCount;
                        underscoreCount++;

                        AstArgument* variable = new AstVariable(newVariableName.str());

                        // update argument to be a variable
                        newAtom->setArgument(argNum, std::unique_ptr<AstArgument>(variable));
                    }
                }

                // add the new atom to the body
                newClause->addToBody(std::unique_ptr<AstLiteral>(newAtom));
            }

            // swap out the old clause with the new one
            rel->removeClause(clause);
            rel->addClause(std::unique_ptr<AstClause>(newClause));
        }
    }

    return changed;
}

}  // end of namespace souffle
