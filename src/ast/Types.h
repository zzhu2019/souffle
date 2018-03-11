/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Types.h
 *
 * Defines a numeric type for the AST, mimics "RamTypes.h"
 *
 ***********************************************************************/

#pragma once

#include <stdint.h>

#include "RamTypes.h"

namespace souffle {
namespace ast {

/** ast domain that contains ram domain */
#ifdef AST_DOMAIN_TYPE
typedef AST_DOMAIN_TYPE Domain;
#else
typedef int64_t Domain;
#endif

/** Lower and upper boundaries for the AST domain. The range must be able to be
 * stored in a RamDomain, so we simply compute the limits of that type. **/
#define MIN_AST_DOMAIN (std::numeric_limits<RamDomain>::min())
#define MAX_AST_DOMAIN (std::numeric_limits<RamDomain>::max())

}  // namespace ast
}  // namespace souffle
