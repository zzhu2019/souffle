/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ParserDriver.h
 *
 * Defines the parser driver.
 *
 ***********************************************************************/
#pragma once

#include "parser.hh"

#include <memory>

#define YY_DECL yy::parser::symbol_type yylex(ParserDriver& driver, yyscan_t yyscanner)
YY_DECL;

namespace souffle {
namespace ast {
class TranslationUnit;
class Relation;
class Type;
class Program;
class Pragma;
}
class DebugReport;
class ErrorReport;
class SymbolTable;

typedef void* yyscan_t;

struct scanner_data {
    ast::SrcLocation yylloc;

    /* Stack of parsed files */
    const char* yyfilename;
};

class ParserDriver {
public:
    ParserDriver();
    virtual ~ParserDriver();

    std::unique_ptr<ast::TranslationUnit> translationUnit;

    void addRelation(std::unique_ptr<ast::Relation> r);
    void addIODirective(std::unique_ptr<ast::IODirective> d);
    void addIODirectiveChain(std::unique_ptr<ast::IODirective> d);
    void addType(std::unique_ptr<ast::Type> type);
    void addClause(std::unique_ptr<ast::Clause> c);
    void addComponent(std::unique_ptr<ast::Component> c);
    void addInstantiation(std::unique_ptr<ast::ComponentInit> ci);
    void addPragma(std::unique_ptr<ast::Pragma> p);

    souffle::SymbolTable& getSymbolTable();

    bool trace_scanning;

    std::unique_ptr<ast::TranslationUnit> parse(const std::string& filename, FILE* in, SymbolTable& symbolTable,
            ErrorReport& errorReport, DebugReport& debugReport);
    std::unique_ptr<ast::TranslationUnit> parse(const std::string& code, SymbolTable& symbolTable,
            ErrorReport& errorReport, DebugReport& debugReport);
    static std::unique_ptr<ast::TranslationUnit> parseTranslationUnit(const std::string& filename, FILE* in,
            SymbolTable& symbolTable, ErrorReport& errorReport, DebugReport& debugReport);
    static std::unique_ptr<ast::TranslationUnit> parseTranslationUnit(const std::string& code,
            SymbolTable& symbolTable, ErrorReport& errorReport, DebugReport& debugReport);

    bool trace_parsing;

    void error(const ast::SrcLocation& loc, const std::string& msg);
    void error(const std::string& msg);
};

}  // end of namespace souffle
