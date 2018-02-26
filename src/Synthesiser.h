/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Synthesiser.h
 *
 * Declares synthesiser classes to synthesise C++ code from a RAM program.
 *
 ***********************************************************************/

#pragma once

#include "RamProgram.h"
#include "RamRelation.h"
#include "RamTranslationUnit.h"
#include "SymbolTable.h"

#include "RamStatement.h"

#include <ostream>
#include <string>
#include <vector>

namespace souffle {

/**
 * A RAM synthesiser: synthesises a C++ program from a RAM program.
 */
class Synthesiser {
public:
    Synthesiser() = default;
    virtual ~Synthesiser() = default;

    /** Generate code
     *
     * @param tu translation unit
     * @param os output stream for generating code
     * @param id name of base identifier for tranlation unit
     */
    void generateCode(const RamTranslationUnit& tu, std::ostream& os, const std::string& id) const;
};
}  // end of namespace souffle
