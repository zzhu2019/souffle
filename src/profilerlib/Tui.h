/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2016, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#pragma once

#include "profilerlib/OutputProcessor.h"
#include "profilerlib/Reader.h"
#include "profilerlib/Table.h"
#include "profilerlib/UserInputReader.h"
#include <string>
#include <vector>

/*
 * Text User interface for SouffleProf
 * OutputProcessor creates a ProgramRun object
 * ProgramRun -> Reader.h ProgramRun stores all the data
 * OutputProcessor grabs the data and makes tables
 * Tui displays the data
 * TODO: move parts of the code into other classes, especially the outputJson function
 */
class Tui {
private:
    OutputProcessor out;
    bool loaded;
    std::string f_name;
    bool alive = false;
    int sort_col = 0;
    int precision = -1;
    Table rel_table_state;
    Table rul_table_state;
    std::shared_ptr<Reader> reader;
    InputReader linereader;

public:
    Tui(std::string filename, bool live, bool gui);

    void runCommand(std::vector<std::string> c);

    void runProf();

    void outputJson();

    void loadMenu();

    void quit();

    void save(std::string save_name);

    void load(std::string method, std::string load_file);

    static void help();

    void setupTabCompletion();

    void top();

    void rel();

    void rul();

    void id(std::string col);

    void relRul(std::string str);

    void verRul(std::string str);

    void iterRel(std::string c, std::string col);

    void iterRul(std::string c, std::string col);

    void verGraph(std::string c, std::string col);

    void graphD(std::vector<double> list);

    void graphL(std::vector<long> list);

    static bool string_sort(std::vector<std::string> a, std::vector<std::string> b);

protected:
    void verAtoms(Table& atomTable, const std::string& ruleName = "");
};
