/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TypeAnalysis.cpp
 *
 * A collection of type analyses operating on AST constructs.
 *
 ***********************************************************************/

#include "ast/Argument.h"
#include "ast/TypeAnalysis.h"
#include "ast/Utils.h"
#include "ast/Visitor.h"

#include "BinaryConstraintOps.h"
#include "Constraints.h"
#include "Global.h"
#include "Util.h"

namespace souffle {
namespace ast {

namespace {

// -----------------------------------------------------------------------------
//                        AST Constraint Analysis Infrastructure
// -----------------------------------------------------------------------------

/**
 * A variable type to be utilized by AST constraint analysis. Each such variable is
 * associated with an Argument which's property it is describing.
 *
 * @tparam PropertySpace the property space associated to the analysis
 */
template <typename PropertySpace>
struct ConstraintAnalysisVar : public ::souffle::Variable<const Argument*, PropertySpace> {
    explicit ConstraintAnalysisVar(const Argument* arg) : ::souffle::Variable<const Argument*, PropertySpace>(arg) {}
    explicit ConstraintAnalysisVar(const Argument& arg) : ::souffle::Variable<const Argument*, PropertySpace>(&arg) {}

    /** adds print support */
    void print(std::ostream& out) const override {
        out << "var(" << *(this->id) << ")";
    }
};

/**
 * A base class for ConstraintAnalysis collecting constraints for an analysis
 * by visiting every node of a given AST. The collected constraints are
 * then utilized to obtain the desired analysis result.
 *
 * @tparam AnalysisVar the type of variable (and included property space)
 *      to be utilized by this analysis.
 */
template <typename AnalysisVar>
class ConstraintAnalysis : public Visitor<void> {
    typedef typename AnalysisVar::property_space::value_type value_type;

    /** The list of constraints making underlying this analysis */
    Problem<AnalysisVar> constraints;

    /** A map mapping variables to unique instances to facilitate the unification of variables */
    std::map<std::string, AnalysisVar> variables;

protected:
    // a few type definitions
    typedef std::shared_ptr<::souffle::Constraint<AnalysisVar>> constraint_type;
    typedef std::map<const Argument*, value_type> solution_type;

    /**
     * A utility function mapping an Argument to its associated analysis variable.
     *
     * @param arg the AST argument to be mapped
     * @return the analysis variable representing its associated value
     */
    AnalysisVar getVar(const Argument& arg) {
        const auto* var = dynamic_cast<const Variable*>(&arg);
        if (!var) {
            // no mapping required
            return AnalysisVar(arg);
        }

        // filter through map => always take the same variable
        auto res = variables.insert(std::make_pair(var->getName(), AnalysisVar(var)));
        return res.first->second;
    }

    /**
     * A utility function mapping an Argument to its associated analysis variable.
     *
     * @param arg the AST argument to be mapped
     * @return the analysis variable representing its associated value
     */
    AnalysisVar getVar(const Argument* arg) {
        return getVar(*arg);
    }

