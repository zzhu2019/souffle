/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Pragma.cpp
 *
 * Define the class Pragma to update global options based on parameter.
 *
 ***********************************************************************/

#include "Pragma.h"
#include "Global.h"
#include "TranslationUnit.h"
#include "Visitor.h"

namespace souffle {
namespace ast {
bool PragmaChecker::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    Program* program = translationUnit.getProgram();

    visitDepthFirst(*program, [&](const Pragma& pragma) {
        std::pair<std::string, std::string> kvp = pragma.getkvp();
        // command line options take precedence
        // TODO (azreika): currently does not override default values if set
        if (!Global::config().has(kvp.first)) {
            changed = true;
            Global::config().set(kvp.first, kvp.second);
        }
    });
    return changed;
}
}  // namespace ast
}  // namespace souffle
