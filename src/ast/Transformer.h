/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Transformer.h
 *
 * Defines the interface for AST transformation passes.
 *
 ***********************************************************************/
#pragma once

#include <string>

namespace souffle {
namespace ast {

class TranslationUnit;

class Transformer {
private:
    virtual bool transform(TranslationUnit& translationUnit) = 0;

public:
    virtual ~Transformer() = default;

    bool apply(TranslationUnit& translationUnit);

    virtual std::string getName() const = 0;
};

/**
 * Transformer that coordinates other sub-transformations
 */
class MetaTransformer : public Transformer {
protected:
    bool verbose = false;

public:
    /* Enable the debug-report for all sub-transformations */
    virtual void setDebugReport() = 0;

    /* Enable high verbosity */
    virtual void setVerbosity(bool verbose) = 0;

    /* Apply a nested transformer */
    bool applySubtransformer(TranslationUnit& translationUnit, Transformer* transformer);
};

}  // namespace ast
}  // namespace souffle
