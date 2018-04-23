/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "Iteration.h"

#include <iostream>

void Iteration::addRule(std::vector<std::string> data, std::string rec_id) {
    std::string strTemp = data[4] + data[3] + data[2];

    std::shared_ptr<Rule> rul_rec = nullptr;
    if (rul_rec_map.find(strTemp) == rul_rec_map.end()) {
        rul_rec = std::make_shared<Rule>(Rule(data[4], std::stoi(data[2]), rec_id));
        rul_rec->setRuntime(0);
        rul_rec_map[strTemp] = rul_rec;
    } else {
        rul_rec = rul_rec_map[strTemp];
    }

    if (data[0].at(0) == 't') {
        rul_rec->setRuntime(std::stod(data[7]) + rul_rec->getRuntime());
        rul_rec->setLocator(data[3]);
    } else if (data[0].at(0) == 'n') {
        rul_rec->setNum_tuples(std::stol(data[5]) - prev_num_tuples);
        this->prev_num_tuples = std::stol(data[5]);
    }
}

std::string Iteration::toString() {
    std::ostringstream output;

    output << runtime << "," << num_tuples << "," << copy_time << ",";
    output << " recRule:";
    for (auto& rul : rul_rec_map) output << rul.second->toString();
    output << "\n";
    return output.str();
}
