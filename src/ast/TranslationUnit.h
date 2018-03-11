/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TranslationUnit.h
 *
 * Define AST translation unit class consisting of
 * an symbol table, AST program, error reports, and
 * cached analysis results.
 *
 ***********************************************************************/
#pragma once

#include "ast/Analysis.h"
#include "ast/Program.h"
#include "DebugReport.h"
#include "ErrorReport.h"
#include "SymbolTable.h"

#include <map>
#include <memory>

namespace souffle {
namespace ast {

/**
 * TranslationUnit class
 */

class TranslationUnit {
private:
    /** cached analyses */
    mutable std::map<std::string, std::unique_ptr<Analysis>> analyses;

    /** AST program */
    std::unique_ptr<Program> program;

    /** Symbol table of AST program */
    SymbolTable& symbolTable;

    /** Error report capturing errors while compiling */
    ErrorReport& errorReport;

    /** HTML debug report */
    DebugReport& debugReport;

public:
    TranslationUnit(std::unique_ptr<Program> program, SymbolTable& s, ErrorReport& e, DebugReport& d,
            bool nowarn = false)
            : program(std::move(program)), symbolTable(s), errorReport(e), debugReport(d) {}

    virtual ~TranslationUnit() = default;

    /** get analysis: analysis is generated on the fly if not present */
    template <class Analysis>
    Analysis* getAnalysis() const {
        std::string name = Analysis::name;
        auto it = analyses.find(name);
        if (it == analyses.end()) {
            // analysis does not exist yet, create instance and run it.
            analyses[name] = std::make_unique<Analysis>();
            analyses[name]->run(*this);
        }
        return dynamic_cast<Analysis*>(analyses[name].get());
    }

    /** get the AST program */
    Program* getProgram() {
        return program.get();
    }

    /** get the AST program */
    const Program* getProgram() const {
        return program.get();
    }

    /** get symbol table */
    souffle::SymbolTable& getSymbolTable() {
        return symbolTable;
    }

    /** get symbol table */
    const souffle::SymbolTable& getSymbolTable() const {
        return symbolTable;
    }

    /** get error report */
    ErrorReport& getErrorReport() {
        return errorReport;
    }

    /** get error report */
    const ErrorReport& getErrorReport() const {
        return errorReport;
    }

    /** destroy all cached analyses of translation unit */
    void invalidateAnalyses() {
        analyses.clear();
    }

    /** get debug report */
    DebugReport& getDebugReport() {
        return debugReport;
    }

    /** get debug report */
    const DebugReport& getDebugReport() const {
        return debugReport;
    }
};

}  // namespace ast
}  // namespace souffle
