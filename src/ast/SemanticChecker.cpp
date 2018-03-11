/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SemanticChecker.cpp
 *
 * Implementation of the semantic checker pass.
 *
 ***********************************************************************/

#include "ast/SemanticChecker.h"
#include "ast/Clause.h"
#include "ast/Program.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/TypeAnalysis.h"
#include "ast/Utils.h"
#include "ast/Visitor.h"
#include "Global.h"
#include "GraphUtils.h"
#include "PrecedenceGraph.h"
#include "Util.h"

#include <set>
#include <vector>

namespace souffle {
namespace ast {

class TranslationUnit;

bool SemanticChecker::transform(TranslationUnit& translationUnit) {
	const ::souffle::TypeEnvironment& typeEnv =
            translationUnit.getAnalysis<TypeEnvironmentAnalysis>()->getTypeEnvironment();
    TypeAnalysis* typeAnalysis = translationUnit.getAnalysis<TypeAnalysis>();
    PrecedenceGraph* precedenceGraph = translationUnit.getAnalysis<PrecedenceGraph>();
    RecursiveClauses* recursiveClauses = translationUnit.getAnalysis<RecursiveClauses>();
    checkProgram(translationUnit.getErrorReport(), *translationUnit.getProgram(), typeEnv, *typeAnalysis,
            *precedenceGraph, *recursiveClauses);
    return false;
}

void SemanticChecker::checkProgram(ErrorReport& report, const Program& program,
        const ::souffle::TypeEnvironment& typeEnv, const TypeAnalysis& typeAnalysis,
        const PrecedenceGraph& precedenceGraph, const RecursiveClauses& recursiveClauses) {
    // -- conduct checks --
    // TODO: re-write to use visitors
    checkTypes(report, program);
    checkRules(report, typeEnv, program, recursiveClauses);
    checkNamespaces(report, program);
    checkIODirectives(report, program);
    checkWitnessProblem(report, program);
    checkInlining(report, program, precedenceGraph);

    // get the list of components to be checked
    std::vector<const Node*> nodes;
    for (const auto& rel : program.getRelations()) {
        for (const auto& cls : rel->getClauses()) {
            nodes.push_back(cls);
        }
    }

    // -- check grounded variables --
    visitDepthFirst(nodes, [&](const Clause& clause) {
        // only interested in rules
        if (clause.isFact()) {
            return;
        }

        // compute all grounded terms
        auto isGrounded = getGroundedTerms(clause);

        // all terms in head need to be grounded
        std::set<std::string> reportedVars;
        for (const Variable* cur : getVariables(clause)) {
            if (!isGrounded[cur] && reportedVars.insert(cur->getName()).second) {
                report.addError("Ungrounded variable " + cur->getName(), cur->getSrcLoc());
            }
        }

    });

    // -- type checks --

    // - variables -
    visitDepthFirst(nodes, [&](const Variable& var) {
        if (typeAnalysis.getTypes(&var).empty()) {
            report.addError("Unable to deduce type for variable " + var.getName(), var.getSrcLoc());
        }
    });

    // - constants -

    // all string constants are used as symbols
    visitDepthFirst(nodes, [&](const StringConstant& cnst) {
        TypeSet types = typeAnalysis.getTypes(&cnst);
        if (!isSymbolType(types)) {
            report.addError("Symbol constant (type mismatch)", cnst.getSrcLoc());
        }
    });

    // all number constants are used as numbers
    visitDepthFirst(nodes, [&](const NumberConstant& cnst) {
        TypeSet types = typeAnalysis.getTypes(&cnst);
        if (!isNumberType(types)) {
            report.addError("Number constant (type mismatch)", cnst.getSrcLoc());
        }
        Domain idx = cnst.getIndex();
        if (idx > MAX_AST_DOMAIN || idx < MIN_AST_DOMAIN) {
            report.addError("Number constant not in range [" + std::to_string(MIN_AST_DOMAIN) + ", " +
                                    std::to_string(MAX_AST_DOMAIN) + "]",
                    cnst.getSrcLoc());
        }
    });

    // all null constants are used as records
    visitDepthFirst(nodes, [&](const NullConstant& cnst) {
        TypeSet types = typeAnalysis.getTypes(&cnst);
        if (!isRecordType(types)) {
            // TODO (#467) remove the next line to enable subprogram compilation for record types
            report.addError("Null constant used as a non-record", cnst.getSrcLoc());
        }
    });

    // record initializations have the same size as their types
    visitDepthFirst(nodes, [&](const RecordInit& cnst) {
        TypeSet types = typeAnalysis.getTypes(&cnst);
        if (isRecordType(types)) {
            // TODO (#467) remove the next line to enable subprogram compilation for record types
            for (const ::souffle::Type& type : types) {
                if (cnst.getArguments().size() !=
                        dynamic_cast<const ::souffle::RecordType*>(&type)->getFields().size()) {
                    report.addError("Wrong number of arguments given to record", cnst.getSrcLoc());
                }
            }
        }
    });

    // - unary functors -
    visitDepthFirst(nodes, [&](const UnaryFunctor& fun) {

        // check arg
        auto arg = fun.getOperand();

        // check appropriate use use of a numeric functor
        if (fun.isNumerical() && !isNumberType(typeAnalysis.getTypes(&fun))) {
            report.addError("Non-numeric use for numeric functor", fun.getSrcLoc());
        }

        // check argument type of a numeric functor
        if (fun.acceptsNumbers() && !isNumberType(typeAnalysis.getTypes(arg))) {
            report.addError("Non-numeric argument for numeric functor", arg->getSrcLoc());
        }

        // check symbolic operators
        if (fun.isSymbolic() && !isSymbolType(typeAnalysis.getTypes(&fun))) {
            report.addError("Non-symbolic use for symbolic functor", fun.getSrcLoc());
        }

        // check symbolic operands
        if (fun.acceptsSymbols() && !isSymbolType(typeAnalysis.getTypes(arg))) {
            report.addError("Non-symbolic argument for symbolic functor", arg->getSrcLoc());
        }
    });

    // - binary functors -
    visitDepthFirst(nodes, [&](const BinaryFunctor& fun) {

        // check left and right side
        auto lhs = fun.getLHS();
        auto rhs = fun.getRHS();

        // check numeric types of result, first and second argument
        if (fun.isNumerical() && !isNumberType(typeAnalysis.getTypes(&fun))) {
            report.addError("Non-numeric use for numeric functor", fun.getSrcLoc());
        }
        if (fun.acceptsNumbers(0) && !isNumberType(typeAnalysis.getTypes(lhs))) {
            report.addError("Non-numeric first argument for functor", lhs->getSrcLoc());
        }
        if (fun.acceptsNumbers(1) && !isNumberType(typeAnalysis.getTypes(rhs))) {
            report.addError("Non-numeric second argument for functor", rhs->getSrcLoc());
        }

        // check symbolic types of result, first and second argument
        if (fun.isSymbolic() && !isSymbolType(typeAnalysis.getTypes(&fun))) {
            report.addError("Non-symbolic use for symbolic functor", fun.getSrcLoc());
        }
        if (fun.acceptsSymbols(0) && !isSymbolType(typeAnalysis.getTypes(lhs))) {
            report.addError("Non-symbolic first argument for functor", lhs->getSrcLoc());
        }
        if (fun.acceptsSymbols(1) && !isSymbolType(typeAnalysis.getTypes(rhs))) {
            report.addError("Non-symbolic second argument for functor", rhs->getSrcLoc());
        }

    });

    // - ternary functors -
    visitDepthFirst(nodes, [&](const TernaryFunctor& fun) {

        // check left and right side
        auto a0 = fun.getArg(0);
        auto a1 = fun.getArg(1);
        auto a2 = fun.getArg(2);

        // check numeric types of result, first and second argument
        if (fun.isNumerical() && !isNumberType(typeAnalysis.getTypes(&fun))) {
            report.addError("Non-numeric use for numeric functor", fun.getSrcLoc());
        }
        if (fun.acceptsNumbers(0) && !isNumberType(typeAnalysis.getTypes(a0))) {
            report.addError("Non-numeric first argument for functor", a0->getSrcLoc());
        }
        if (fun.acceptsNumbers(1) && !isNumberType(typeAnalysis.getTypes(a1))) {
            report.addError("Non-numeric second argument for functor", a1->getSrcLoc());
        }
        if (fun.acceptsNumbers(2) && !isNumberType(typeAnalysis.getTypes(a2))) {
            report.addError("Non-numeric third argument for functor", a2->getSrcLoc());
        }

        // check symbolic types of result, first and second argument
        if (fun.isSymbolic() && !isSymbolType(typeAnalysis.getTypes(&fun))) {
            report.addError("Non-symbolic use for symbolic functor", fun.getSrcLoc());
        }
        if (fun.acceptsSymbols(0) && !isSymbolType(typeAnalysis.getTypes(a0))) {
            report.addError("Non-symbolic first argument for functor", a0->getSrcLoc());
        }
        if (fun.acceptsSymbols(1) && !isSymbolType(typeAnalysis.getTypes(a1))) {
            report.addError("Non-symbolic second argument for functor", a1->getSrcLoc());
        }
        if (fun.acceptsSymbols(2) && !isSymbolType(typeAnalysis.getTypes(a2))) {
            report.addError("Non-symbolic third argument for functor", a2->getSrcLoc());
        }

    });

    // - binary relation -
    visitDepthFirst(nodes, [&](const BinaryConstraint& constraint) {

        // only interested in non-equal constraints
        auto op = constraint.getOperator();
        if (op == BinaryConstraintOp::EQ || op == BinaryConstraintOp::NE) {
            return;
        }

        // get left and right side
        auto lhs = constraint.getLHS();
        auto rhs = constraint.getRHS();

        if (constraint.isNumerical()) {
            // check numeric type
            if (!isNumberType(typeAnalysis.getTypes(lhs))) {
                report.addError("Non-numerical operand for comparison", lhs->getSrcLoc());
            }
            if (!isNumberType(typeAnalysis.getTypes(rhs))) {
                report.addError("Non-numerical operand for comparison", rhs->getSrcLoc());
            }
        } else if (constraint.isSymbolic()) {
            // check symbolic type
            if (!isSymbolType(typeAnalysis.getTypes(lhs))) {
                report.addError("Non-string operand for operation", lhs->getSrcLoc());
            }
            if (!isSymbolType(typeAnalysis.getTypes(rhs))) {
                report.addError("Non-string operand for operation", rhs->getSrcLoc());
            }
        }
    });

    // - stratification --

    // check for cyclic dependencies
    const Graph<const Relation*, NameComparison>& depGraph = precedenceGraph.graph();
    for (const Relation* cur : depGraph.vertices()) {
        if (depGraph.reaches(cur, cur)) {
            RelationSet clique = depGraph.clique(cur);
            for (const Relation* cyclicRelation : clique) {
                // Negations and aggregations need to be stratified
                const Literal* foundLiteral = nullptr;
                bool hasNegation = hasClauseWithNegatedRelation(cyclicRelation, cur, &program, foundLiteral);
                if (hasNegation ||
                        hasClauseWithAggregatedRelation(cyclicRelation, cur, &program, foundLiteral)) {
                    std::string relationsListStr = toString(join(
                            clique, ",", [](std::ostream& out, const Relation* r) { out << r->getName(); }));
                    std::vector<DiagnosticMessage> messages;
                    messages.push_back(
                            DiagnosticMessage("Relation " + toString(cur->getName()), cur->getSrcLoc()));
                    std::string negOrAgg = hasNegation ? "negation" : "aggregation";
                    messages.push_back(
                            DiagnosticMessage("has cyclic " + negOrAgg, foundLiteral->getSrcLoc()));
                    report.addDiagnostic(Diagnostic(Diagnostic::ERROR,
                            DiagnosticMessage("Unable to stratify relation(s) {" + relationsListStr + "}"),
                            messages));
                    break;
                }
            }
        }
    }
}

void SemanticChecker::checkAtom(ErrorReport& report, const Program& program, const Atom& atom) {
    // check existence of relation
    auto* r = program.getRelation(atom.getName());
    if (!r) {
        report.addError("Undefined relation " + toString(atom.getName()), atom.getSrcLoc());
    }

    // check arity
    if (r && r->getArity() != atom.getArity()) {
        report.addError("Mismatching arity of relation " + toString(atom.getName()), atom.getSrcLoc());
    }

    for (const Argument* arg : atom.getArguments()) {
        checkArgument(report, program, *arg);
    }
}

/* Check whether an unnamed variable occurs in an argument (expression) */
static bool hasUnnamedVariable(const Argument* arg) {
    if (dynamic_cast<const UnnamedVariable*>(arg)) {
        return true;
    }
    if (dynamic_cast<const Variable*>(arg)) {
        return false;
    }
    if (dynamic_cast<const Constant*>(arg)) {
        return false;
    }
    if (dynamic_cast<const Counter*>(arg)) {
        return false;
    }
    if (const UnaryFunctor* uf = dynamic_cast<const UnaryFunctor*>(arg)) {
        return hasUnnamedVariable(uf->getOperand());
    }
    if (const BinaryFunctor* bf = dynamic_cast<const BinaryFunctor*>(arg)) {
        return hasUnnamedVariable(bf->getLHS()) || hasUnnamedVariable(bf->getRHS());
    }
    if (const TernaryFunctor* tf = dynamic_cast<const TernaryFunctor*>(arg)) {
        return hasUnnamedVariable(tf->getArg(0)) || hasUnnamedVariable(tf->getArg(1)) ||
               hasUnnamedVariable(tf->getArg(2));
    }
    if (const RecordInit* ri = dynamic_cast<const RecordInit*>(arg)) {
        return any_of(ri->getArguments(), (bool (*)(const Argument*))hasUnnamedVariable);
    }
    if (dynamic_cast<const Aggregator*>(arg)) {
        return false;
    }
    std::cout << "Unsupported Argument type: " << typeid(*arg).name() << "\n";
    ASSERT(false && "Unsupported Argument Type!");
    return false;
}

static bool hasUnnamedVariable(const Literal* lit) {
    if (const Atom* at = dynamic_cast<const Atom*>(lit)) {
        return any_of(at->getArguments(), (bool (*)(const Argument*))hasUnnamedVariable);
    }
    if (const Negation* neg = dynamic_cast<const Negation*>(lit)) {
        return hasUnnamedVariable(neg->getAtom());
    }
    if (dynamic_cast<const Constraint*>(lit)) {
        if (dynamic_cast<const BooleanConstraint*>(lit)) {
            return false;
        }
        if (const BinaryConstraint* br = dynamic_cast<const BinaryConstraint*>(lit)) {
            return hasUnnamedVariable(br->getLHS()) || hasUnnamedVariable(br->getRHS());
        }
    }
    std::cout << "Unsupported Literal type: " << typeid(lit).name() << "\n";
    ASSERT(false && "Unsupported Argument Type!");
    return false;
}

void SemanticChecker::checkLiteral(ErrorReport& report, const Program& program, const Literal& literal) {
    // check potential nested atom
    if (auto* atom = literal.getAtom()) {
        checkAtom(report, program, *atom);
    }

    if (const BinaryConstraint* constraint = dynamic_cast<const BinaryConstraint*>(&literal)) {
        checkArgument(report, program, *constraint->getLHS());
        checkArgument(report, program, *constraint->getRHS());
    }

    // check for invalid underscore utilization
    if (hasUnnamedVariable(&literal)) {
        if (dynamic_cast<const Atom*>(&literal)) {
            // nothing to check since underscores are allowed
        } else if (dynamic_cast<const Negation*>(&literal)) {
            // nothing to check since underscores are allowed
        } else if (dynamic_cast<const BinaryConstraint*>(&literal)) {
            report.addError("Underscore in binary relation", literal.getSrcLoc());
        } else {
            std::cout << "Unsupported Literal type: " << typeid(literal).name() << "\n";
            ASSERT(false && "Unsupported Argument Type!");
        }
    }
}

void SemanticChecker::checkAggregator(
        ErrorReport& report, const Program& program, const Aggregator& aggregator) {
    for (Literal* literal : aggregator.getBodyLiterals()) {
        checkLiteral(report, program, *literal);
    }
}

void SemanticChecker::checkArgument(ErrorReport& report, const Program& program, const Argument& arg) {
    if (const Aggregator* agg = dynamic_cast<const Aggregator*>(&arg)) {
        checkAggregator(report, program, *agg);
    } else if (const UnaryFunctor* unaryFunc = dynamic_cast<const UnaryFunctor*>(&arg)) {
        checkArgument(report, program, *unaryFunc->getOperand());
    } else if (const BinaryFunctor* binFunc = dynamic_cast<const BinaryFunctor*>(&arg)) {
        checkArgument(report, program, *binFunc->getLHS());
        checkArgument(report, program, *binFunc->getRHS());
    } else if (const TernaryFunctor* ternFunc = dynamic_cast<const TernaryFunctor*>(&arg)) {
        checkArgument(report, program, *ternFunc->getArg(0));
        checkArgument(report, program, *ternFunc->getArg(1));
        checkArgument(report, program, *ternFunc->getArg(2));
    }
}

static bool isConstantArithExpr(const Argument& argument) {
    if (dynamic_cast<const NumberConstant*>(&argument)) {
        return true;
    }
    if (const UnaryFunctor* unOp = dynamic_cast<const UnaryFunctor*>(&argument)) {
        return unOp->isNumerical() && isConstantArithExpr(*unOp->getOperand());
    }
    if (const BinaryFunctor* binOp = dynamic_cast<const BinaryFunctor*>(&argument)) {
        return binOp->isNumerical() && isConstantArithExpr(*binOp->getLHS()) &&
               isConstantArithExpr(*binOp->getRHS());
    }
    if (const TernaryFunctor* ternOp = dynamic_cast<const TernaryFunctor*>(&argument)) {
        return ternOp->isNumerical() && isConstantArithExpr(*ternOp->getArg(0)) &&
               isConstantArithExpr(*ternOp->getArg(1)) && isConstantArithExpr(*ternOp->getArg(2));
    }
    return false;
}

void SemanticChecker::checkConstant(ErrorReport& report, const Argument& argument) {
    if (const Variable* var = dynamic_cast<const Variable*>(&argument)) {
        report.addError("Variable " + var->getName() + " in fact", var->getSrcLoc());
    } else if (dynamic_cast<const UnnamedVariable*>(&argument)) {
        report.addError("Underscore in fact", argument.getSrcLoc());
    } else if (dynamic_cast<const UnaryFunctor*>(&argument)) {
        if (!isConstantArithExpr(argument)) {
            report.addError("Unary function in fact", argument.getSrcLoc());
        }
    } else if (dynamic_cast<const BinaryFunctor*>(&argument)) {
        if (!isConstantArithExpr(argument)) {
            report.addError("Binary function in fact", argument.getSrcLoc());
        }
    } else if (dynamic_cast<const TernaryFunctor*>(&argument)) {
        if (!isConstantArithExpr(argument)) {
            report.addError("Ternary function in fact", argument.getSrcLoc());
        }
    } else if (dynamic_cast<const Counter*>(&argument)) {
        report.addError("Counter in fact", argument.getSrcLoc());
    } else if (dynamic_cast<const Constant*>(&argument)) {
        // this one is fine - type checker will make sure of number and symbol constants
    } else if (auto* ri = dynamic_cast<const RecordInit*>(&argument)) {
        for (auto* arg : ri->getArguments()) {
            checkConstant(report, *arg);
        }
    } else {
        std::cout << "Unsupported Argument: " << typeid(argument).name() << "\n";
        ASSERT(false && "Unknown case");
    }
}

/* Check if facts contain only constants */
void SemanticChecker::checkFact(ErrorReport& report, const Program& program, const Clause& fact) {
    assert(fact.isFact());

    Atom* head = fact.getHead();
    if (head == nullptr) {
        return;  // checked by clause
    }

    Relation* rel = program.getRelation(head->getName());
    if (rel == nullptr) {
        return;  // checked by clause
    }

    // facts must only contain constants
    for (size_t i = 0; i < head->argSize(); i++) {
        checkConstant(report, *head->getArgument(i));
    }
}

void SemanticChecker::checkClause(ErrorReport& report, const Program& program, const Clause& clause,
        const RecursiveClauses& recursiveClauses) {
    // check head atom
    checkAtom(report, program, *clause.getHead());

    // check for absence of underscores in head
    if (hasUnnamedVariable(clause.getHead())) {
        report.addError("Underscore in head of rule", clause.getHead()->getSrcLoc());
    }

    // check body literals
    for (Literal* lit : clause.getAtoms()) {
        checkLiteral(report, program, *lit);
    }
    for (Negation* neg : clause.getNegations()) {
        checkLiteral(report, program, *neg);
    }
    for (Literal* lit : clause.getConstraints()) {
        checkLiteral(report, program, *lit);
    }

    // check facts
    if (clause.isFact()) {
        checkFact(report, program, clause);
    }

    // check for use-once variables
    std::map<std::string, int> var_count;
    std::map<std::string, const Variable*> var_pos;
    visitDepthFirst(clause, [&](const Variable& var) {
        var_count[var.getName()]++;
        var_pos[var.getName()] = &var;
    });

    // check for variables only occurring once
    if (!clause.isGenerated()) {
        for (const auto& cur : var_count) {
            if (cur.second == 1 && cur.first[0] != '_') {
                report.addWarning(
                        "Variable " + cur.first + " only occurs once", var_pos[cur.first]->getSrcLoc());
            }
        }
    }

    // check execution plan
    if (clause.getExecutionPlan()) {
        auto numAtoms = clause.getAtoms().size();
        for (const auto& cur : clause.getExecutionPlan()->getOrders()) {
            if (cur.second->size() != numAtoms || !cur.second->isComplete()) {
                report.addError("Invalid execution plan", cur.second->getSrcLoc());
            }
        }
    }
    // check auto-increment
    if (recursiveClauses.recursive(&clause)) {
        visitDepthFirst(clause, [&](const Counter& ctr) {
            report.addError("Auto-increment functor in a recursive rule", ctr.getSrcLoc());
        });
    }
}

void SemanticChecker::checkRelationDeclaration(ErrorReport& report, const TypeEnvironment& typeEnv,
        const Program& program, const Relation& relation) {
    for (size_t i = 0; i < relation.getArity(); i++) {
        Attribute* attr = relation.getAttribute(i);
        TypeIdentifier typeName = attr->getTypeName();

        /* check whether type exists */
        if (typeName != "number" && typeName != "symbol" && !program.getType(typeName)) {
            report.addError("Undefined type in attribute " + attr->getAttributeName() + ":" +
                                    toString(attr->getTypeName()),
                    attr->getSrcLoc());
        }

        /* check whether name occurs more than once */
        for (size_t j = 0; j < i; j++) {
            if (attr->getAttributeName() == relation.getAttribute(j)->getAttributeName()) {
                report.addError("Doubly defined attribute name " + attr->getAttributeName() + ":" +
                                        toString(attr->getTypeName()),
                        attr->getSrcLoc());
            }
        }

        /* check whether type is a record type */
        if (typeEnv.isType(typeName)) {
		const ::souffle::Type& type = typeEnv.getType(typeName);
            if (isRecordType(type)) {
                if (relation.isInput()) {
                    report.addError(
                            "Input relations must not have record types. "
                            "Attribute " +
                                    attr->getAttributeName() + " has record type " +
                                    toString(attr->getTypeName()),
                            attr->getSrcLoc());
                }
                if (relation.isOutput()) {
                    report.addWarning(
                            "Record types in output relations are not printed verbatim: attribute " +
                                    attr->getAttributeName() + " has record type " +
                                    toString(attr->getTypeName()),
                            attr->getSrcLoc());
                }
            }
        }
    }
}

void SemanticChecker::checkRelation(ErrorReport& report, const TypeEnvironment& typeEnv,
        const Program& program, const Relation& relation, const RecursiveClauses& recursiveClauses) {
    if (relation.isEqRel()) {
        if (relation.getArity() == 2) {
            if (relation.getAttribute(0)->getTypeName() != relation.getAttribute(1)->getTypeName()) {
                report.addError(
                        "Domains of equivalence relation " + toString(relation.getName()) + " are different",
                        relation.getSrcLoc());
            }
        } else {
            report.addError("Equivalence relation " + toString(relation.getName()) + " is not binary",
                    relation.getSrcLoc());
        }
    }

    // start with declaration
    checkRelationDeclaration(report, typeEnv, program, relation);

    // check clauses
    for (Clause* c : relation.getClauses()) {
        checkClause(report, program, *c, recursiveClauses);
    }

    // check whether this relation is empty
    if (relation.clauseSize() == 0 && !relation.isInput()) {
        report.addWarning(
                "No rules/facts defined for relation " + toString(relation.getName()), relation.getSrcLoc());
    }
}

void SemanticChecker::checkRules(ErrorReport& report, const TypeEnvironment& typeEnv, const Program& program,
        const RecursiveClauses& recursiveClauses) {
    for (Relation* cur : program.getRelations()) {
        checkRelation(report, typeEnv, program, *cur, recursiveClauses);
    }

    for (Clause* cur : program.getOrphanClauses()) {
        checkClause(report, program, *cur, recursiveClauses);
    }
}

// ----- types --------

void SemanticChecker::checkUnionType(ErrorReport& report, const Program& program, const UnionType& type) {
    // check presence of all the element types
    for (const TypeIdentifier& sub : type.getTypes()) {
        if (sub != "number" && sub != "symbol" && !program.getType(sub)) {
            report.addError("Undefined type " + toString(sub) + " in definition of union type " +
                                    toString(type.getName()),
                    type.getSrcLoc());
        }
    }
}

void SemanticChecker::checkRecordType(ErrorReport& report, const Program& program, const RecordType& type) {
    // check proper definition of all field types
    for (const auto& field : type.getFields()) {
        if (field.type != "number" && field.type != "symbol" && !program.getType(field.type)) {
            report.addError(
                    "Undefined type " + toString(field.type) + " in definition of field " + field.name,
                    type.getSrcLoc());
        }
    }

    // check that field names are unique
    auto& fields = type.getFields();
    std::size_t numFields = fields.size();
    for (std::size_t i = 0; i < numFields; i++) {
        const std::string& cur_name = fields[i].name;
        for (std::size_t j = 0; j < i; j++) {
            if (fields[j].name == cur_name) {
                report.addError("Doubly defined field name " + cur_name + " in definition of type " +
                                        toString(type.getName()),
                        type.getSrcLoc());
            }
        }
    }
}

void SemanticChecker::checkType(ErrorReport& report, const Program& program, const Type& type) {
    if (const UnionType* u = dynamic_cast<const UnionType*>(&type)) {
        checkUnionType(report, program, *u);
    } else if (const RecordType* r = dynamic_cast<const RecordType*>(&type)) {
        checkRecordType(report, program, *r);
    }
}

void SemanticChecker::checkTypes(ErrorReport& report, const Program& program) {
    // check each type individually
    for (const auto& cur : program.getTypes()) {
        checkType(report, program, *cur);
    }
}

void SemanticChecker::checkIODirectives(ErrorReport& report, const Program& program) {
    for (const auto& directive : program.getIODirectives()) {
        auto* r = program.getRelation(directive->getName());
        if (r == nullptr) {
            report.addError("Undefined relation " + toString(directive->getName()), directive->getSrcLoc());
        }
    }
}

static const std::vector<SrcLocation> usesInvalidWitness(
        const std::vector<Literal*>& literals, const std::set<std::unique_ptr<Argument>>& groundedArguments) {
    // Node-mapper that replaces aggregators with new (unique) variables
    struct M : public NodeMapper {
        // Variables introduced to replace aggregators
        mutable std::set<std::string> aggregatorVariables;

        const std::set<std::string>& getAggregatorVariables() {
            return aggregatorVariables;
        }

        std::unique_ptr<Node> operator()(std::unique_ptr<Node> node) const override {
            static int numReplaced = 0;
            if (dynamic_cast<Aggregator*>(node.get())) {
                // Replace the aggregator with a variable
                std::stringstream newVariableName;
                newVariableName << "+aggr_var_" << numReplaced++;

                // Keep track of which variables are bound to aggregators
                aggregatorVariables.insert(newVariableName.str());

                return std::make_unique<Variable>(newVariableName.str());
            }
            node->apply(*this);
            return node;
        }
    };

    std::vector<SrcLocation> result;

    // Create two versions of the original clause

    // Clause 1 - will remain equivalent to the original clause in terms of variable groundedness
    auto originalClause = std::make_unique<Clause>();
    originalClause->setHead(std::make_unique<Atom>("*"));

    // Clause 2 - will have aggregators replaced with intrinsically grounded variables
    auto aggregatorlessClause = std::make_unique<Clause>();
    aggregatorlessClause->setHead(std::make_unique<Atom>("*"));

    // Construct both clauses in the same manner to match the original clause
    // Must keep track of the subnode in Clause 1 that each subnode in Clause 2 matches to
    std::map<const Argument*, const Argument*> identicalSubnodeMap;
    for (const Literal* lit : literals) {
        auto firstClone = std::unique_ptr<Literal>(lit->clone());
        auto secondClone = std::unique_ptr<Literal>(lit->clone());

        // Construct the mapping between equivalent literal subnodes
        std::vector<const Argument*> firstCloneArguments;
        visitDepthFirst(*firstClone, [&](const Argument& arg) { firstCloneArguments.push_back(&arg); });

        std::vector<const Argument*> secondCloneArguments;
        visitDepthFirst(*secondClone, [&](const Argument& arg) { secondCloneArguments.push_back(&arg); });

        for (size_t i = 0; i < firstCloneArguments.size(); i++) {
            identicalSubnodeMap[secondCloneArguments[i]] = firstCloneArguments[i];
        }

        // Actually add the literal clones to each clause
        originalClause->addToBody(std::move(firstClone));
        aggregatorlessClause->addToBody(std::move(secondClone));
    }

    // Replace the aggregators in Clause 2 with variables
    M update;
    aggregatorlessClause->apply(update);

    // Create a dummy atom to force certain arguments to be grounded in the aggregatorlessClause
    auto groundingAtomAggregatorless = std::make_unique<Atom>("grounding_atom");
    auto groundingAtomOriginal = std::make_unique<Atom>("grounding_atom");

    // Force the new aggregator variables to be grounded in the aggregatorless clause
    const std::set<std::string>& aggregatorVariables = update.getAggregatorVariables();
    for (const std::string& str : aggregatorVariables) {
        groundingAtomAggregatorless->addArgument(std::make_unique<Variable>(str));
    }

    // Force the given grounded arguments to be grounded in both clauses
    for (const std::unique_ptr<Argument>& arg : groundedArguments) {
        groundingAtomAggregatorless->addArgument(std::unique_ptr<Argument>(arg->clone()));
        groundingAtomOriginal->addArgument(std::unique_ptr<Argument>(arg->clone()));
    }

    aggregatorlessClause->addToBody(std::move(groundingAtomAggregatorless));
    originalClause->addToBody(std::move(groundingAtomOriginal));

    // Compare the grounded analysis of both generated clauses
    // All added arguments in Clause 2 were forced to be grounded, so if an ungrounded argument
    // appears in Clause 2, it must also appear in Clause 1. Consequently, have two cases:
    //   - The argument is also ungrounded in Clause 1 - handled by another check
    //   - The argument is grounded in Clause 1 => the argument was grounded in the
    //     first clause somewhere along the line by an aggregator-body - not allowed!
    std::set<std::unique_ptr<Argument>> newlyGroundedArguments;
    std::map<const Argument*, bool> originalGrounded = getGroundedTerms(*originalClause);
    std::map<const Argument*, bool> aggregatorlessGrounded = getGroundedTerms(*aggregatorlessClause);
    for (auto pair : aggregatorlessGrounded) {
        if (!pair.second && originalGrounded[identicalSubnodeMap[pair.first]]) {
            result.push_back(pair.first->getSrcLoc());
        }

        // Otherwise, it can now be considered grounded
        newlyGroundedArguments.insert(std::unique_ptr<Argument>(pair.first->clone()));
    }

    // All previously grounded are still grounded
    for (const std::unique_ptr<Argument>& arg : groundedArguments) {
        newlyGroundedArguments.insert(std::unique_ptr<Argument>(arg->clone()));
    }

    // Everything on this level is fine, check subaggregators of each literal
    for (const Literal* lit : literals) {
        visitDepthFirst(*lit, [&](const Aggregator& aggr) {
            // Check recursively if an invalid witness is used
            std::vector<Literal*> aggrBodyLiterals = aggr.getBodyLiterals();
            std::vector<SrcLocation> subresult = usesInvalidWitness(aggrBodyLiterals, newlyGroundedArguments);
            for (SrcLocation argloc : subresult) {
                result.push_back(argloc);
            }
        });
    }

    return result;
}

void SemanticChecker::checkWitnessProblem(ErrorReport& report, const Program& program) {
    // Visit each clause to check if an invalid aggregator witness is used
    visitDepthFirst(program, [&](const Clause& clause) {
        // Body literals of the clause to check
        std::vector<Literal*> bodyLiterals = clause.getBodyLiterals();

        // Add in all head variables as new ungrounded body literals
        auto headVariables = std::make_unique<Atom>("*");
        visitDepthFirst(*clause.getHead(), [&](const Variable& var) {
            headVariables->addArgument(std::unique_ptr<Variable>(var.clone()));
        });
        Negation* headNegation = new Negation(std::move(headVariables));
        bodyLiterals.push_back(headNegation);

        // Perform the check
        std::set<std::unique_ptr<Argument>> groundedArguments;
        std::vector<SrcLocation> invalidArguments = usesInvalidWitness(bodyLiterals, groundedArguments);
        for (SrcLocation invalidArgument : invalidArguments) {
            report.addError(
                    "Witness problem: argument grounded by an aggregator's inner scope is used ungrounded in "
                    "outer scope",
                    invalidArgument);
        }

        delete headNegation;
    });
}

/**
 * Find a cycle consisting entirely of inlined relations.
 * If no cycle exists, then an empty vector is returned.
 */
std::vector<RelationIdentifier> findInlineCycle(const PrecedenceGraph& precedenceGraph,
        std::map<const Relation*, const Relation*>& origins, const Relation* current, RelationSet& unvisited,
        RelationSet& visiting, RelationSet& visited) {
    std::vector<RelationIdentifier> result;

    if (current == nullptr) {
        // Not looking at any nodes at the moment, so choose any node from the unvisited list

        if (unvisited.empty()) {
            // Nothing left to visit - so no cycles exist!
            return result;
        }

        // Choose any element from the unvisited set
        current = *unvisited.begin();
        origins[current] = nullptr;

        // Move it to "currently visiting"
        unvisited.erase(current);
        visiting.insert(current);

        // Check if we can find a cycle beginning from this node
        std::vector<RelationIdentifier> subresult =
                findInlineCycle(precedenceGraph, origins, current, unvisited, visiting, visited);

        if (subresult.empty()) {
            // No cycle found, try again from another node
            return findInlineCycle(precedenceGraph, origins, nullptr, unvisited, visiting, visited);
        } else {
            // Cycle found! Return it
            return subresult;
        }
    }

    // Check neighbours
    const RelationSet& successors = precedenceGraph.graph().successors(current);
    for (const Relation* successor : successors) {
        // Only care about inlined neighbours in the graph
        if (successor->isInline()) {
            if (visited.find(successor) != visited.end()) {
                // The neighbour has already been visited, so move on
                continue;
            }

            if (visiting.find(successor) != visiting.end()) {
                // Found a cycle!!
                // Construct the cycle in reverse
                while (current != nullptr) {
                    result.push_back(current->getName());
                    current = origins[current];
                }
                return result;
            }

            // Node has not been visited yet
            origins[successor] = current;

            // Move from unvisited to visiting
            unvisited.erase(successor);
            visiting.insert(successor);

            // Visit recursively and check if a cycle is formed
            std::vector<RelationIdentifier> subgraphCycle =
                    findInlineCycle(precedenceGraph, origins, successor, unvisited, visiting, visited);

            if (!subgraphCycle.empty()) {
                // Found a cycle!
                return subgraphCycle;
            }
        }
    }

    // Visited all neighbours with no cycle found, so done visiting this node.
    visiting.erase(current);
    visited.insert(current);
    return result;
}

void SemanticChecker::checkInlining(
        ErrorReport& report, const Program& program, const PrecedenceGraph& precedenceGraph) {
    // Find all inlined relations
    RelationSet inlinedRelations;
    visitDepthFirst(program, [&](const Relation& relation) {
        if (relation.isInline()) {
            inlinedRelations.insert(&relation);

            // Inlined relations cannot be computed or input relations
            if (relation.isComputed()) {
                report.addError("Computed relation " + toString(relation.getName()) + " cannot be inlined",
                        relation.getSrcLoc());
            }

            if (relation.isInput()) {
                report.addError("Input relation " + toString(relation.getName()) + " cannot be inlined",
                        relation.getSrcLoc());
            }
        }
    });

    // Check 1:
    // Let G' be the subgraph of the precedence graph G containing only those nodes
    // which are marked with the inline directive.
    // If G' contains a cycle, then inlining cannot be performed.

    RelationSet unvisited;  // nodes that have not been visited yet
    RelationSet visiting;   // nodes that we are currently visiting
    RelationSet visited;    // nodes that have been completely explored

    // All nodes are initially unvisited
    for (const Relation* rel : inlinedRelations) {
        unvisited.insert(rel);
    }

    // Remember the parent node of each visited node to construct the found cycle
    std::map<const Relation*, const Relation*> origins;

    std::vector<RelationIdentifier> result =
            findInlineCycle(precedenceGraph, origins, nullptr, unvisited, visiting, visited);

    // If the result contains anything, then a cycle was found
    if (!result.empty()) {
        Relation* cycleOrigin = program.getRelation(result[result.size() - 1]);

        // Construct the string representation of the cycle
        std::stringstream cycle;
        cycle << "{" << cycleOrigin->getName();

        // Print it backwards to preserve the initial cycle order
        for (int i = result.size() - 2; i >= 0; i--) {
            cycle << ", " << result[i];
        }

        cycle << "}";

        report.addError(
                "Cannot inline cyclically dependent relations " + cycle.str(), cycleOrigin->getSrcLoc());
    }

    // Check 2:
    // Cannot use the counter argument ('$') in inlined relations

    // Check if an inlined literal ever takes in a $
    visitDepthFirst(program, [&](const Atom& atom) {
        Relation* associatedRelation = program.getRelation(atom.getName());
        if (associatedRelation != nullptr && associatedRelation->isInline()) {
            visitDepthFirst(atom, [&](const Argument& arg) {
                if (dynamic_cast<const Counter*>(&arg)) {
                    report.addError(
                            "Cannot inline literal containing a counter argument '$'", arg.getSrcLoc());
                }
            });
        }
    });

    // Check if an inlined clause ever contains a $
    for (const Relation* rel : inlinedRelations) {
        for (Clause* clause : rel->getClauses()) {
            visitDepthFirst(*clause, [&](const Argument& arg) {
                if (dynamic_cast<const Counter*>(&arg)) {
                    report.addError(
                            "Cannot inline clause containing a counter argument '$'", arg.getSrcLoc());
                }
            });
        }
    }

    // Check 3:
    // Suppose the relation b is marked with the inline directive, but appears negated
    // in a clause. Then, if b introduces a new variable in its body, we cannot inline
    // the relation b.

    // Find all relations with the inline declarative that introduce new variables in their bodies
    RelationSet nonNegatableRelations;
    for (const Relation* rel : inlinedRelations) {
        bool foundNonNegatable = false;
        for (const Clause* clause : rel->getClauses()) {
            // Get the variables in the head
            std::set<std::string> headVariables;
            visitDepthFirst(
                    *clause->getHead(), [&](const Variable& var) { headVariables.insert(var.getName()); });

            // Get the variables in the body
            std::set<std::string> bodyVariables;
            visitDepthFirst(clause->getBodyLiterals(),
                    [&](const Variable& var) { bodyVariables.insert(var.getName()); });

            // Check if all body variables are in the head
            // Do this separately to the above so only one error is printed per variable
            for (const std::string& var : bodyVariables) {
                if (headVariables.find(var) == headVariables.end()) {
                    nonNegatableRelations.insert(rel);
                    foundNonNegatable = true;
                    break;
                }
            }

            if (foundNonNegatable) {
                break;
            }
        }
    }

    // Check that these relations never appear negated
    visitDepthFirst(program, [&](const Negation& neg) {
        Relation* associatedRelation = program.getRelation(neg.getAtom()->getName());
        if (associatedRelation != nullptr &&
                nonNegatableRelations.find(associatedRelation) != nonNegatableRelations.end()) {
            report.addError(
                    "Cannot inline negated relation which may introduce new variables", neg.getSrcLoc());
        }
    });

    // Check 4:
    // Don't support inlining atoms within aggregators at this point.

    // Reasoning: Suppose we have an aggregator like `max X: a(X)`, where `a` is inlined to `a1` and `a2`.
    // Then, `max X: a(X)` will become `max( max X: a1(X),  max X: a2(X) )`. Suppose further that a(X) has
    // values X where it is true, while a2(X) does not. Then, the produced argument
    // `max( max X: a1(X),  max X: a2(X) )` will not return anything (as one of its arguments fails), while
    // `max X: a(X)` will.

    // This corner case prevents generalising aggregator inlining with the current set up.

    visitDepthFirst(program, [&](const Aggregator& aggr) {
        visitDepthFirst(aggr, [&](const Atom& subatom) {
            const Relation* rel = program.getRelation(subatom.getName());
            if (rel != nullptr && rel->isInline()) {
                report.addError("Cannot inline relations that appear in aggregator", subatom.getSrcLoc());
            }
        });
    });

    // Check 5:
    // Suppose a relation `a` is inlined, appears negated in a clause, and contains a
    // (possibly nested) unnamed variable in its arguments. Then, the atom can't be
    // inlined, as unnamed variables are named during inlining (since they may appear
    // multiple times in an inlined-clause's body) => ungroundedness!

    // Exception: It's fine if the unnamed variable appears in a nested aggregator, as
    // the entire aggregator will automatically be grounded.

    // TODO (azreika): special case where all rules defined for `a` use the
    // underscored-argument exactly once: can workaround by remapping the variable
    // back to an underscore - involves changes to the actual inlining algo, though

    // Returns the pair (isValid, lastSrcLoc) where:
    //  - isValid is true if and only if the node contains an invalid underscore, and
    //  - lastSrcLoc is the source location of the last visited node
    std::function<std::pair<bool, SrcLocation>(const Node*)> checkInvalidUnderscore = [&](const Node* node) {
        if (dynamic_cast<const UnnamedVariable*>(node)) {
            // Found an invalid underscore
            return std::make_pair(true, node->getSrcLoc());
        } else if (dynamic_cast<const Aggregator*>(node)) {
            // Don't care about underscores within aggregators
            return std::make_pair(false, node->getSrcLoc());
        }

        // Check if any children nodes use invalid underscores
        for (const Node* child : node->getChildNodes()) {
            std::pair<bool, SrcLocation> childStatus = checkInvalidUnderscore(child);
            if (childStatus.first) {
                // Found an invalid underscore
                return childStatus;
            }
        }

        return std::make_pair(false, node->getSrcLoc());
    };

    // Perform the check
    visitDepthFirst(program, [&](const Negation& negation) {
        const Atom* associatedAtom = negation.getAtom();
        const Relation* associatedRelation = program.getRelation(associatedAtom->getName());
        if (associatedRelation != nullptr && associatedRelation->isInline()) {
            std::pair<bool, SrcLocation> atomStatus = checkInvalidUnderscore(associatedAtom);
            if (atomStatus.first) {
                report.addError(
                        "Cannot inline negated atom containing an unnamed variable unless the variable is "
                        "within an aggregator",
                        atomStatus.second);
            }
        }
    });
}

// Check that type and relation names are disjoint sets.
void SemanticChecker::checkNamespaces(ErrorReport& report, const Program& program) {
    std::map<std::string, SrcLocation> names;

    // Find all names and report redeclarations as we go.
    for (const auto& type : program.getTypes()) {
        const std::string name = toString(type->getName());
        if (names.count(name)) {
            report.addError("Name clash on type " + name, type->getSrcLoc());
        } else {
            names[name] = type->getSrcLoc();
        }
    }

    for (const auto& rel : program.getRelations()) {
        const std::string name = toString(rel->getName());
        if (names.count(name)) {
            report.addError("Name clash on relation " + name, rel->getSrcLoc());
        } else {
            names[name] = rel->getSrcLoc();
        }
    }
}

bool ExecutionPlanChecker::transform(TranslationUnit& translationUnit) {
    RelationSchedule* relationSchedule = translationUnit.getAnalysis<RelationSchedule>();
    RecursiveClauses* recursiveClauses = translationUnit.getAnalysis<RecursiveClauses>();

    for (const RelationScheduleStep& step : relationSchedule->schedule()) {
        const std::set<const Relation*>& scc = step.computed();
        for (const Relation* rel : scc) {
            for (const Clause* clause : rel->getClauses()) {
                if (!recursiveClauses->recursive(clause)) {
                    continue;
                }
                if (!clause->getExecutionPlan()) {
                    continue;
                }
                int version = 0;
                for (const Atom* atom : clause->getAtoms()) {
                    if (scc.count(getAtomRelation(atom, translationUnit.getProgram()))) {
                        version++;
                    }
                }
                if (version <= clause->getExecutionPlan()->getMaxVersion()) {
                    for (const auto& cur : clause->getExecutionPlan()->getOrders()) {
                        if (cur.first >= version) {
                            translationUnit.getErrorReport().addDiagnostic(Diagnostic(Diagnostic::ERROR,
                                    DiagnosticMessage(
                                            "execution plan for version " + std::to_string(cur.first),
                                            cur.second->getSrcLoc()),
                                    {DiagnosticMessage("only versions 0.." + std::to_string(version - 1) +
                                                       " permitted")}));
                        }
                    }
                }
            }
        }
    }
    return false;
}

}  // namespace ast
}  // namespace souffle
