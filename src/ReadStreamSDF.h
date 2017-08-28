/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ReadStreamSDF.h
 *
 ***********************************************************************/

#pragma once

#include "RamTypes.h"
#include "ReadStream.h"
#include "SymbolMask.h"
#include "SymbolTable.h"
#include "Util.h"
#include <fstream>

#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace souffle {

class ReadStreamSDF : public ReadStream {
public:
    ReadStreamSDF(const std::string filename, const SymbolMask& symbolMask, SymbolTable& symbolTable,
            const bool provenance = false)
            : ReadStream(symbolMask, symbolTable, provenance), fileName(std::move(filename)),
              file(fileName, std::ifstream::binary) {
        if (!file.is_open()) {
            throw std::invalid_argument("Cannot open fact file " + souffle::baseName(filename) + "\n");
        }
        readSymbolTable();
        char version, arity;
        file.read(&version, 1);
        file.read(&arity, 1);
        if (arity != symbolMask.getArity()) {
            std::stringstream errorMessage;
            errorMessage << "Fact file " << souffle::baseName(filename) << " has incorrect arity. Was "
                         << arity << ", expected " << symbolMask.getArity() << "\n";
            throw std::invalid_argument(errorMessage.str());
        }

        char symbolMaskEntry;
        for (size_t i = 0; i < symbolMask.getArity(); ++i) {
            file.read(&symbolMaskEntry, 1);
        }
    }

    ~ReadStreamSDF() override = default;

protected:
    void readSymbolTable() {
        std::ifstream symbolFile(fileName + ".symbols");
        if (!symbolFile.is_open()) {
            throw std::invalid_argument(
                    "Cannot open fact file " + souffle::baseName(fileName) + ".symbols\n");
        }
        uint32_t oldIndex;
        std::string symbol;

        auto lease = symbolTable.acquireLock();
        (void)lease;
        while (!symbolFile.eof()) {
            if (!(symbolFile >> oldIndex)) {
                break;
            }
            symbolFile >> symbol;
            idMap[oldIndex] = symbolTable.unsafeLookup(symbol.c_str());
        }
    }

    /**
     * Read and return the next tuple.
     *
     * Returns nullptr if no tuple was readable.
     * @return
     */
    std::unique_ptr<RamDomain[]> readNextTuple() override {
        if (!file) {
            return nullptr;
        }
        std::unique_ptr<RamDomain[]> tuple(new RamDomain[symbolMask.getArity()]);
        std::stringstream tupleStream;

        for (uint32_t column = 0; column < symbolMask.getArity(); ++column) {
            file.read(reinterpret_cast<char*>(&tuple[column]), sizeof(tuple[column]));
            // Check that a complete tuple was read.
            if (file.gcount() == 0) {
                return nullptr;
            } else if (file.gcount() != sizeof(tuple[column])) {
                std::stringstream errorMessage;
                errorMessage << "Cannot parse fact file " << souffle::baseName(fileName) << "!\n";
                throw std::invalid_argument(errorMessage.str());
            }
            if (symbolMask.isSymbol(column)) {
                tuple[column] = idMap[tuple[column]];
            }
            tupleStream << tuple[column] << ' ';
        }
        return tuple;
    }

    std::string fileName;
    std::ifstream file;
    std::map<uint32_t, uint32_t> idMap;
};

class ReadStreamSDFFactory : public ReadStreamFactory {
public:
    std::unique_ptr<ReadStream> getReader(const SymbolMask& symbolMask, SymbolTable& symbolTable,
            const IODirectives& ioDirectives, const bool provenance) override {
        std::string filename = ioDirectives.has("filename") ? ioDirectives.get("filename")
                                                            : (ioDirectives.getRelationName() + ".facts");
        return std::unique_ptr<ReadStreamSDF>(
                new ReadStreamSDF(filename, symbolMask, symbolTable, provenance));
    }
    const std::string& getName() const override {
        static const std::string name = "SDFile";
        return name;
    }

    ~ReadStreamSDFFactory() override = default;
};

} /* namespace souffle */
