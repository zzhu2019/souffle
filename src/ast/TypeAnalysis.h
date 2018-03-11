/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TypeAnalysis.h
 *
 * A collection of type analyses operating on AST constructs.
 *
 ***********************************************************************/
#pragma once

#include "ast/Analysis.h"
#include "ast/TranslationUnit.h"
#include "TypeSystem.h"

#include <map>

namespace souffle {
namespace ast {

class Node;
class Program;
class Argument;
class Clause;
class Variable;
class Relation;

/**
 * Analyse the given clause and computes for each contained argument
 * whether it is a constant value or not.
 *
 * @param clause the clause to be analyzed
 * @return a map mapping each contained argument to a boolean indicating
 *      whether the argument represents a constant value or not
 */
std::map<const Argument*, bool> getConstTerms(const Clause& clause);

/**
 * Analyse the given clause and computes for each contained argument
 * whether it is a grounded value or not.
 *
 * @param clause the clause to be analyzed
 * @return a map mapping each contained argument to a boolean indicating
 *      whether the argument represents a grounded value or not
 */
std::map<const Argument*, bool> getGroundedTerms(const Clause& clause);

class TypeEnvironmentAnalysis : public Analysis {
private:
    ::souffle::TypeEnvironment env;

    void updateTypeEnvironment(const Program& program);

public:
    static constexpr const char* name = "type-environment";

    void run(const TranslationUnit& translationUnit) override;

    const ::souffle::TypeEnvironment& getTypeEnvironment() {
        return env;
    }
};

class TypeAnalysis : public Analysis {
private:
    std::map<const Argument*, TypeSet> argumentTypes;
    std::vector<std::unique_ptr<Clause>> annotatedClauses;

public:
    static constexpr const char* name = "type-analysis";

    void run(const TranslationUnit& translationUnit) override;

    void print(std::ostream& os) const override;

    /**
     * Get the computed types for the given argument.
     */
    TypeSet getTypes(const Argument* argument) const {
        auto found = argumentTypes.find(argument);
        assert(found != argumentTypes.end());
        return found->second;
    }

    /**
     * Analyse the given clause and computes for each contained argument
     * a set of potential types. If the set associated to an argument is empty,
     * no consistent typing can be found and the rule can not be properly typed.
     *
     * @param env a typing environment describing the set of available types
     * @param clause the clause to be typed
     * @param program the program
     * @return a map mapping each contained argument to a a set of types
     */
    static std::map<const Argument*, TypeSet> analyseTypes(
            const TypeEnvironment& env, const Clause& clause, const Program* program, bool verbose = false);
};

}  // namespace ast
}  // namespace souffle
