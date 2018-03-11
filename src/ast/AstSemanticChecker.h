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

class AstTranslationUnit;
class ErrorReport;
class AstProgram;
class AstAtom;
class AstLiteral;
class AstAggregator;
class AstArgument;
class AstClause;
class AstRelation;
class AstSrcLocation;
class TypeBinding;
class AstUnionType;
class AstRecordType;
class AstType;
class TypeEnvironment;
class TypeAnalysis;
class PrecedenceGraph;
class RecursiveClauses;

class AstSemanticChecker : public AstTransformer {
private:
    bool transform(AstTranslationUnit& translationUnit) override;

    static void checkProgram(ErrorReport& report, const AstProgram& program, const TypeEnvironment& typeEnv,
            const TypeAnalysis& typeAnalysis, const PrecedenceGraph& precedenceGraph,
            const RecursiveClauses& recursiveClauses);

    static void checkAtom(ErrorReport& report, const AstProgram& program, const AstAtom& atom);
    static void checkLiteral(ErrorReport& report, const AstProgram& program, const AstLiteral& literal);
    static void checkAggregator(
            ErrorReport& report, const AstProgram& program, const AstAggregator& aggregator);
    static void checkArgument(ErrorReport& report, const AstProgram& program, const AstArgument& arg);
    static void checkConstant(ErrorReport& report, const AstArgument& argument);
    static void checkFact(ErrorReport& report, const AstProgram& program, const AstClause& fact);
    static void checkClause(ErrorReport& report, const AstProgram& program, const AstClause& clause,
            const RecursiveClauses& recursiveClauses);
    static void checkRelationDeclaration(ErrorReport& report, const TypeEnvironment& typeEnv,
            const AstProgram& program, const AstRelation& relation);
    static void checkRelation(ErrorReport& report, const TypeEnvironment& typeEnv, const AstProgram& program,
            const AstRelation& relation, const RecursiveClauses& recursiveClauses);
    static void checkRules(ErrorReport& report, const TypeEnvironment& typeEnv, const AstProgram& program,
            const RecursiveClauses& recursiveClauses);

    static void checkUnionType(ErrorReport& report, const AstProgram& program, const AstUnionType& type);
    static void checkRecordType(ErrorReport& report, const AstProgram& program, const AstRecordType& type);
    static void checkType(ErrorReport& report, const AstProgram& program, const AstType& type);
    static void checkTypes(ErrorReport& report, const AstProgram& program);

    static void checkNamespaces(ErrorReport& report, const AstProgram& program);
    static void checkIODirectives(ErrorReport& report, const AstProgram& program);
    static void checkWitnessProblem(ErrorReport& report, const AstProgram& program);
    static void checkInlining(
            ErrorReport& report, const AstProgram& program, const PrecedenceGraph& precedenceGraph);

public:
    ~AstSemanticChecker() override = default;

    std::string getName() const override {
        return "AstSemanticChecker";
    }
};

class AstExecutionPlanChecker : public AstTransformer {
private:
    bool transform(AstTranslationUnit& translationUnit) override;

public:
    std::string getName() const override {
        return "AstExecutionPlanChecker";
    }
};

}  // end of namespace souffle
