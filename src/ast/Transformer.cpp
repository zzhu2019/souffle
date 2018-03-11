/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Transformer.cpp
 *
 * Defines the interface for AST transformation passes.
 *
 ***********************************************************************/

#include "Transformer.h"
#include "TranslationUnit.h"

namespace souffle {
namespace ast {

bool Transformer::apply(TranslationUnit& translationUnit) {
    bool changed = transform(translationUnit);
    if (changed) {
        translationUnit.invalidateAnalyses();
    }
    return changed;
}

bool MetaTransformer::applySubtransformer(TranslationUnit& translationUnit, Transformer* transformer) {
    auto start = std::chrono::high_resolution_clock::now();
    bool changed = transformer->apply(translationUnit);
    auto end = std::chrono::high_resolution_clock::now();

    if (verbose && !dynamic_cast<MetaTransformer*>(transformer)) {
        std::cout << transformer->getName() << " time: " << std::chrono::duration<double>(end - start).count()
                  << "sec" << std::endl;
    }

    /* Abort evaluation of the program if errors were encountered */
    if (translationUnit.getErrorReport().getNumErrors() != 0) {
        std::cerr << translationUnit.getErrorReport();
        std::cerr << std::to_string(translationUnit.getErrorReport().getNumErrors()) +
                             " errors generated, evaluation aborted"
                  << std::endl;
        exit(1);
    }

    return changed;
}

}  // namespace ast
}  // namespace souffle
