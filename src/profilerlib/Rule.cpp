/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "profilerlib/Rule.h"
#include <sstream>

std::string Rule::toString() {
    std::ostringstream output;
    if (recursive) {
        output << "{" << name << "," << version << ":";
    } else {
        output << "{" << name << ":";
    }
    output << "[" << runtime << "," << num_tuples << "]}";
    return output.str();
}
