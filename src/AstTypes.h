/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file AstTypes.h
 *
 * Defines a numeric type for the AST, mimics "RamTypes.h"
 *
 ***********************************************************************/

#pragma once

#include <stdint.h>

namespace souffle {

/** ast domain that contains ram domain */
#ifdef AST_DOMAIN_TYPE
typedef AST_DOMAIN_TYPE AstDomain;
#else
typedef int64_t AstDomain;
#endif

/** Lower and upper boundaries for the AST domain. The range must be able to be stored in a RamDomain. By
 *  default, RamDomain is uint32_t and the allowed AstDomain range is that of a int32_t. **/
#define MIN_AST_DOMAIN (std::numeric_limits<AstDomain>::min())
#define MAX_AST_DOMAIN (std::numeric_limits<AstDomain>::max())
}  // namespace souffle