    /** Adds another constraint to the internally maintained list of constraints */
    void addConstraint(const constraint_type& constraint) {
        constraints.add(constraint);
    }

public:
    /**
     * Runs this constraint analysis on the given clause.
     *
     * @param clause the close to be analysed
     * @param debug a flag enabling the printing of debug information
     * @return an assignment mapping a property to each argument in the given clause
     */
    solution_type analyse(const Clause& clause, bool debug = false) {
        // collect constraints
        visitDepthFirstPreOrder(clause, *this);

        // solve constraints
        auto ass = constraints.solve();

        // print debug information if desired
        if (debug) {
            std::cout << "Clause: " << clause << "\n";
            std::cout << "Problem:\n" << constraints << "\n";
            std::cout << "Solution:\n" << ass << "\n";
        }

        // convert assignment to result
        solution_type res;
        visitDepthFirst(clause, [&](const Argument& cur) { res[&cur] = ass[getVar(cur)]; });
        return res;
    }
};

}  // end namespace

namespace {

// -----------------------------------------------------------------------------
//                        Boolean Disjunct Lattice
// -----------------------------------------------------------------------------

/**
 * The disjunct meet operator, aka boolean or.
 */
struct bool_or {
    bool operator()(bool& a, bool b) const {
        bool t = a;
        a = a || b;
        return t != a;
    }
};

/**
 * A factory producing the value false.
 */
struct false_factory {
    bool operator()() const {
        return false;
    }
};

/**
 * The definition of a lattice utilizing the boolean values {true} and {false} as
 * its value set and the || operation as its meet operation. Correspondingly,
 * the bottom value is {false} and the top value {true}.
 */
struct bool_disjunct_lattic : public property_space<bool, bool_or, false_factory> {};

/** A base type for analysis based on the boolean disjunct lattice */
typedef ConstraintAnalysisVar<bool_disjunct_lattic> BoolDisjunctVar;

/** A base type for constraints on the boolean disjunct lattice */
typedef std::shared_ptr<::souffle::Constraint<BoolDisjunctVar>> BoolDisjunctConstraint;

/**
 * A constraint factory for a constraint ensuring that the value assigned to the
 * given variable is (at least) {true}
 */
BoolDisjunctConstraint isTrue(const BoolDisjunctVar& var) {
    struct C : public ::souffle::Constraint<BoolDisjunctVar> {
        BoolDisjunctVar var;
        C(const BoolDisjunctVar& var) : var(var) {}
        bool update(Assignment<BoolDisjunctVar>& ass) const override {
            auto res = !ass[var];
            ass[var] = true;
            return res;
        }
        void print(std::ostream& out) const override {
            out << var << " is true";
        }
    };
    return std::make_shared<C>(var);
}

/**
 * A constraint factory for a constraint ensuring the constraint
 *
 *                              a ⇒ b
 *
 * Hence, whenever a is mapped to {true}, so is b.
 */
BoolDisjunctConstraint imply(const BoolDisjunctVar& a, const BoolDisjunctVar& b) {
    return sub(a, b, "⇒");
}

/**
 * A constraint factory for a constraint ensuring the constraint
 *
 *               vars[0] ∧ vars[1] ∧ ... ∧ vars[n] ⇒ res
 *
 * Hence, whenever all variables vars[i] are mapped to true, so is res.
 */
BoolDisjunctConstraint imply(const std::vector<BoolDisjunctVar>& vars, const BoolDisjunctVar& res) {
    struct C : public ::souffle::Constraint<BoolDisjunctVar> {
        BoolDisjunctVar res;
        std::vector<BoolDisjunctVar> vars;

        C(const BoolDisjunctVar& res, const std::vector<BoolDisjunctVar>& vars) : res(res), vars(vars) {}

        bool update(Assignment<BoolDisjunctVar>& ass) const override {
            bool r = ass[res];
            if (r) {
                return false;
            }

            for (const auto& cur : vars) {
                if (!ass[cur]) {
                    return false;
                }
            }

            ass[res] = true;
            return true;
        }

        void print(std::ostream& out) const override {
            out << join(vars, " ∧ ") << " ⇒ " << res;
        }
    };

    return std::make_shared<C>(res, vars);
}
}  // namespace

std::map<const Argument*, bool> getConstTerms(const Clause& clause) {
    // define analysis ..
    struct Analysis : public ConstraintAnalysis<BoolDisjunctVar> {
        // #1 - constants are constant
        void visitConstant(const Constant& cur) override {
            // this is a constant value
            addConstraint(isTrue(getVar(cur)));
        }

        // #2 - binary relations may propagate const
        void visitBinaryConstraint(const BinaryConstraint& cur) override {
            // only target equality
            if (cur.getOperator() != BinaryConstraintOp::EQ) {
                return;
            }

            // if equal, link right and left side
            auto lhs = getVar(cur.getLHS());
            auto rhs = getVar(cur.getRHS());

            addConstraint(imply(lhs, rhs));
            addConstraint(imply(rhs, lhs));
        }

        // #3 - const is propagated via unary functors
        void visitUnaryFunctor(const UnaryFunctor& cur) override {
            auto fun = getVar(cur);
            auto op = getVar(cur.getOperand());

            addConstraint(imply(op, fun));
        }

        // #4 - const is propagated via binary functors
        void visitBinaryFunctor(const BinaryFunctor& cur) override {
            auto fun = getVar(cur);
            auto lhs = getVar(cur.getLHS());
            auto rhs = getVar(cur.getRHS());

            addConstraint(imply({lhs, rhs}, fun));
            addConstraint(imply({fun, lhs}, rhs));
            addConstraint(imply({fun, rhs}, lhs));
        }

        // #5 - const is propagated via ternary functors
        void visitTernaryFunctor(const TernaryFunctor& cur) override {
            auto fun = getVar(cur);
            auto a0 = getVar(cur.getArg(0));
            auto a1 = getVar(cur.getArg(1));
            auto a2 = getVar(cur.getArg(2));

            addConstraint(imply({a0, a1, a2}, fun));
        }

        // #6 - if pack nodes and its components
        void visitRecordInit(const RecordInit& init) override {
            auto pack = getVar(init);

            std::vector<BoolDisjunctVar> subs;
            for (const auto& cur : init.getArguments()) {
                subs.push_back(getVar(cur));
            }

            // link vars in both directions
            addConstraint(imply(subs, pack));
            for (const auto& c : subs) {
                addConstraint(imply(pack, c));
            }
        }
    };

    // run analysis on given clause
    return Analysis().analyse(clause);
}

std::map<const Argument*, bool> getGroundedTerms(const Clause& clause) {
    // define analysis ..
    struct Analysis : public ConstraintAnalysis<BoolDisjunctVar> {
        std::set<const Atom*> ignore;

        // #1 - atoms are producing grounded variables
        void visitAtom(const Atom& cur) override {
            // some atoms need to be skipped (head or negation)
            if (ignore.find(&cur) != ignore.end()) {
                return;
            }

            // all arguments are grounded
            for (const auto& arg : cur.getArguments()) {
                addConstraint(isTrue(getVar(arg)));
            }
        }

        // #2 - negations need to be skipped
        void visitNegation(const Negation& cur) override {
            // add nested atom to black-list
            ignore.insert(cur.getAtom());
        }

        // #3 - also skip head
        void visitClause(const Clause& clause) override {
            // ignore head
            ignore.insert(clause.getHead());
        }

        // #4 - binary equality relations propagates groundness
        void visitBinaryConstraint(const BinaryConstraint& cur) override {
            // only target equality
            if (cur.getOperator() != BinaryConstraintOp::EQ) {
                return;
            }

            // if equal, link right and left side
            auto lhs = getVar(cur.getLHS());
            auto rhs = getVar(cur.getRHS());

            addConstraint(imply(lhs, rhs));
            addConstraint(imply(rhs, lhs));
        }

        // #5 - record init nodes
        void visitRecordInit(const RecordInit& init) override {
            auto cur = getVar(init);

            std::vector<BoolDisjunctVar> vars;

            // if record is grounded, so are all its arguments
            for (const auto& arg : init.getArguments()) {
                auto arg_var = getVar(arg);
                addConstraint(imply(cur, arg_var));
                vars.push_back(arg_var);
            }

            // if all arguments are grounded, so is the record
            addConstraint(imply(vars, cur));
        }

        // #6 - constants are also sources of grounded values
        void visitConstant(const Constant& c) override {
            addConstraint(isTrue(getVar(c)));
        }

        // #7 - aggregators are grounding values
        void visitAggregator(const Aggregator& c) override {
            addConstraint(isTrue(getVar(c)));
        }

        // #8 - functors with grounded values are grounded values
        void visitUnaryFunctor(const UnaryFunctor& cur) override {
            auto fun = getVar(cur);
            auto arg = getVar(cur.getOperand());

            addConstraint(imply(arg, fun));
        }

        void visitBinaryFunctor(const BinaryFunctor& cur) override {
            auto fun = getVar(cur);
            auto lhs = getVar(cur.getLHS());
            auto rhs = getVar(cur.getRHS());

            addConstraint(imply({lhs, rhs}, fun));
        }

        void visitTernaryFunctor(const TernaryFunctor& cur) override {
            auto fun = getVar(cur);
            auto a0 = getVar(cur.getArg(0));
            auto a1 = getVar(cur.getArg(1));
            auto a2 = getVar(cur.getArg(2));

            addConstraint(imply({a0, a1, a2}, fun));
        }
    };

    // run analysis on given clause
    return Analysis().analyse(clause);
}

namespace {

// -----------------------------------------------------------------------------
//                          Type Deduction Lattice
// -----------------------------------------------------------------------------

/**
 * An implementation of a meet operation between sets of types computing
 * the set of pair-wise greatest common subtypes.
 */
struct sub_type {
    bool operator()(TypeSet& a, const TypeSet& b) const {
        // compute result set
        TypeSet res = getGreatestCommonSubtypes(a, b);

        // check whether a should change
        if (res == a) {
            return false;
        }

        // update a
        a = res;
        return true;
    }
};

/**
 * A factory for computing sets of types covering all potential types.
 */
struct all_type_factory {
    TypeSet operator()() const {
        return TypeSet::getAllTypes();
    }
};

/**
 * The type lattice forming the property space for the ::souffle::Type analysis. The
 * value set is given by sets of types and the meet operator is based on the
 * pair-wise computation of greatest common subtypes. Correspondingly, the
 * bottom element has to be the set of all types.
 */
struct type_lattice : public property_space<TypeSet, sub_type, all_type_factory> {};

/** The definition of the type of variable to be utilized in the type analysis */
typedef ConstraintAnalysisVar<type_lattice> TypeVar;

/** The definition of the type of constraint to be utilized in the type analysis */
typedef std::shared_ptr<::souffle::Constraint<TypeVar>> TypeConstraint;

/**
 * A constraint factory ensuring that all the types associated to the variable
 * a are subtypes of the variable b.
 */
TypeConstraint isSubtypeOf(const TypeVar& a, const TypeVar& b) {
    return sub(a, b, "<:");
}

/**
 * A constraint factory ensuring that all the types associated to the variable
 * a are subtypes of type b.
 */
TypeConstraint isSubtypeOf(const TypeVar& a, const ::souffle::Type& b) {
    struct C : public ::souffle::Constraint<TypeVar> {
        TypeVar a;
        const ::souffle::Type& b;

        C(const TypeVar& a, const ::souffle::Type& b) : a(a), b(b) {}

        bool update(Assignment<TypeVar>& ass) const override {
            // get current value of variable a
            TypeSet& s = ass[a];

            // remove all types that are not sub-types of b
            if (s.isAll()) {
                s = TypeSet(b);
                return true;
            }

            TypeSet res;
            for (const ::souffle::Type& t : s) {
                res.insert(getGreatestCommonSubtypes(t, b));
            }

            // check whether there was a change
            if (res == s) {
                return false;
            }
            s = res;
            return true;
        }

        void print(std::ostream& out) const override {
            out << a << " <: " << b.getName();
        }
    };

    return std::make_shared<C>(a, b);
}

/**
 * A constraint factory ensuring that all the types associated to the variable
 * a are subtypes of type b.
 */
TypeConstraint isSupertypeOf(const TypeVar& a, const ::souffle::Type& b) {
    struct C : public ::souffle::Constraint<TypeVar> {
        TypeVar a;
        const ::souffle::Type& b;
        mutable bool repeat;

        C(const TypeVar& a, const ::souffle::Type& b) : a(a), b(b), repeat(true) {}

        bool update(Assignment<TypeVar>& ass) const override {
            // don't continually update super type constraints
            if (!repeat) {
                return false;
            }
            repeat = false;

            // get current value of variable a
            TypeSet& s = ass[a];

            // remove all types that are not super-types of b
            if (s.isAll()) {
                s = TypeSet(b);
                return true;
            }

            TypeSet res;
            for (const ::souffle::Type& t : s) {
                res.insert(getLeastCommonSupertypes(t, b));
            }

            // check whether there was a change
            if (res == s) {
                return false;
            }
            s = res;
            return true;
        }

        void print(std::ostream& out) const override {
            out << a << " >: " << b.getName();
        }
    };

    return std::make_shared<C>(a, b);
}

/**
 * A constraint factory ensuring that all the types associated to the variable
 * a are subtypes of the least common super types of types associated to the variables {vars}.
 */
// TypeConstraint isSubtypeOfSuperType(const TypeVar& a, const std::vector<TypeVar>& vars) {
//    // TODO: this function is not used, so maybe it should be deleted
//    assert(!vars.empty() && "Unsupported for no variables!");

//    // if there is only one variable => chose easy way
//    if (vars.size() == 1u) {
//        return isSubtypeOf(a, vars[0]);
//    }

//    struct C : public ::souffle::Constraint<TypeVar> {
//        TypeVar a;
//        std::vector<TypeVar> vars;

//        C(const TypeVar& a, const std::vector<TypeVar>& vars) : a(a), vars(vars) {}

//        virtual bool update(Assignment<TypeVar>& ass) const {
//            // get common super types of given variables
//            TypeSet limit = ass[a];
//            for (const TypeVar& cur : vars) {
//                limit = getLeastCommonSupertypes(limit, ass[cur]);
//            }

//            // compute new value
//            TypeSet res = getGreatestCommonSubtypes(ass[a], limit);

//            // get current value of variable a
//            TypeSet& s = ass[a];

//            // check whether there was a change
//            if (res == s) {
//                return false;
//            }
//            s = res;
//            return true;
//        }

//        virtual void print(std::ostream& out) const {
//            if (vars.size() == 1) {
//                out << a << " <: " << vars[0];
//                return;
//            }
//            out << a << " <: super(" << join(vars, ",") << ")";
//        }
//    };

//    return std::make_shared<C>(a, vars);
//}

TypeConstraint isSubtypeOfComponent(const TypeVar& a, const TypeVar& b, int index) {
    struct C : public ::souffle::Constraint<TypeVar> {
        TypeVar a;
        TypeVar b;
        unsigned index;

        C(const TypeVar& a, const TypeVar& b, int index) : a(a), b(b), index(index) {}

        bool update(Assignment<TypeVar>& ass) const override {
            // get list of types for b
            const TypeSet& recs = ass[b];

            // if it is (not yet) constrainted => skip
            if (recs.isAll()) {
                return false;
            }

            // compute new types for a and b
            TypeSet typesA;
            TypeSet typesB;

            // iterate through types of b
            for (const ::souffle::Type& t : recs) {
                // only retain records
                if (!isRecordType(t)) {
                    continue;
                }
		const ::souffle::RecordType& rec = static_cast<const ::souffle::RecordType&>(t);

                // of proper size
                if (rec.getFields().size() <= index) {
                    continue;
                }

                // this is a valid type for b
                typesB.insert(rec);

                // and its corresponding field for a
                typesA.insert(rec.getFields()[index].type);
            }

            // combine with current types assigned to a
            typesA = getGreatestCommonSubtypes(ass[a], typesA);

            // update values
            bool changed = false;
            if (recs != typesB) {
                ass[b] = typesB;
                changed = true;
            }

            if (ass[a] != typesA) {
                ass[a] = typesA;
                changed = true;
            }

            // done
            return changed;
        }

        void print(std::ostream& out) const override {
            out << a << " <: " << b << "::" << index;
        }
    };

    return std::make_shared<C>(a, b, index);
}
}  // namespace

void TypeEnvironmentAnalysis::run(const TranslationUnit& translationUnit) {
    updateTypeEnvironment(*translationUnit.getProgram());
}

/**
 * A utility function utilized by the finishParsing member function to update a type environment
 * out of a given list of types in the AST
 *
 * @param types the types specified in the input file, contained in the AST
 * @param env the type environment to be updated
 */
void TypeEnvironmentAnalysis::updateTypeEnvironment(const Program& program) {
    // build up new type system based on defined types

    // create all type symbols in a first step
    for (const auto& cur : program.getTypes()) {
        // support faulty codes with multiple definitions
        if (env.isType(cur->getName())) {
            continue;
        }

        // create type within type environment
        if (auto* t = dynamic_cast<const PrimitiveType*>(cur)) {
            if (t->isNumeric()) {
                env.createNumericType(cur->getName());
            } else {
                env.createSymbolType(cur->getName());
            }
        } else if (dynamic_cast<const UnionType*>(cur)) {
            // initialize the union
            env.createUnionType(cur->getName());
        } else if (dynamic_cast<const RecordType*>(cur)) {
            // initialize the record
            env.createRecordType(cur->getName());
        } else {
            std::cout << "Unsupported type construct: " << typeid(cur).name() << "\n";
            ASSERT(false && "Unsupported ::souffle::Type Construct!");
        }
    }

    // link symbols in a second step
    for (const auto& cur : program.getTypes()) {
        ::souffle::Type* type = env.getModifiableType(cur->getName());
        assert(type && "It should be there!");

        if (dynamic_cast<const PrimitiveType*>(cur)) {
            // nothing to do here
        } else if (auto* t = dynamic_cast<const UnionType*>(cur)) {
            // get type as union type
            auto* ut = dynamic_cast<::souffle::UnionType*>(type);
            if (!ut) {
                continue;  // support faulty input
            }

            // add element types
            for (const auto& cur : t->getTypes()) {
                if (env.isType(cur)) {
                    ut->add(env.getType(cur));
                }
            }
        } else if (auto* t = dynamic_cast<const RecordType*>(cur)) {
            // get type as record type
            auto* rt = dynamic_cast<::souffle::RecordType*>(type);
            if (!rt) {
                continue;  // support faulty input
            }

            // add fields
            for (const auto& f : t->getFields()) {
                if (env.isType(f.type)) {
                    rt->add(f.name, env.getType(f.type));
                }
            }
        } else {
            std::cout << "Unsupported type construct: " << typeid(cur).name() << "\n";
            ASSERT(false && "Unsupported ::souffle::Type Construct!");
        }
    }
}

/* Return a new clause with type-annotated variables */
Clause* createAnnotatedClause(const Clause* clause, const std::map<const Argument*, TypeSet> argumentTypes) {
    // Annotates each variable with its type based on a given type analysis result
    struct TypeAnnotator : public NodeMapper {
        const std::map<const Argument*, TypeSet>& types;

        TypeAnnotator(const std::map<const Argument*, TypeSet>& types) : types(types) {}

        std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
            if (Variable* var = dynamic_cast<Variable*>(node.get())) {
                std::stringstream newVarName;
                newVarName << var->getName() << "&isin;" << types.find(var)->second;
                return std::make_unique<Variable>(newVarName.str());
            } else if (UnnamedVariable* var = dynamic_cast<UnnamedVariable*>(node.get())) {
                std::stringstream newVarName;
                newVarName << "_"
                           << "&isin;" << types.find(var)->second;
                return std::make_unique<Variable>(newVarName.str());
            }
            node->apply(*this);
            return node;
        }
    };

