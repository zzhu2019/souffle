/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SemanticChecker.h
 *
 * Defines the semantic checker pass.
 *
 ***********************************************************************/
#pragma once

#include "Transformer.h"

namespace souffle {
class ErrorReport;
class PrecedenceGraph;
class RecursiveClauses;
class TypeEnvironment;
}

namespace souffle {
namespace ast {

class Aggregator;
class Argument;
class Atom;
class Clause;
class Literal;
class Program;
class RecordType;
class Relation;
class SrcLocation;
class TranslationUnit;
class Type;
class TypeAnalysis;
class TypeBinding;
class UnionType;

class SemanticChecker : public Transformer {
private:
    bool transform(TranslationUnit& translationUnit) override;

    static void checkProgram(ErrorReport& report, const Program& program, const ::souffle::TypeEnvironment& typeEnv,
            const TypeAnalysis& typeAnalysis, const PrecedenceGraph& precedenceGraph,
            const RecursiveClauses& recursiveClauses);

    static void checkAtom(ErrorReport& report, const Program& program, const Atom& atom);
    static void checkLiteral(ErrorReport& report, const Program& program, const Literal& literal);
    static void checkAggregator(ErrorReport& report, const Program& program, const Aggregator& aggregator);
    static void checkArgument(ErrorReport& report, const Program& program, const Argument& arg);
    static void checkConstant(ErrorReport& report, const Argument& argument);
    static void checkFact(ErrorReport& report, const Program& program, const Clause& fact);
    static void checkClause(ErrorReport& report, const Program& program, const Clause& clause,
            const RecursiveClauses& recursiveClauses);
    static void checkRelationDeclaration(ErrorReport& report, const TypeEnvironment& typeEnv,
            const Program& program, const Relation& relation);
    static void checkRelation(ErrorReport& report, const TypeEnvironment& typeEnv, const Program& program,
            const Relation& relation, const RecursiveClauses& recursiveClauses);
    static void checkRules(ErrorReport& report, const TypeEnvironment& typeEnv, const Program& program,
            const RecursiveClauses& recursiveClauses);

    static void checkUnionType(ErrorReport& report, const Program& program, const UnionType& type);
    static void checkRecordType(ErrorReport& report, const Program& program, const RecordType& type);
    static void checkType(ErrorReport& report, const Program& program, const Type& type);
    static void checkTypes(ErrorReport& report, const Program& program);

    static void checkNamespaces(ErrorReport& report, const Program& program);
    static void checkIODirectives(ErrorReport& report, const Program& program);
    static void checkWitnessProblem(ErrorReport& report, const Program& program);
    static void checkInlining(
            ErrorReport& report, const Program& program, const PrecedenceGraph& precedenceGraph);

public:
    ~SemanticChecker() override = default;

    std::string getName() const override {
        return "SemanticChecker";
    }
};

class ExecutionPlanChecker : public Transformer {
private:
    bool transform(TranslationUnit& translationUnit) override;

public:
    std::string getName() const override {
        return "ExecutionPlanChecker";
    }
};

}  // namespace ast
}  // namespace souffle
