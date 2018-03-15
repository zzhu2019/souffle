/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file FileFormatConverter.h
 *
 * Defines a class providing static methods for file format conversion.
 *
 ***********************************************************************/

#pragma once

#include <cassert>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace souffle {

namespace {

template <typename Error>
inline void error(const std::function<void(std::stringstream&)>& callback) {
    std::stringstream ss;
    callback(ss);
    throw Error(ss.str());
}

template <typename Data, const Data (*Consumer)(std::istream&)>
const Data read(const std::string& inputFilePath) {
    std::ifstream file(inputFilePath);
    if (!file.is_open()) {
        error<std::runtime_error>([&](std::stringstream& ss) {
            ss << "The input file " << inputFilePath << " could not be opened for reading.";
        });
    }
    const auto data = Consumer(file);
    file.close();
    return data;
}

template <typename Data, const Data (*Consumer)(const std::string&), char DELIMITER = '\0'>
const std::vector<Data> read(std::istream& inputStream) {
    assert((DELIMITER != '\0'));
    // TODO (lyndonhenry): this should permit PREFIX and SUFFIX as with write
    auto data = std::vector<Data>();
    std::string line;
    while (std::getline(inputStream, line, DELIMITER)) {
        data.push_back(Consumer(line));
    }
    return data;
}

template <char DELIMITER = '\0', char SINGLE_QUOTE = '\0', char DOUBLE_QUOTE = '\0', char ESCAPE = '\0',
        char WHITESPACE = '\0'>
const std::vector<std::string> readRow(const std::string& inputLine) {
    // TODO (lyndonhenry): should allow sensible defaults or modify behavior if any are null chars
    assert((DELIMITER != '\0' && SINGLE_QUOTE != '\0' && DOUBLE_QUOTE != '\0' && ESCAPE != '\0' &&
            WHITESPACE != '\0'));
    enum State {
        AFTER_DELIMITER,
        IN_SINGLE_QUOTES,
        IN_DOUBLE_QUOTES,
        NOT_IN_QUOTES,
        ESCAPED_IN_SINGLE_QUOTES,
        ESCAPED_IN_DOUBLE_QUOTES,
        BEFORE_DELIMITER
    };
    if (inputLine.empty()) {
        return std::vector<std::string>();
    }
    auto data = std::vector<std::string>();
    std::stringstream column;
    State state = AFTER_DELIMITER;
    for (const char& character : inputLine) {
        switch (state) {
            case AFTER_DELIMITER:
                switch (character) {
                    case SINGLE_QUOTE:
                        state = IN_SINGLE_QUOTES;
                        break;
                    case DOUBLE_QUOTE:
                        state = IN_DOUBLE_QUOTES;
                        break;
                    case WHITESPACE:
                        break;
                    case DELIMITER:
                        data.push_back(std::string());
                        break;
                    case ESCAPE:
                    default:
                        state = NOT_IN_QUOTES;
                        (void)(column << character);
                        break;
                }
                break;
            case IN_SINGLE_QUOTES:
                switch (character) {
                    case SINGLE_QUOTE:
                        state = BEFORE_DELIMITER;
                        break;
                    case ESCAPE:
                        state = ESCAPED_IN_SINGLE_QUOTES;
                        break;
                    case DOUBLE_QUOTE:
                    case WHITESPACE:
                    case DELIMITER:
                    default:
                        (void)(column << character);
                        break;
                }
                break;
            case IN_DOUBLE_QUOTES:
                switch (character) {
                    case DOUBLE_QUOTE:
                        state = BEFORE_DELIMITER;
                        break;
                    case ESCAPE:
                        state = ESCAPED_IN_DOUBLE_QUOTES;
                        break;
                    case SINGLE_QUOTE:
                    case WHITESPACE:
                    case DELIMITER:
                    default:
                        (void)(column << character);
                        break;
                }
                break;
            case NOT_IN_QUOTES:
                switch (character) {
                    case DELIMITER:
                        data.push_back(column.str());
                        column.str(std::string());
                        column.clear();
                        state = AFTER_DELIMITER;
                        break;
                    case SINGLE_QUOTE:
                    case DOUBLE_QUOTE:
                    case ESCAPE:
                    case WHITESPACE:
                    default:
                        (void)(column << character);
                        break;
                }
                break;
            case ESCAPED_IN_SINGLE_QUOTES:
                switch (character) {
                    case SINGLE_QUOTE:
                    case ESCAPE:
                        state = IN_SINGLE_QUOTES;
                        (void)(column << character);
                        break;
                    default:
                        error<std::runtime_error>([&](std::stringstream& ss) {
                            ss << "Unexpected character '" << character << "' in input.";
                        });
                }
                break;
            case ESCAPED_IN_DOUBLE_QUOTES:
                switch (character) {
                    case DOUBLE_QUOTE:
                    case ESCAPE:
                        state = IN_DOUBLE_QUOTES;
                        (void)(column << character);
                        break;
                    default:
                        error<std::runtime_error>([&](std::stringstream& ss) {
                            ss << "Unexpected character '" << character << "' in input.";
                        });
                }
                break;
            case BEFORE_DELIMITER:
                switch (character) {
                    case DELIMITER:
                        data.push_back(column.str());
                        column.str(std::string());
                        column.clear();
                        break;
                    case WHITESPACE:
                        break;
                    case SINGLE_QUOTE:
                    case DOUBLE_QUOTE:
                    case ESCAPE:
                    default:
                        error<std::runtime_error>([&](std::stringstream& ss) {
                            ss << "Unexpected character '" << character << "' in input.";
                        });
                }
                break;
        }
    }
    data.push_back(column.str());
    return data;
}

template <typename Data, void (*Producer)(std::ostream&, const Data&)>
void write(const std::string& filePath, const Data& data) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        error<std::runtime_error>([&](std::stringstream& ss) {
            ss << "The output file " << filePath << " could not be opened for writing.";
        });
    }
    Producer(file, data);
    file.close();
}

