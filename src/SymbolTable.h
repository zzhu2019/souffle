/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SymbolTable.h
 *
 * Data container to store symbols of the Datalog program.
 *
 ***********************************************************************/

#pragma once

#include "ParallelUtils.h"
#include "RamTypes.h"
#include "Util.h"

#include <deque>
#include <initializer_list>
#include <iostream>
#include <string>
#include <unordered_map>

namespace souffle {

/**
 * @class SymbolTable
 *
 * Global pool of re-usable strings
 *
 * SymbolTable stores Datalog symbols and converts them to numbers and vice versa.
 */
class SymbolTable {
private:
    /** A lock to synchronize parallel accesses */
    mutable std::mutex lock;

    /** Map indices to strings. */
    std::deque<std::string> numToStr;

    /** Map strings to indices. */
    std::unordered_map<std::string, size_t> strToNum;

    /** Convenience method to place a new symbol in the table, if it does not exist, and return the index of
     * it. */
    inline size_t newSymbolOfIndex(const std::string& symbol) {
        size_t index;
        auto it = strToNum.find(symbol);
        if (it == strToNum.end()) {
            index = numToStr.size();
            strToNum[symbol] = index;
            numToStr.push_back(symbol);
        } else {
            index = it->second;
        }
        return index;
    }

    /** Convenience method to place a new symbol in the table, if it does not exist. */
    inline void newSymbol(const std::string& symbol) {
        if (strToNum.find(symbol) == strToNum.end()) {
            strToNum[symbol] = numToStr.size();
            numToStr.push_back(symbol);
        }
    }

public:
    /** Empty constructor. */
    SymbolTable() = default;

    /** Copy constructor, performs a deep copy. */
    SymbolTable(const SymbolTable& other) : numToStr(other.numToStr), strToNum(other.strToNum) {}

    /** Copy constructor for r-value reference. */
    SymbolTable(SymbolTable&& other) noexcept {
        numToStr.swap(other.numToStr);
        strToNum.swap(other.strToNum);
    }

    SymbolTable(std::initializer_list<std::string> symbols) {
        strToNum.reserve(symbols.size());
        for (const auto& symbol : symbols) {
            newSymbol(symbol);
        }
    }

    /** Destructor, frees memory allocated for all strings. */
    virtual ~SymbolTable() = default;

    /** Assignment operator, performs a deep copy and frees memory allocated for all strings. */
    SymbolTable& operator=(const SymbolTable& other) {
        if (this == &other) {
            return *this;
        }
        numToStr = other.numToStr;
        strToNum = other.strToNum;
        return *this;
    }

    /** Assignment operator for r-value references. */
    SymbolTable& operator=(SymbolTable&& other) noexcept {
        numToStr.swap(other.numToStr);
        strToNum.swap(other.strToNum);
        return *this;
    }

    /** Find the index of a symbol in the table, inserting a new symbol if it does not exist there already. */
    RamDomain lookup(const std::string& symbol) {
        std::lock_guard<std::mutex> guard(lock);
        return static_cast<RamDomain>(newSymbolOfIndex(symbol));
    }

    /** Finds the index of a symbol in the table, giving an error if it's not found */
    RamDomain lookupExisting(const std::string& symbol) const {
        std::lock_guard<std::mutex> guard(lock);
        auto result = strToNum.find(symbol);
        if (result == strToNum.end()) {
            std::cerr << "Error string not found in call to SymbolTable::lookupExisting.\n";
            exit(1);
        }
        return static_cast<RamDomain>(result->second);
    }

    /** Find the index of a symbol in the table, inserting a new symbol if it does not exist there already. */
    RamDomain unsafeLookup(const std::string& symbol) {
        return newSymbolOfIndex(symbol);
    }

    /** Find a symbol in the table by its index, note that this gives an error if the index is out of bounds.
     */
    const std::string& resolve(const RamDomain index) const {
        std::lock_guard<std::mutex> guard(lock);
        auto pos = static_cast<size_t>(index);
        if (pos >= size()) {
            // TODO: use different error reporting here!!
            std::cerr << "Error index out of bounds in call to SymbolTable::resolve.\n";
            exit(1);
        }
        return numToStr[pos];
    }

    const std::string& unsafeResolve(const RamDomain index) const {
        return numToStr[static_cast<size_t>(index)];
    }

    /* Return the size of the symbol table, being the number of symbols it currently holds. */
    size_t size() const {
        return numToStr.size();
    }

    /** Bulk insert symbols into the table, note that this operation is more efficient than repeated inserts
     * of single symbols. */
    void insert(const std::vector<std::string>& symbols) {
        std::lock_guard<std::mutex> guard(lock);
        strToNum.reserve(size() + symbols.size());
        for (auto& symbol : symbols) {
            newSymbol(symbol);
        }
    }

    /** Insert a single symbol into the table, not that this operation should not be used if inserting symbols
     * in bulk. */
    void insert(const std::string& symbol) {
        std::lock_guard<std::mutex> guard(lock);
        newSymbol(symbol);
    }

    /** Print the symbol table to the given stream. */
    void print(std::ostream& out) const {
        out << "SymbolTable: {\n\t";
        out << join(strToNum, "\n\t", [](std::ostream& out,
                                              const std::pair<std::string, std::size_t>& entry) {
            out << entry.first << "\t => " << entry.second;
        }) << "\n";
        out << "}\n";
    }

    std::unique_lock<std::mutex> acquireLock() const {
        return std::unique_lock<std::mutex>(lock);
    }

    /** Stream operator, used as a convenience for print. */
    friend std::ostream& operator<<(std::ostream& out, const SymbolTable& table) {
        table.print(out);
        return out;
    }
};
}  // namespace souffle