    /* Note:
     * Because the type of each argument is stored in the form [address -> type-set],
     * the type-analysis result does not immediately apply to the clone due to differing
     * addresses.
     * Two ways around this:
     *  (1) Perform the type-analysis again for the cloned clause
     *  (2) Keep track of the addresses of equivalent arguments in the cloned clause
     * Method (2) was chosen to avoid having to recompute the analysis each time.
     */
    Clause* annotatedClause = clause->clone();

    // Maps x -> y, where x is the address of an argument in the original clause, and y
    // is the address of the equivalent argument in the clone.
    std::map<const Argument*, const Argument*> memoryMap;

    std::vector<const Argument*> originalAddresses;
    visitDepthFirst(*clause, [&](const Argument& arg) { originalAddresses.push_back(&arg); });

    std::vector<const Argument*> cloneAddresses;
    visitDepthFirst(*annotatedClause, [&](const Argument& arg) { cloneAddresses.push_back(&arg); });

    assert(cloneAddresses.size() == originalAddresses.size());

    for (size_t i = 0; i < originalAddresses.size(); i++) {
        memoryMap[originalAddresses[i]] = cloneAddresses[i];
    }

    // Map the types to the clause clone
    std::map<const Argument*, TypeSet> cloneArgumentTypes;
    for (auto& pair : argumentTypes) {
        cloneArgumentTypes[memoryMap[pair.first]] = pair.second;
    }

