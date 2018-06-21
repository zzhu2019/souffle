/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#pragma once

#include "FileFormatConverter.h"
#include "profilerlib/StringUtils.h"
#include "profilerlib/Tui.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace souffle {
namespace profile {

/*
 * CLI to parse command line arguments and start up the TUI to either run a single command,
 * generate the GUI file or run the TUI
 */
class Cli {
public:
    std::vector<std::string> args;

    Cli(int argc, char* argv[]) : args() {
        for (int i = 0; i < argc; i++) {
            args.push_back(std::string(argv[i]));
        }
    }

    void error() {
        std::cout << "Unknown error.\nTry souffle-profile -h for help.\n";
        exit(1);
    }

    void parse() {
        if (args.size() == 1) {
            std::cout << "No arguments provided.\nTry souffle-profile -h for help.\n";
            exit(1);
        }

        switch (args.at(1).at(0)) {
            case '-':
                switch (args.at(1).at(args.at(1).length() - 1)) {
                    case 'h':
                        std::cout << "Souffle Profiler v4.0.0" << std::endl
                                  << "Usage: souffle-profile -v | -h | <log-file> [ -c <command> | -o <file> "
                                     "[options] | -j | -l ]"
                                  << std::endl
                                  << "<log-file>            The log file to profile." << std::endl
                                  << "-c <command>          Run the given command on the log file, try with  "
                                     "'-c help' for a list"
                                  << std::endl
                                  << "                      of commands." << std::endl
                                  << "-j                    Generate a GUI (html/js) version of the profiler."
                                  << std::endl
                                  << "-l                    Run the profiler in live mode." << std::endl
                                  << "-o <file> [options]   Convert log file to a file in a format "
                                     "determined by its extension,"
                                  << std::endl
                                  << "                      try with '-o help' for more information."
                                  << std::endl
                                  << "-v                    Print the profiler version." << std::endl
                                  << "-h                    Print this help message." << std::endl;
                        break;
                    case 'o':
                        if (args.at(2).compare("help") != 0) {
                            std::cout << "Unknown argument " << args.at(2) << " for option " << args.at(1)
                                      << ".\nTry souffle-profile -h for help.\n";
                            exit(1);
                        }
                        std::cout
                                << "Souffle Profiler v4.0.0: Help: -o" << std::endl
                                << "Usage: souffle-profile <in-file.log> -o <out-file.ext> [k1=v1,k2=v2,...]"
                                << std::endl
                                << "Where: " << std::endl
                                << " - <in-file.log>      is an input log file produced by use of the "
                                   "'--profile' option to "
                                << std::endl
                                << "                      souffle." << std::endl
                                << " - <out-file.ext>     is a an output file, where 'ext' is the extension "
                                   "of that file. The"
                                << std::endl
                                << "                      only supported extension is 'csv'." << std::endl
                                << " - [k1=v1,k2=v2,...]  is a comma separated list of options as "
                                   "'key=value' pairs, such that"
                                << std::endl
                                << "                      boolean options can be enabled by providing only a "
                                   "'key'. The "
                                << std::endl
                                << "                      supported options are 'headers' to enable headers, "
                                   "and 'quotes' to "
                                << std::endl
                                << "                      enable quoted columns." << std::endl
                                << "Notes: " << std::endl
                                << "The user supplies a souffle log file, an output file with some "
                                   "particular extension, and"
                                << std::endl
                                << "(optionally) a list of options. The log file is read by souffle-profile, "
                                   "converted to a"
                                << std::endl
                                << "format determined by the extension of the output file and the given "
                                   "options, then written"
                                << std::endl
                                << "to the output file." << std::endl;
                        break;
                    case 'v':
                        std::cout << "Souffle Profiler v3.0.1\n";
                        break;
                    default:
                        std::cout << "Unknown option " << args.at(1)
                                  << ".\nTry souffle-profile -h for help.\n";
                        exit(1);
                        break;
                }
                break;
            default:

                std::string filename = args.at(1);

                if (args.size() == 2) {
                    Tui(filename, false, false).runProf();

                } else {
                    switch (args.at(2).at(args.at(2).length() - 1)) {
                        case 'c':
                            if (args.size() == 3) {
                                std::cout << "No arguments provided for option " << args.at(2)
                                          << ".\nTry souffle-profile -h for help.\n";
                                exit(1);
                            }
                            Tui(filename, false, false).runCommand(Tools::split(args.at(3), " "));
                            break;
                        case 'l':
                            Tui(filename, true, false).runProf();
                            break;
                        case 'j':
                            Tui(filename, false, true).outputJson();
                            break;
                        case 'o':
                            auto stringEndsWith = [](
                                    const std::string& str, const std::string& suffix) -> bool {
                                return (str.length() >= suffix.length() &&
                                        str.compare(str.length() - suffix.length(), suffix.length(),
                                                suffix) == 0);
                            };
                            auto stringToMap = [](const std::string& str, const std::string& sep,
                                    const std::string& delim) {
                                auto kvMap = std::map<std::string, std::string>();
                                if (str.find(delim) != std::string::npos) {
                                    for (const auto kvStr : Tools::split(str, std::string(delim))) {
                                        if (kvStr.find(sep) != std::string::npos) {
                                            const auto kv = Tools::split(kvStr, std::string(sep));
                                            kvMap[kv.at(0)] = kv.at(1);
                                        } else {
                                            kvMap[kvStr] = std::string();
                                        }
                                    }
                                } else {
                                    kvMap[str] = std::string();
                                }
                                return kvMap;
                            };
                            if (stringEndsWith(args.at(3), ".csv")) {
                                FileFormatConverter::fromLogToCsv(filename, args.at(3),
                                        stringToMap(
                                                (args.size() > 4) ? args.at(4) : std::string(), "=", ","));
                            }
                            break;
                    }
                }
                break;
        }
    }
};

}  // namespace profile
}  // namespace souffle
