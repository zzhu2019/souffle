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

#include "IndexSetAnalysis.h"
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
private:
    /** RAM identifier to C++ identifier map */
    std::map<const std::string, const std::string> identifiers;

protected:
    /** Convert RAM identifier */
    const std::string convertRamIdent(const std::string& name);

    /** Check whether indexes are disabled */
    bool areIndexesDisabled();

    /** Get relation name */
    const std::string getRelationName(const RamRelation& rel);

    /** Get context name */
    const std::string getOpContextName(const RamRelation& rel);

    /** Get relation type */
    std::string getRelationType(const RamRelation& rel, std::size_t arity, const IndexSet& indexes);

    /* Convert SearchColums to a template index */
    std::string toIndex(SearchColumns key);

    /** Get referenced relations */
    std::set<RamRelation> getReferencedRelations(const RamOperation& op);

    /** Generate code */
    void emitCode(std::ostream& out, const RamStatement& stmt);

public:
    Synthesiser() = default;
    virtual ~Synthesiser() = default;

    /** Generate code */
    void generateCode(const RamTranslationUnit& tu, std::ostream& os, const std::string& id);
};
}  // end of namespace souffle
