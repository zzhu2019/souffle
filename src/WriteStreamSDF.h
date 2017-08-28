/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file WriteStreamSDF.h
 *
 ***********************************************************************/

#pragma once

#include "SymbolMask.h"
#include "SymbolTable.h"
#include "WriteStream.h"

#include <fstream>
#include <map>
#include <memory>
#include <ostream>
#include <string>

namespace souffle {

class WriteFileSDF : public WriteStream {
public:
    WriteFileSDF(const std::string filename, const SymbolMask& symbolMask, const SymbolTable& symbolTable,
            const bool provenance = false)
            : WriteStream(symbolMask, symbolTable, provenance), filename(std::move(filename)),
              file(filename, std::ofstream::binary) {
        // Write SDF version header
        file << static_cast<uint8_t>(1);
        // Write arity
        file << static_cast<uint8_t>(symbolMask.getArity());
        // write symbolmask (wasting a few bytes for easier reading later)
        for (size_t i = 0; i < symbolMask.getArity(); ++i) {
            file << static_cast<uint8_t>(symbolMask.isSymbol(i));
        }
    }

    ~WriteFileSDF() override {
        std::ofstream symbols(filename + ".symbols");
        for (auto current : symbolMap) {
            symbols << current.first << '\t' << current.second << '\n';
        }
    };

protected:
    void writeNextTuple(const RamDomain* tuple) override {
        size_t arity = symbolMask.getArity();
        if (arity == 0) {
            file << 1;
            return;
        }
        for (size_t column = 0; column < arity; ++column) {
            std::cout << "Writing <" << static_cast<uint32_t>(tuple[column]) << '>' << std::endl;
            if (symbolMask.isSymbol(column)) {
                symbolMap[tuple[column]] = symbolTable.unsafeResolve(tuple[column]);
            }
            file.write(reinterpret_cast<const char*>(&tuple[column]), sizeof(tuple[column]));
        }
    }

    std::map<uint32_t, const char*> symbolMap;
    const std::string delimiter;
    const std::string filename;
    std::ofstream file;
};

class WriteFileSDFFactory : public WriteStreamFactory {
public:
    std::unique_ptr<WriteStream> getWriter(const SymbolMask& symbolMask, const SymbolTable& symbolTable,
            const IODirectives& ioDirectives, const bool provenance) override {
        return std::unique_ptr<WriteFileSDF>(
                new WriteFileSDF(ioDirectives.get("filename"), symbolMask, symbolTable, provenance));
    }
    const std::string& getName() const override {
        static const std::string name = "SDFile";
        return name;
    }
    ~WriteFileSDFFactory() override = default;
};

} /* namespace souffle */
