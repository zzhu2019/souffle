/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#pragma once

#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

/*
 * Class to hold information about souffle Rule profile information
 */
class Rule {
protected:
    std::string name;
    double runtime = 0;
    long num_tuples = 0;
    std::string identifier;
    std::string locator = "";
    std::map<std::string, std::tuple<std::string, long, long>> atoms{};

private:
    bool recursive = false;
    int version = 0;

public:
    Rule(std::string name, std::string id) : name(std::move(name)), identifier(std::move(id)) {}

    Rule(std::string name, int version, std::string id)
            : name(std::move(name)), identifier(std::move(id)), recursive(true), version(version) {}

    inline std::string getId() {
        return identifier;
    }

    inline double getRuntime() {
        return runtime;
    }

    inline long getNum_tuples() {
        return num_tuples;
    }

    inline void setRuntime(double runtime) {
        this->runtime = runtime;
    }

    inline void setNum_tuples(long num_tuples) {
        this->num_tuples = num_tuples;
    }

    inline void addAtomFrequency(
            const std::string& subruleName, std::string atom, long version, long frequency) {
        atoms[subruleName] = std::make_tuple(std::move(atom), version, frequency);
    }

    const std::map<std::string, std::tuple<std::string, long, long>>& getAtoms() {
        return atoms;
    }
    inline std::string getName() {
        return name;
    }

    inline void setId(std::string id) {
        identifier = id;
    }

    inline std::string getLocator() {
        return locator;
    }

    void setLocator(std::string locator) {
        this->locator = locator;
    }

    inline bool isRecursive() {
        return recursive;
    }

    inline void setRecursive(bool recursive) {
        this->recursive = recursive;
    }

    inline int getVersion() {
        return version;
    }

    inline void setVersion(int version) {
        this->version = version;
    }

    std::string toString();
};
