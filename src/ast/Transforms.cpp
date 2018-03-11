/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Transforms.cpp
 *
 * Implementation of AST transformation passes.
 *
 ***********************************************************************/

#include "Transforms.h"
#include "PrecedenceGraph.h"
#include "TypeAnalysis.h"
#include "Utils.h"
#include "Visitor.h"

namespace souffle {
namespace ast {

bool PipelineTransformer::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    for (auto& transformer : pipeline) {
        changed |= applySubtransformer(translationUnit, transformer.get());
    }
    return changed;
}

bool ConditionalTransformer::transform(TranslationUnit& translationUnit) {
    return condition() ? applySubtransformer(translationUnit, transformer.get()) : false;
}

void ResolveAliasesTransformer::resolveAliases(Program& program) {
    // get all clauses
    std::vector<const Clause*> clauses;
    visitDepthFirst(program, [&](const Relation& rel) {
        for (const auto& cur : rel.getClauses()) {
            clauses.push_back(cur);
        }
    });

    // clean all clauses
    for (const Clause* cur : clauses) {
        // -- Step 1 --
        // get rid of aliases
        std::unique_ptr<Clause> noAlias = resolveAliases(*cur);

        // clean up equalities
        std::unique_ptr<Clause> cleaned = removeTrivialEquality(*noAlias);

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
 * as a transformation to Arguments.
 */
class Substitution {
    // the type of map for storing mappings internally
    //   - variables are identified by their name (!)
    typedef std::map<std::string, std::unique_ptr<Argument>> map_t;

    /** The mapping of variables to terms (see type def above) */
    map_t map;

public:
    // -- Ctors / Dtors --

    Substitution(){};

    Substitution(const std::string& var, const Argument* arg) {
        map.insert(std::make_pair(var, std::unique_ptr<Argument>(arg->clone())));
    }

    virtual ~Substitution() = default;

    /**
     * Applies this substitution to the given argument and
     * returns a pointer to the modified argument.
     *
     * @param node the node to be transformed
     * @return a pointer to the modified or replaced node
     */
    virtual std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const {
        // create a substitution mapper
        struct M : public NodeMapper {
            const map_t& map;

            M(const map_t& map) : map(map) {}

            using NodeMapper::operator();

            std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
                // see whether it is a variable to be substituted
                if (auto var = dynamic_cast<Variable*>(node.get())) {
                    auto pos = map.find(var->getName());
                    if (pos != map.end()) {
                        return std::unique_ptr<Node>(pos->second->clone());
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
        std::unique_ptr<Node> resPtr = (*this)(std::unique_ptr<Node>(static_cast<Node*>(node.release())));
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
            map.insert(std::make_pair(cur.first, std::unique_ptr<Argument>(cur.second->clone())));
        }
    }

    /** A print function (for debugging) */
    void print(std::ostream& out) const {
        out << "{" << join(map, ",",
                              [](std::ostream& out,
                                      const std::pair<const std::string, std::unique_ptr<Argument>>& cur) {
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
 * An equality constraint between to Arguments utilized by the
 * unification algorithm required by the alias resolution.
 */
struct Equation {
    /** The two terms to be equivalent */
    std::unique_ptr<Argument> lhs;
    std::unique_ptr<Argument> rhs;

    Equation(const Argument& lhs, const Argument& rhs)
            : lhs(std::unique_ptr<Argument>(lhs.clone())), rhs(std::unique_ptr<Argument>(rhs.clone())) {}

    Equation(const Argument* lhs, const Argument* rhs)
            : lhs(std::unique_ptr<Argument>(lhs->clone())), rhs(std::unique_ptr<Argument>(rhs->clone())) {}

    Equation(const Equation& other)
            : lhs(std::unique_ptr<Argument>(other.lhs->clone())),
              rhs(std::unique_ptr<Argument>(other.rhs->clone())) {}

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

std::unique_ptr<Clause> ResolveAliasesTransformer::resolveAliases(const Clause& clause) {
    /**
     * This alias analysis utilizes unification over the equality
     * constraints in clauses.
     */

    // -- utilities --

    // tests whether something is a ungrounded variable
    auto isVar = [&](const Argument& arg) { return dynamic_cast<const Variable*>(&arg); };

    // tests whether something is a record
    auto isRec = [&](const Argument& arg) { return dynamic_cast<const RecordInit*>(&arg); };

    // tests whether a value a occurs in a term b
    auto occurs = [](const Argument& a, const Argument& b) {
        bool res = false;
        visitDepthFirst(b, [&](const Argument& cur) { res = res || cur == a; });
        return res;
    };

    // I) extract equations
    std::vector<Equation> equations;
    visitDepthFirst(clause, [&](const BinaryConstraint& rel) {
        if (rel.getOperator() == BinaryConstraintOp::EQ) {
            equations.push_back(Equation(rel.getLHS(), rel.getRHS()));
        }
    });

    // II) compute unifying substitution
    Substitution substitution;

    // a utility for processing newly identified mappings
    auto newMapping = [&](const std::string& var, const Argument* term) {
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
        const Argument& a = *cur.lhs;
        const Argument& b = *cur.rhs;

        // #1:   t = t   => skip
        if (a == b) {
            continue;
        }

        // #2:   [..] = [..]   => decompose
        if (isRec(a) && isRec(b)) {
            // get arguments
            const auto& args_a = static_cast<const RecordInit&>(a).getArguments();
            const auto& args_b = static_cast<const RecordInit&>(b).getArguments();

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
            auto& var = static_cast<const Variable&>(a);
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
        const Variable& v = static_cast<const Variable&>(a);
        const Argument& t = b;

        // #4:   v occurs in t
        if (occurs(v, t)) {
            continue;
        }

        assert(!occurs(v, t));

        // add new maplet
        newMapping(v.getName(), &t);
    }

    // III) compute resulting clause
    return substitution(std::unique_ptr<Clause>(clause.clone()));
}

std::unique_ptr<Clause> ResolveAliasesTransformer::removeTrivialEquality(const Clause& clause) {
    // finally: remove t = t constraints
    std::unique_ptr<Clause> res(clause.cloneHead());
    for (Literal* cur : clause.getBodyLiterals()) {
        // filter out t = t
        if (BinaryConstraint* rel = dynamic_cast<BinaryConstraint*>(cur)) {
            if (rel->getOperator() == BinaryConstraintOp::EQ) {
                if (*rel->getLHS() == *rel->getRHS()) {
                    continue;  // skip this one
                }
            }
        }
        res->addToBody(std::unique_ptr<Literal>(cur->clone()));
    }

    // done
    return res;
}

void ResolveAliasesTransformer::removeComplexTermsInAtoms(Clause& clause) {
    // restore temporary variables for expressions in atoms

    // get list of atoms
    std::vector<Atom*> atoms;
    for (Literal* cur : clause.getBodyLiterals()) {
        if (Atom* arg = dynamic_cast<Atom*>(cur)) {
            atoms.push_back(arg);
        }
    }

    // find all binary operations in atoms
    std::vector<const Argument*> terms;
    for (const Atom* cur : atoms) {
        for (const Argument* arg : cur->getArguments()) {
            // only interested in functions
            if (!(dynamic_cast<const Functor*>(arg))) {
                continue;
            }
            // add this one if not yet registered
            if (!any_of(terms, [&](const Argument* cur) { return *cur == *arg; })) {
                terms.push_back(arg);
            }
        }
    }

    // substitute them with variables (a real map would compare pointers)
    typedef std::vector<std::pair<std::unique_ptr<Argument>, std::unique_ptr<Variable>>> substitution_map;
    substitution_map map;

    int var_counter = 0;
    for (const Argument* arg : terms) {
        map.push_back(std::make_pair(std::unique_ptr<Argument>(arg->clone()),
                std::make_unique<Variable>(" _tmp_" + toString(var_counter++))));
    }

    // apply mapping to replace terms with variables
    struct Update : public NodeMapper {
        const substitution_map& map;
        Update(const substitution_map& map) : map(map) {}
        std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
            // check whether node needs to be replaced
            for (const auto& cur : map) {
                if (*cur.first == *node) {
                    return std::unique_ptr<Node>(cur.second->clone());
                }
            }
            // continue recursively
            node->apply(*this);
            return node;
        }
    };

    Update update(map);

    // update atoms
    for (Atom* atom : atoms) {
        atom->apply(update);
    }

    // add variable constraints to clause
    for (const auto& cur : map) {
        clause.addToBody(std::unique_ptr<Literal>(
                new BinaryConstraint(BinaryConstraintOp::EQ, std::unique_ptr<Argument>(cur.second->clone()),
                        std::unique_ptr<Argument>(cur.first->clone()))));
    }
}

bool RemoveRelationCopiesTransformer::removeRelationCopies(Program& program) {
    typedef std::map<RelationIdentifier, RelationIdentifier> alias_map;

    // tests whether something is a variable
    auto isVar = [&](const Argument& arg) { return dynamic_cast<const Variable*>(&arg); };

    // tests whether something is a record
    auto isRec = [&](const Argument& arg) { return dynamic_cast<const RecordInit*>(&arg); };

    // collect aliases
    alias_map isDirectAliasOf;

    // search for relations only defined by a single rule ..
    for (Relation* rel : program.getRelations()) {
        if (!rel->isComputed() && rel->getClauses().size() == 1u) {
            // .. of shape r(x,y,..) :- s(x,y,..)
            Clause* cl = rel->getClause(0);
            if (!cl->isFact() && cl->getBodySize() == 1u && cl->getAtoms().size() == 1u) {
                Atom* atom = cl->getAtoms()[0];
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
                                const auto& rec_args = static_cast<const RecordInit&>(*cur).getArguments();
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
    std::set<RelationIdentifier> cycle_reps;

    for (std::pair<RelationIdentifier, RelationIdentifier> cur : isDirectAliasOf) {
        // compute replacement

        std::set<RelationIdentifier> visited;
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
    visitDepthFirst(program, [&](const Atom& atom) {
        auto pos = isAliasOf.find(atom.getName());
        if (pos != isAliasOf.end()) {
            const_cast<Atom&>(atom).setName(pos->second);
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

bool UniqueAggregationVariablesTransformer::transform(TranslationUnit& translationUnit) {
    bool changed = false;

    // make variables in aggregates unique
    int aggNumber = 0;
    visitDepthFirstPostOrder(*translationUnit.getProgram(), [&](const Aggregator& agg) {

        // only applicable for aggregates with target expression
        if (!agg.getTargetExpression()) {
            return;
        }

        // get all variables in the target expression
        std::set<std::string> names;
        visitDepthFirst(
                *agg.getTargetExpression(), [&](const Variable& var) { names.insert(var.getName()); });

        // rename them
        visitDepthFirst(agg, [&](const Variable& var) {
            auto pos = names.find(var.getName());
            if (pos == names.end()) {
                return;
            }
            const_cast<Variable&>(var).setName(" " + var.getName() + toString(aggNumber));
            changed = true;
        });

        // increment aggregation number
        aggNumber++;
    });
    return changed;
}

bool MaterializeAggregationQueriesTransformer::materializeAggregationQueries(
        TranslationUnit& translationUnit) {
    bool changed = false;

    Program& program = *translationUnit.getProgram();
    const TypeEnvironment& env = translationUnit.getAnalysis<TypeEnvironmentAnalysis>()->getTypeEnvironment();

    // if an aggregator has a body consisting of more than an atom => create new relation
    int counter = 0;
    visitDepthFirst(program, [&](const Clause& clause) {
        visitDepthFirstPostOrder(clause, [&](const Aggregator& agg) {
            // check whether a materialization is required
            if (!needsMaterializedRelation(agg)) {
                return;
            }

            changed = true;

            // for more body literals: create a new relation
            std::set<std::string> vars;
            visitDepthFirst(agg, [&](const Variable& var) { vars.insert(var.getName()); });

            // -- create a new clause --

            auto relName = "__agg_rel_" + toString(counter++);
            while (program.getRelation(relName) != nullptr) {
                relName = "__agg_rel_" + toString(counter++);
            }

            Atom* head = new Atom();
            head->setName(relName);
            std::vector<bool> symbolArguments;
            for (const auto& cur : vars) {
                head->addArgument(std::make_unique<Variable>(cur));
            }

            Clause* aggClause = new Clause();
            aggClause->setHead(std::unique_ptr<Atom>(head));
            for (const auto& cur : agg.getBodyLiterals()) {
                aggClause->addToBody(std::unique_ptr<Literal>(cur->clone()));
            }

            // instantiate unnamed variables in count operations
            if (agg.getOperator() == Aggregator::count) {
                int count = 0;
                for (const auto& cur : aggClause->getBodyLiterals()) {
                    cur->apply(makeLambdaMapper([&](std::unique_ptr<Node> node) -> std::unique_ptr<Node> {
                        // check whether it is a unnamed variable
                        UnnamedVariable* var = dynamic_cast<UnnamedVariable*>(node.get());
                        if (!var) {
                            return node;
                        }

                        // replace by variable
                        auto name = " _" + toString(count++);
                        auto res = new Variable(name);

                        // extend head
                        head->addArgument(std::unique_ptr<Argument>(res->clone()));

                        // return replacement
                        return std::unique_ptr<Node>(res);
                    }));
                }
            }

            // -- build relation --

            Relation* rel = new Relation();
            rel->setName(relName);
            // add attributes
            std::map<const Argument*, TypeSet> argTypes =
                    TypeAnalysis::analyseTypes(env, *aggClause, &program);
            for (const auto& cur : head->getArguments()) {
                rel->addAttribute(std::unique_ptr<Attribute>(
                        new Attribute(toString(*cur), (isNumberType(argTypes[cur])) ? "number" : "symbol")));
            }

            rel->addClause(std::unique_ptr<Clause>(aggClause));
            program.appendRelation(std::unique_ptr<Relation>(rel));

            // -- update aggregate --
            Atom* aggAtom = head->clone();

            // count the usage of variables in the clause
            // outside of aggregates. Note that the visitor
            // is exhaustive hence double counting occurs
            // which needs to be deducted for variables inside
            // the aggregators and variables in the expression
            // of aggregate need to be added. Counter is zero
            // if the variable is local to the aggregate
            std::map<std::string, int> varCtr;
            visitDepthFirst(clause, [&](const Argument& arg) {
                if (const Aggregator* a = dynamic_cast<const Aggregator*>(&arg)) {
                    visitDepthFirst(arg, [&](const Variable& var) { varCtr[var.getName()]--; });
                    if (a->getTargetExpression() != nullptr) {
                        visitDepthFirst(*a->getTargetExpression(),
                                [&](const Variable& var) { varCtr[var.getName()]++; });
                    }
                } else {
                    visitDepthFirst(arg, [&](const Variable& var) { varCtr[var.getName()]++; });
                }
            });
            for (size_t i = 0; i < aggAtom->getArity(); i++) {
                if (Variable* var = dynamic_cast<Variable*>(aggAtom->getArgument(i))) {
                    // replace local variable by underscore if local
                    if (varCtr[var->getName()] == 0) {
                        aggAtom->setArgument(i, std::make_unique<UnnamedVariable>());
                    }
                }
            }
            const_cast<Aggregator&>(agg).clearBodyLiterals();
            const_cast<Aggregator&>(agg).addBodyLiteral(std::unique_ptr<Literal>(aggAtom));
        });
    });

    return changed;
}

bool MaterializeAggregationQueriesTransformer::needsMaterializedRelation(const Aggregator& agg) {
    // everything with more than 1 body literal => materialize
    if (agg.getBodyLiterals().size() > 1) {
        return true;
    }

    // inspect remaining atom more closely
    const Atom* atom = dynamic_cast<const Atom*>(agg.getBodyLiterals()[0]);
    if (!atom) {
        // no atoms, so materialize
        return true;
    }

    // if the same variable occurs several times => materialize
    bool duplicates = false;
    std::set<std::string> vars;
    visitDepthFirst(*atom,
            [&](const Variable& var) { duplicates = duplicates | !vars.insert(var.getName()).second; });

    // if there are duplicates a materialization is required
    if (duplicates) {
        return true;
    }

    // for all others the materialization can be skipped
    return false;
}

bool RemoveEmptyRelationsTransformer::removeEmptyRelations(TranslationUnit& translationUnit) {
    Program& program = *translationUnit.getProgram();
    bool changed = false;
    for (auto rel : program.getRelations()) {
        if (rel->clauseSize() == 0 && !rel->isInput()) {
            removeEmptyRelationUses(translationUnit, rel);

            bool usedInAggregate = false;
            visitDepthFirst(program, [&](const Aggregator& agg) {
                for (const auto lit : agg.getBodyLiterals()) {
                    visitDepthFirst(*lit, [&](const Atom& atom) {
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
        TranslationUnit& translationUnit, Relation* emptyRelation) {
    Program& program = *translationUnit.getProgram();

    //
    // (1) drop rules from the program that have empty relations in their bodies.
    // (2) drop negations of empty relations
    //
    // get all clauses
    std::vector<const Clause*> clauses;
    visitDepthFirst(program, [&](const Clause& cur) { clauses.push_back(&cur); });

    // clean all clauses
    for (const Clause* cl : clauses) {
        // check for an atom whose relation is the empty relation

        bool removed = false;
        ;
        for (Literal* lit : cl->getBodyLiterals()) {
            if (Atom* arg = dynamic_cast<Atom*>(lit)) {
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
            for (Literal* lit : cl->getBodyLiterals()) {
                if (Negation* neg = dynamic_cast<Negation*>(lit)) {
                    if (getAtomRelation(neg->getAtom(), &program) == emptyRelation) {
                        rewrite = true;
                        break;
                    }
                }
            }

            if (rewrite) {
                // clone clause without negation for empty relations

                auto res = std::unique_ptr<Clause>(cl->cloneHead());

                for (Literal* lit : cl->getBodyLiterals()) {
                    if (Negation* neg = dynamic_cast<Negation*>(lit)) {
                        if (getAtomRelation(neg->getAtom(), &program) != emptyRelation) {
                            res->addToBody(std::unique_ptr<Literal>(lit->clone()));
                        }
                    } else {
                        res->addToBody(std::unique_ptr<Literal>(lit->clone()));
                    }
                }

                program.removeClause(cl);
                program.appendClause(std::move(res));
            }
        }
    }
}

bool RemoveRedundantRelationsTransformer::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    RedundantRelations* redundantRelationsAnalysis = translationUnit.getAnalysis<RedundantRelations>();
    const std::set<const Relation*>& redundantRelations = redundantRelationsAnalysis->getRedundantRelations();
    if (!redundantRelations.empty()) {
        for (auto rel : redundantRelations) {
            translationUnit.getProgram()->removeRelation(rel->getName());
            changed = true;
        }
    }
    return changed;
}

bool RemoveBooleanConstraintsTransformer::transform(TranslationUnit& translationUnit) {
    Program& program = *translationUnit.getProgram();

    // If any boolean constraints exist, they will be removed
    bool changed = false;
    visitDepthFirst(program, [&](const BooleanConstraint& bc) { changed = true; });

    // Remove true and false constant literals from all aggregators
    struct M : public NodeMapper {
        std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
            // Remove them from child nodes
            node->apply(*this);

            if (Aggregator* aggr = dynamic_cast<Aggregator*>(node.get())) {
                bool containsTrue = false;
                bool containsFalse = false;

                for (Literal* lit : aggr->getBodyLiterals()) {
                    if (BooleanConstraint* bc = dynamic_cast<BooleanConstraint*>(lit)) {
                        bc->isTrue() ? containsTrue = true : containsFalse = true;
                    }
                }

                if (containsFalse || containsTrue) {
                    // Only keep literals that aren't boolean constraints
                    auto replacementAggregator = std::unique_ptr<Aggregator>(aggr->clone());
                    replacementAggregator->clearBodyLiterals();

                    bool isEmpty = true;

                    // Don't bother copying over body literals if any are false
                    if (!containsFalse) {
                        for (Literal* lit : aggr->getBodyLiterals()) {
                            // Don't add in 'true' boolean constraints
                            if (!dynamic_cast<BooleanConstraint*>(lit)) {
                                isEmpty = false;
                                replacementAggregator->addBodyLiteral(std::unique_ptr<Literal>(lit->clone()));
                            }
                        }
                    }

                    if (containsFalse || isEmpty) {
                        // Empty aggregator body!
                        // Not currently handled, so add in a false literal in the body
                        // E.g. max x : { } =becomes=> max 1 : {0 = 1}
                        replacementAggregator->setTargetExpression(std::make_unique<NumberConstant>(1));

                        // Add '0 = 1' if false was found, '1 = 1' otherwise
                        int lhsConstant = containsFalse ? 0 : 1;
                        replacementAggregator->addBodyLiteral(std::make_unique<BinaryConstraint>(
                                BinaryConstraintOp::EQ, std::make_unique<NumberConstant>(lhsConstant),
                                std::make_unique<NumberConstant>(1)));
                    }

                    return std::move(replacementAggregator);
                }
            }

            // no false or true, so return the original node
            return node;
        }
    };

    M update;
    program.apply(update);

    // Remove true and false constant literals from all clauses
    for (Relation* rel : program.getRelations()) {
        for (Clause* clause : rel->getClauses()) {
            bool containsTrue = false;
            bool containsFalse = false;

            for (Literal* lit : clause->getBodyLiterals()) {
                if (BooleanConstraint* bc = dynamic_cast<BooleanConstraint*>(lit)) {
                    bc->isTrue() ? containsTrue = true : containsFalse = true;
                }
            }

            if (containsFalse) {
                // Clause will always fail
                rel->removeClause(clause);
            } else if (containsTrue) {
                auto replacementClause = std::unique_ptr<Clause>(clause->cloneHead());

                // Only keep non-'true' literals
                for (Literal* lit : clause->getBodyLiterals()) {
                    if (!dynamic_cast<BooleanConstraint*>(lit)) {
                        replacementClause->addToBody(std::unique_ptr<Literal>(lit->clone()));
                    }
                }

                rel->removeClause(clause);
                rel->addClause(std::move(replacementClause));
            }
        }
    }

    return changed;
}

bool ExtractDisconnectedLiteralsTransformer::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    Program& program = *translationUnit.getProgram();

    /* Process:
     * Go through each clause and construct a variable dependency graph G.
     * The nodes of G are the variables. A path between a and b exists iff
     * a and b appear in a common body literal.
     *
     * Based on the graph, we can extract the body literals that are not associated
     * with the arguments in the head atom into a new relation.
     *
     * E.g. a(x) :- b(x), c(y), d(y). will be transformed into:
     *      - a(x) :- b(x), newrel().
     *      - newrel() :- c(y), d(y).
     *
     * Note that only one pass through the clauses is needed:
     *  - All arguments in the body literals of the transformed clause cannot be
     *    independent of the head arguments (by construction).
     *  - The new relations holding the disconnected body literals have no
     *    head arguments by definition, and so the transformation does not apply.
     */

    std::vector<Clause*> clausesToAdd;
    std::vector<const Clause*> clausesToRemove;

    visitDepthFirst(program, [&](const Clause& clause) {
        // get head variables
        std::set<std::string> headVars;
        visitDepthFirst(*clause.getHead(), [&](const Variable& var) { headVars.insert(var.getName()); });

        // nothing to do if no arguments in the head
        if (headVars.empty()) {
            return;
        }

        // construct the graph
        Graph<std::string> variableGraph = Graph<std::string>();

        // add in its nodes
        visitDepthFirst(clause, [&](const Variable& var) { variableGraph.insert(var.getName()); });

        // add in the edges
        // since we are only looking at reachability, we can just add in an
        // undirected edge from the first argument in the literal to each of the others

        // edges from the head
        std::string firstVariable = *headVars.begin();
        headVars.erase(headVars.begin());
        for (const std::string& var : headVars) {
            variableGraph.insert(firstVariable, var);
            variableGraph.insert(var, firstVariable);
        }

        // edges from literals
        for (Literal* bodyLiteral : clause.getBodyLiterals()) {
            std::set<std::string> litVars;

            visitDepthFirst(*bodyLiteral, [&](const Variable& var) { litVars.insert(var.getName()); });

            // no new edges if only one variable is present
            if (litVars.size() > 1) {
                std::string firstVariable = *litVars.begin();
                litVars.erase(litVars.begin());

                // create the undirected edge
                for (const std::string& var : litVars) {
                    variableGraph.insert(firstVariable, var);
                    variableGraph.insert(var, firstVariable);
                }
            }
        }

        // run a DFS from the first head variable
        std::set<std::string> importantVariables;
        variableGraph.visitDepthFirst(
                firstVariable, [&](const std::string& var) { importantVariables.insert(var); });

        // partition the literals into connected and disconnected based on their variables
        std::vector<Literal*> connectedLiterals;
        std::vector<Literal*> disconnectedLiterals;

        for (Literal* bodyLiteral : clause.getBodyLiterals()) {
            bool connected = false;
            bool hasArgs = false;  // ignore literals with no arguments

            visitDepthFirst(*bodyLiteral, [&](const Argument& arg) {
                hasArgs = true;

                if (auto var = dynamic_cast<const Variable*>(&arg)) {
                    if (importantVariables.find(var->getName()) != importantVariables.end()) {
                        connected = true;
                    }
                }
            });

            if (connected || !hasArgs) {
                connectedLiterals.push_back(bodyLiteral);
            } else {
                disconnectedLiterals.push_back(bodyLiteral);
            }
        }

        if (!disconnectedLiterals.empty()) {
            // need to extract some disconnected lits!
            changed = true;

            static int disconnectedCount = 0;
            std::stringstream nextName;
            nextName << "+disconnected" << disconnectedCount;
            RelationIdentifier newRelationName = nextName.str();
            disconnectedCount++;

            // create the extracted relation and clause
            // newrel() :- disconnectedLiterals(x).
            auto newRelation = std::make_unique<Relation>();
            newRelation->setName(newRelationName);
            program.appendRelation(std::move(newRelation));

            Clause* disconnectedClause = new Clause();
            disconnectedClause->setSrcLoc(clause.getSrcLoc());
            disconnectedClause->setHead(std::make_unique<Atom>(newRelationName));

            for (Literal* disconnectedLit : disconnectedLiterals) {
                disconnectedClause->addToBody(std::unique_ptr<Literal>(disconnectedLit->clone()));
            }

            // create the replacement clause
            // a(x) :- b(x), c(y). --> a(x) :- b(x), newrel().
            Clause* newClause = new Clause();
            newClause->setSrcLoc(clause.getSrcLoc());
            newClause->setHead(std::unique_ptr<Atom>(clause.getHead()->clone()));

            for (Literal* connectedLit : connectedLiterals) {
                newClause->addToBody(std::unique_ptr<Literal>(connectedLit->clone()));
            }

            // add the disconnected clause to the body
            newClause->addToBody(std::make_unique<Atom>(newRelationName));

            // replace the original clause with the new clauses
            clausesToAdd.push_back(newClause);
            clausesToAdd.push_back(disconnectedClause);
            clausesToRemove.push_back(&clause);
        }
    });

    for (Clause* newClause : clausesToAdd) {
        program.appendClause(std::unique_ptr<Clause>(newClause));
    }

    for (const Clause* oldClause : clausesToRemove) {
        program.removeClause(oldClause);
    }

    return changed;
}

bool ReduceExistentialsTransformer::transform(TranslationUnit& translationUnit) {
    Program& program = *translationUnit.getProgram();

    // Checks whether a given clause is recursive
    auto isRecursiveClause = [&](const Clause& clause) {
        RelationIdentifier relationName = clause.getHead()->getName();
        bool recursive = false;
        visitDepthFirst(clause.getBodyLiterals(), [&](const Atom& atom) {
            if (atom.getName() == relationName) {
                recursive = true;
            }
        });
        return recursive;
    };

    // Checks whether an atom is of the form a(_,_,...,_)
    auto isExistentialAtom = [&](const Atom& atom) {
        for (Argument* arg : atom.getArguments()) {
            if (!dynamic_cast<UnnamedVariable*>(arg)) {
                return false;
            }
        }
        return true;
    };

    // Construct a dependency graph G where:
    // - Each relation is a node
    // - An edge (a,b) exists iff a uses b "non-existentially" in one of its *recursive* clauses
    // This way, a relation can be transformed into an existential form
    // if and only if all its predecessors can also be transformed.
    Graph<RelationIdentifier> relationGraph = Graph<RelationIdentifier>();

    // Add in the nodes
    for (Relation* relation : program.getRelations()) {
        relationGraph.insert(relation->getName());
    }

    // Keep track of all relations that cannot be transformed
    std::set<RelationIdentifier> minimalIrreducibleRelations;

    for (Relation* relation : program.getRelations()) {
        if (relation->isComputed() || relation->isInput()) {
            // No I/O relations can be transformed
            minimalIrreducibleRelations.insert(relation->getName());
        }

        for (Clause* clause : relation->getClauses()) {
            bool recursive = isRecursiveClause(*clause);
            visitDepthFirst(*clause, [&](const Atom& atom) {
                if (atom.getName() == clause->getHead()->getName()) {
                    return;
                }

                if (!isExistentialAtom(atom)) {
                    if (recursive) {
                        // Clause is recursive, so add an edge to the dependency graph
                        relationGraph.insert(clause->getHead()->getName(), atom.getName());
                    } else {
                        // Non-existential apperance in a non-recursive clause, so
                        // it's out of the picture
                        minimalIrreducibleRelations.insert(atom.getName());
                    }
                }
            });
        }
    }

    // TODO (see issue #564): Don't transform relations appearing in aggregators
    //                        due to aggregator issues with unnamed variables.
    visitDepthFirst(program, [&](const Aggregator& aggr) {
        visitDepthFirst(aggr, [&](const Atom& atom) { minimalIrreducibleRelations.insert(atom.getName()); });
    });

    // Run a DFS from each 'bad' source
    // A node is reachable in a DFS from an irreducible node if and only if it is
    // also an irreducible node
    std::set<RelationIdentifier> irreducibleRelations;
    for (RelationIdentifier relationName : minimalIrreducibleRelations) {
        relationGraph.visitDepthFirst(
                relationName, [&](const RelationIdentifier& subRel) { irreducibleRelations.insert(subRel); });
    }

    // All other relations are necessarily existential
    std::set<RelationIdentifier> existentialRelations;
    for (Relation* relation : program.getRelations()) {
        if (!relation->getClauses().empty() &&
                irreducibleRelations.find(relation->getName()) == irreducibleRelations.end()) {
            existentialRelations.insert(relation->getName());
        }
    }

    // Reduce the existential relations
    for (RelationIdentifier relationName : existentialRelations) {
        Relation* originalRelation = program.getRelation(relationName);

        std::stringstream newRelationName;
        newRelationName << "+?exists_" << relationName;

        auto newRelation = std::make_unique<Relation>();
        newRelation->setName(newRelationName.str());
        newRelation->setSrcLoc(originalRelation->getSrcLoc());

        // EqRel relations require two arguments, so remove it from the qualifier
        newRelation->setQualifier(originalRelation->getQualifier() & ~(EQREL_RELATION));

        // Keep all non-recursive clauses
        for (Clause* clause : originalRelation->getClauses()) {
            if (!isRecursiveClause(*clause)) {
                auto newClause = std::make_unique<Clause>();

                newClause->setSrcLoc(clause->getSrcLoc());
                if (const ExecutionPlan* plan = clause->getExecutionPlan()) {
                    newClause->setExecutionPlan(std::unique_ptr<ExecutionPlan>(plan->clone()));
                }
                newClause->setGenerated(clause->isGenerated());
                newClause->setFixedExecutionPlan(clause->hasFixedExecutionPlan());
                newClause->setHead(std::make_unique<Atom>(newRelationName.str()));
                for (Literal* lit : clause->getBodyLiterals()) {
                    newClause->addToBody(std::unique_ptr<Literal>(lit->clone()));
                }

                newRelation->addClause(std::move(newClause));
            }
        }

        program.appendRelation(std::move(newRelation));
    }

    // Mapper that renames the occurrences of marked relations with their existential
    // counterparts
    struct M : public NodeMapper {
        const std::set<RelationIdentifier>& relations;

        M(std::set<RelationIdentifier>& relations) : relations(relations) {}

        std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
            if (Clause* clause = dynamic_cast<Clause*>(node.get())) {
                if (relations.find(clause->getHead()->getName()) != relations.end()) {
                    // Clause is going to be removed, so don't rename it
                    return node;
                }
            } else if (Atom* atom = dynamic_cast<Atom*>(node.get())) {
                if (relations.find(atom->getName()) != relations.end()) {
                    // Relation is now existential, so rename it
                    std::stringstream newName;
                    newName << "+?exists_" << atom->getName();
                    return std::make_unique<Atom>(newName.str());
                }
            }
            node->apply(*this);
            return node;
        }
    };

    M update(existentialRelations);
    program.apply(update);

    bool changed = !existentialRelations.empty();
    return changed;
}

bool NormaliseConstraintsTransformer::transform(TranslationUnit& translationUnit) {
    bool changed = false;

    // set a prefix for variables bound by magic-set for identification later
    // prepended by + to avoid conflict with user-defined variables
    static constexpr const char* boundPrefix = "+abdul";

    Program& program = *translationUnit.getProgram();

    /* Create a node mapper that recursively replaces all constants and underscores
     * with named variables.
     *
     * The mapper keeps track of constraints that should be added to the original
     * clause it is being applied on in a given constraint set.
     */
    struct M : public NodeMapper {
        std::set<BinaryConstraint*>& constraints;
        mutable int changeCount;

        M(std::set<BinaryConstraint*>& constraints, int changeCount)
                : constraints(constraints), changeCount(changeCount) {}

        bool hasChanged() const {
            return changeCount > 0;
        }

        int getChangeCount() const {
            return changeCount;
        }

        std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
            if (StringConstant* stringConstant = dynamic_cast<StringConstant*>(node.get())) {
                // string constant found
                changeCount++;

                // create new variable name (with appropriate suffix)
                std::string constantValue = stringConstant->getConstant();
                std::stringstream newVariableName;
                newVariableName << boundPrefix << changeCount << "_" << constantValue << "_s";

                // create new constraint (+abdulX = constant)
                auto newVariable = std::make_unique<Variable>(newVariableName.str());
                constraints.insert(new BinaryConstraint(BinaryConstraintOp::EQ,
                        std::unique_ptr<Argument>(newVariable->clone()),
                        std::unique_ptr<Argument>(stringConstant->clone())));

                // update constant to be the variable created
                return std::move(newVariable);
            } else if (NumberConstant* numberConstant = dynamic_cast<NumberConstant*>(node.get())) {
                // number constant found
                changeCount++;

                // create new variable name (with appropriate suffix)
                Domain constantValue = numberConstant->getIndex();
                std::stringstream newVariableName;
                newVariableName << boundPrefix << changeCount << "_" << constantValue << "_n";

                // create new constraint (+abdulX = constant)
                auto newVariable = std::make_unique<Variable>(newVariableName.str());
                constraints.insert(new BinaryConstraint(BinaryConstraintOp::EQ,
                        std::unique_ptr<Argument>(newVariable->clone()),
                        std::unique_ptr<Argument>(numberConstant->clone())));

                // update constant to be the variable created
                return std::move(newVariable);
            } else if (dynamic_cast<UnnamedVariable*>(node.get())) {
                // underscore found
                changeCount++;

                // create new variable name
                std::stringstream newVariableName;
                newVariableName << "+underscore" << changeCount;

                return std::make_unique<Variable>(newVariableName.str());
            }

            node->apply(*this);
            return node;
        }
    };

    int changeCount = 0;  // number of constants and underscores seen so far

    // apply the change to all clauses in the program
    for (Relation* rel : program.getRelations()) {
        for (Clause* clause : rel->getClauses()) {
            if (clause->isFact()) {
                continue;  // don't normalise facts
            }

            std::set<BinaryConstraint*> constraints;
            M update(constraints, changeCount);
            clause->apply(update);

            changeCount = update.getChangeCount();
            changed = changed || update.hasChanged();

            for (BinaryConstraint* constraint : constraints) {
                clause->addToBody(std::unique_ptr<BinaryConstraint>(constraint));
            }
        }
    }

    return changed;
}

}  // namespace ast
}  // namespace souffle
