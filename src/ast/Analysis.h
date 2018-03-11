/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Analysis.h
 *
 * Defines an interface for AST analysis
 *
 ***********************************************************************/
#pragma once

#include <iostream>

namespace souffle {
namespace ast {

class TranslationUnit;

/**
 * Abstract class for a AST Analysis.
 */
class Analysis {
public:
    virtual ~Analysis() = default;

    /** run analysis for a RAM translation unit */
    virtual void run(const TranslationUnit& translationUnit) = 0;

    /** print the analysis result in HTML format */
    virtual void print(std::ostream& os) const {}

    /** define output stream operator */
    friend std::ostream& operator<<(std::ostream& out, const Analysis& other) {
        other.print(out);
        return out;
    }
};

}  // namespace ast
}  // namespace souffle