template <typename Data, void (*Producer)(std::ostream&, const Data&), char DELIMITER = '\0',
        char PREFIX = '\0', char SUFFIX = '\0'>
void write(std::ostream& outputStream, const std::vector<Data>& data) {
    if (data.empty()) {
        return;
    }
    if (PREFIX != '\0') {
        outputStream << PREFIX;
    }
    auto it = data.begin();
    Producer(outputStream, *it);
    ++it;
    for (; it != data.end(); ++it) {
        if (DELIMITER != '\0') {
            outputStream << DELIMITER;
        }
        Producer(outputStream, *it);
    }
    if (SUFFIX != '\0') {
        outputStream << SUFFIX;
    }
}

template <char QUOTE = '\0', char ESCAPE = '\0'>
void writeColumn(std::ostream& outputStream, const std::string& data) {
    if (QUOTE != '\0') outputStream << QUOTE;
    for (const char character : data) {
        if (character == QUOTE) {
            if (ESCAPE == '\0') {
                error<std::runtime_error>([&](std::stringstream& ss) {
                    ss << "A '" << QUOTE
                       << "' character cannot occur within quotes without defining an escape.";
                });
            }
            outputStream << ESCAPE << QUOTE;
        } else {
            outputStream << character;
        }
    }
    if (QUOTE != '\0') outputStream << QUOTE;
}
}

class FileFormatConverter {
private:
    static const std::vector<std::vector<std::string>> transformBySchema(
            const std::map<std::string, std::vector<std::string>>& schema,
            const std::vector<std::vector<std::string>>& oldData) {
        auto headers = std::set<std::string>();
        for (const auto& pair : schema) {
            headers.insert(pair.second.begin(), pair.second.end());
        }
        auto headerToIndex = std::map<std::string, size_t>();
        size_t i = 1;
        for (const auto& header : headers) {
            headerToIndex.insert({{header, i}});
            ++i;
        }
        auto firstRow = std::vector<std::string>(headers.size() + 1);
        firstRow[0] = "@";
        for (const auto& pair : headerToIndex) {
            firstRow[pair.second] = pair.first;
        }
        auto newData = std::vector<std::vector<std::string>>(oldData.size() + 1);
        newData[0] = firstRow;
        i = 1;
        for (const auto& oldRow : oldData) {
            auto newRow = std::vector<std::string>(firstRow.size());
            newRow[0] = oldRow.at(0);
            const auto& rowSchema = schema.at(oldRow.at(0));
            for (size_t oldIndex = 1; oldIndex < oldRow.size(); ++oldIndex) {
                newRow[headerToIndex.at(rowSchema.at(oldIndex - 1))] = oldRow.at(oldIndex);
            }
            newData[i] = newRow;
            ++i;
        }
        return newData;
    }

public:
    static void fromLogToCsv(const std::string& inputPath, const std::string& outputPath) {
        const std::map<std::string, std::vector<std::string>> schema = {{"@start-debug", {}},
                {"@t-nonrecursive-relation", {"relation", "src-locator", "time"}},
                {"@n-nonrecursive-relation", {"relation", "src-locator", "tuples"}},
                {"@t-nonrecursive-rule", {"relation", "src-locator", "rule", "time"}},
                {"@n-nonrecursive-rule", {"relation", "src-locator", "rule", "tuples"}},
                {"@t-recursive-rule", {"relation", "version", "src-locator", "rule", "time"}},
                {"@n-recursive-rule", {"relation", "version", "src-locator", "rule", "tuples"}},
                {"@t-recursive-relation", {"relation", "src-locator", "time"}},
                {"@n-recursive-relation", {"relation", "src-locator", "tuples"}},
                {"@c-recursive-relation", {"relation", "src-locator", "copy-time"}},
                {"@runtime", {"total-time"}}};
        auto logData = read<std::vector<std::vector<std::string>>,
                read<std::vector<std::string>, readRow<';', '\'', '\"', '\\', ' '>, '\n'>>(inputPath);
        auto csvData = transformBySchema(schema, logData);
        write<std::vector<std::vector<std::string>>,
                write<std::vector<std::string>, write<std::string, writeColumn<'\'', '\\'>, ','>, '\n'>>(
                outputPath, csvData);
    }

    // TODO (lyndonhenry): should do a 'fromLogToJson' member function
};

}  // end of namespace souffle