    // Create the type-annotated clause
    TypeAnnotator annotator(cloneArgumentTypes);
    annotatedClause->apply(annotator);
    return annotatedClause;
}

void TypeAnalysis::run(const TranslationUnit& translationUnit) {
    TypeEnvironmentAnalysis* typeEnvAnalysis = translationUnit.getAnalysis<TypeEnvironmentAnalysis>();
    for (const Relation* rel : translationUnit.getProgram()->getRelations()) {
        for (const Clause* clause : rel->getClauses()) {
            // Perform the type analysis
            std::map<const Argument*, TypeSet> clauseArgumentTypes = analyseTypes(
                    typeEnvAnalysis->getTypeEnvironment(), *clause, translationUnit.getProgram());
            argumentTypes.insert(clauseArgumentTypes.begin(), clauseArgumentTypes.end());

            if (!Global::config().get("debug-report").empty()) {
                // Store an annotated clause for printing purposes
                Clause* annotatedClause = createAnnotatedClause(clause, clauseArgumentTypes);
                annotatedClauses.push_back(std::unique_ptr<Clause>(annotatedClause));
            }
        }
    }
}

void TypeAnalysis::print(std::ostream& os) const {
    for (const auto& curr : annotatedClauses) {
        os << *curr << std::endl;
    }
}

std::map<const Argument*, TypeSet> TypeAnalysis::analyseTypes(
        const TypeEnvironment& env, const Clause& clause, const Program* program, bool verbose) {
    struct Analysis : public ConstraintAnalysis<TypeVar> {
        const TypeEnvironment& env;
        const Program* program;
        std::set<const Atom*> negated;

        Analysis(const TypeEnvironment& env, const Program* program) : env(env), program(program) {}

        // predicate
        void visitAtom(const Atom& atom) override {
            // get relation
            auto rel = getAtomRelation(&atom, program);
            if (!rel) {
                return;  // error in input program
            }

            auto atts = rel->getAttributes();
            auto args = atom.getArguments();
            if (atts.size() != args.size()) {
                return;  // error in input program
            }

            // set upper boundary of argument types
            for (unsigned i = 0; i < atts.size(); i++) {
                const auto& typeName = atts[i]->getTypeName();
                if (env.isType(typeName)) {
                    // check whether atom is not negated
                    if (negated.find(&atom) == negated.end()) {
                        addConstraint(isSubtypeOf(getVar(args[i]), env.getType(typeName)));
                    } else {
                        addConstraint(isSupertypeOf(getVar(args[i]), env.getType(typeName)));
                    }
                }
            }
        }

        // #2 - negations need to be skipped
        void visitNegation(const Negation& cur) override {
            // add nested atom to black-list
            negated.insert(cur.getAtom());
        }

        // symbol
        void visitStringConstant(const StringConstant& cnst) override {
            // this type has to be a sub-type of symbol
            addConstraint(isSubtypeOf(getVar(cnst), env.getSymbolType()));
        }

        // number
        void visitNumberConstant(const NumberConstant& cnst) override {
            // this type has to be a sub-type of number
            addConstraint(isSubtypeOf(getVar(cnst), env.getNumberType()));
        }

        // binary constraint
        void visitBinaryConstraint(const BinaryConstraint& rel) override {
            auto lhs = getVar(rel.getLHS());
            auto rhs = getVar(rel.getRHS());
            addConstraint(isSubtypeOf(lhs, rhs));
            addConstraint(isSubtypeOf(rhs, lhs));
        }

        // unary functor
        void visitUnaryFunctor(const UnaryFunctor& fun) override {
            auto out = getVar(fun);
            auto in = getVar(fun.getOperand());

            // add a constraint for the return type of the unary functor
            if (fun.isNumerical()) {
                addConstraint(isSubtypeOf(out, env.getNumberType()));
            }
            if (fun.isSymbolic()) {
                addConstraint(isSubtypeOf(out, env.getSymbolType()));
            }

            // add a constraint for the argument type of the unary functor
            if (fun.acceptsNumbers()) {
                addConstraint(isSubtypeOf(in, env.getNumberType()));
            }
            if (fun.acceptsSymbols()) {
                addConstraint(isSubtypeOf(in, env.getSymbolType()));
            }
        }

        // binary functor
        void visitBinaryFunctor(const BinaryFunctor& fun) override {
            auto cur = getVar(fun);
            auto lhs = getVar(fun.getLHS());
            auto rhs = getVar(fun.getRHS());

            // add a constraint for the return type of the binary functor
            if (fun.isNumerical()) {
                addConstraint(isSubtypeOf(cur, env.getNumberType()));
            }
            if (fun.isSymbolic()) {
                addConstraint(isSubtypeOf(cur, env.getSymbolType()));
            }

            // add a constraint for the first argument of the binary functor
            if (fun.acceptsNumbers(0)) {
                addConstraint(isSubtypeOf(lhs, env.getNumberType()));
            }
            if (fun.acceptsSymbols(0)) {
                addConstraint(isSubtypeOf(lhs, env.getSymbolType()));
            }

            // add a constraint for the second argument of the binary functor
            if (fun.acceptsNumbers(1)) {
                addConstraint(isSubtypeOf(rhs, env.getNumberType()));
            }
            if (fun.acceptsSymbols(1)) {
                addConstraint(isSubtypeOf(rhs, env.getSymbolType()));
            }
        }

        // ternary functor
        void visitTernaryFunctor(const TernaryFunctor& fun) override {
            auto cur = getVar(fun);

            auto a0 = getVar(fun.getArg(0));
            auto a1 = getVar(fun.getArg(1));
            auto a2 = getVar(fun.getArg(2));

            // add a constraint for the return type of the ternary functor
            if (fun.isNumerical()) {
                addConstraint(isSubtypeOf(cur, env.getNumberType()));
            }
            if (fun.isSymbolic()) {
                addConstraint(isSubtypeOf(cur, env.getSymbolType()));
            }

            // add a constraint for the first argument of the ternary functor
            if (fun.acceptsNumbers(0)) {
                addConstraint(isSubtypeOf(a0, env.getNumberType()));
            }
            if (fun.acceptsSymbols(0)) {
                addConstraint(isSubtypeOf(a0, env.getSymbolType()));
            }

            // add a constraint for the second argument of the ternary functor
            if (fun.acceptsNumbers(1)) {
                addConstraint(isSubtypeOf(a1, env.getNumberType()));
            }
            if (fun.acceptsSymbols(1)) {
                addConstraint(isSubtypeOf(a1, env.getSymbolType()));
            }

            // add a constraint for the third argument of the ternary functor
            if (fun.acceptsNumbers(2)) {
                addConstraint(isSubtypeOf(a2, env.getNumberType()));
            }
            if (fun.acceptsSymbols(2)) {
                addConstraint(isSubtypeOf(a2, env.getSymbolType()));
            }
        }

        // counter
        void visitCounter(const Counter& counter) override {
            // this value must be a number value
            addConstraint(isSubtypeOf(getVar(counter), env.getNumberType()));
        }

        // components of records
        void visitRecordInit(const RecordInit& init) override {
            // link element types with sub-values
            auto rec = getVar(init);
            int i = 0;

            for (const Argument* value : init.getArguments()) {
                addConstraint(isSubtypeOfComponent(getVar(value), rec, i++));
            }
        }

        // visit aggregates
        void visitAggregator(const Aggregator& agg) override {
            // this value must be a number value
            addConstraint(isSubtypeOf(getVar(agg), env.getNumberType()));

            // also, the target expression needs to be a number
            if (auto expr = agg.getTargetExpression()) {
                addConstraint(isSubtypeOf(getVar(expr), env.getNumberType()));
            }
        }
    };

    // run analysis
    return Analysis(env, program).analyse(clause, verbose);
}

}  // namespace ast
}  // namespace souffle
