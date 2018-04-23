/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file file_format_converter.cpp
 *
 * Tests for FileFormatConverter.h.
 *
 ***********************************************************************/

#include "FileFormatConverter.h"
#include "test.h"
#include <cstdio>
#include <sstream>

namespace souffle {

namespace test {

TEST(FileFormatConverter, fromLogToCsv) {
    // constants for the log files
    struct {
        const std::string FILE_PATH = "/tmp/souffle_file_format_converter_test.log";
        const std::string DATA =
                "@start-debug\n"
                "@t-nonrecursive-rule;RelName;/path/file.dl [8:1-8:35];RelName(x,y) :-     "
                "link(_,x,y).;0;0;2.4863e-05\n"
                "@n-nonrecursive-rule;RelName;/path/file.dl [8:1-8:35];RelName(x,y) :-     link(_,x,y).;8\n"
                "@t-nonrecursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;0.000118451\n"
                "@n-nonrecursive-relation;RelName;/path/file.dl [4:7-0:0];8\n"
                "@t-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;0;0;5.0565e-05\n"
                "@n-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;6\n"
                "@t-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;9.8336e-05\n"
                "@n-recursive-relation;RelName;/path/file.dl [4:7-0:0];6\n"
                "@c-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;9.777e-06\n"
                "@t-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;0;0;2.9892e-05\n"
                "@n-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;4\n"
                "@t-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;7.2635e-05\n"
                "@n-recursive-relation;RelName;/path/file.dl [4:7-0:0];4\n"
                "@c-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;8.381e-06\n"
                "@t-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;0;0;1.9835e-05\n"
                "@n-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;2\n"
                "@t-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;6.0622e-05\n"
                "@n-recursive-relation;RelName;/path/file.dl [4:7-0:0];2\n"
                "@c-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;7.263e-06\n"
                "@t-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;0;0;1.5644e-05\n"
                "@n-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;1\n"
                "@t-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;5.6152e-05\n"
                "@n-recursive-relation;RelName;/path/file.dl [4:7-0:0];1\n"
                "@c-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;6.984e-06\n"
                "@t-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;0;0;0\n"
                "@n-recursive-rule;RelName;0;/path/file.dl [9:1-9:57];RelName(x,y) :-     RelName(x,z),    "
                "link(_,z,y).;0\n"
                "@t-recursive-relation;RelName;/path/file.dl [4:7-0:0];0;0;0\n"
                "@n-recursive-relation;RelName;/path/file.dl [4:7-0:0];0\n"
                "@runtime;0;0;0.00126608\n";

    } LOG;

    // constants for the csv files
    struct {
        const std::string FILE_PATH = "/tmp/souffle_file_format_converter_test.csv";
        struct {
            const std::map<std::string, std::string> DEFAULTS{};
            const std::map<std::string, std::string> HEADERS{{"headers", ""}};
            const std::map<std::string, std::string> QUOTES{{"quotes", ""}};
            const std::map<std::string, std::string> HEADERS_AND_QUOTES{{"headers", ""}, {"quotes", ""}};
        } CONFIG_WITH;
        struct {
            const std::string DEFAULTS =
                    "@start-debug,,,,,,,,,,\n"
                    "@t-nonrecursive-rule,,0,RelName,RelName(x,y) :-     link(_,x,y).,/path/file.dl "
                    "[8:1-8:35],0,2.4863e-05,,,\n"
                    "@n-nonrecursive-rule,,,RelName,RelName(x,y) :-     link(_,x,y).,/path/file.dl "
                    "[8:1-8:35],,,,8,\n"
                    "@t-nonrecursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,0.000118451,,,\n"
                    "@n-nonrecursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,8,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,5.0565e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,6,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,9.8336e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,6,\n"
                    "@c-recursive-relation,9.777e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,2.9892e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,4,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,7.2635e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,4,\n"
                    "@c-recursive-relation,8.381e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,1.9835e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,2,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,6.0622e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,2,\n"
                    "@c-recursive-relation,7.263e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,1.5644e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,1,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,5.6152e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,1,\n"
                    "@c-recursive-relation,6.984e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,0,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,0,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,0,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,0,\n"
                    "@runtime,,0,,,,0,,0.00126608,,\n";

            const std::string HEADERS =
                    "@,copy-time,end-time,relation,rule,src-locator,start-time,time,total-time,tuples,"
                    "version\n"
                    "@start-debug,,,,,,,,,,\n"
                    "@t-nonrecursive-rule,,0,RelName,RelName(x,y) :-     link(_,x,y).,/path/file.dl "
                    "[8:1-8:35],0,2.4863e-05,,,\n"
                    "@n-nonrecursive-rule,,,RelName,RelName(x,y) :-     link(_,x,y).,/path/file.dl "
                    "[8:1-8:35],,,,8,\n"
                    "@t-nonrecursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,0.000118451,,,\n"
                    "@n-nonrecursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,8,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,5.0565e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,6,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,9.8336e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,6,\n"
                    "@c-recursive-relation,9.777e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,2.9892e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,4,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,7.2635e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,4,\n"
                    "@c-recursive-relation,8.381e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,1.9835e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,2,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,6.0622e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,2,\n"
                    "@c-recursive-relation,7.263e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,1.5644e-05,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,1,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,5.6152e-05,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,1,\n"
                    "@c-recursive-relation,6.984e-06,0,RelName,,/path/file.dl [4:7-0:0],0,,,,\n"
                    "@t-recursive-rule,,0,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],0,0,,,0\n"
                    "@n-recursive-rule,,,RelName,RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).,/path/file.dl [9:1-9:57],,,,0,0\n"
                    "@t-recursive-relation,,0,RelName,,/path/file.dl [4:7-0:0],0,0,,,\n"
                    "@n-recursive-relation,,,RelName,,/path/file.dl [4:7-0:0],,,,0,\n"
                    "@runtime,,0,,,,0,,0.00126608,,\n";

            const std::string QUOTES =
                    "'@start-debug','','','','','','','','','',''\n"
                    "'@t-nonrecursive-rule','','0','RelName','RelName(x,y) :-     "
                    "link(_,x,y).','/path/file.dl [8:1-8:35]','0','2.4863e-05','','',''\n"
                    "'@n-nonrecursive-rule','','','RelName','RelName(x,y) :-     "
                    "link(_,x,y).','/path/file.dl [8:1-8:35]','','','','8',''\n"
                    "'@t-nonrecursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','0.000118451','','',''\n"
                    "'@n-nonrecursive-relation','','','RelName','','/path/file.dl "
                    "[4:7-0:0]','','','','8',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','5.0565e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','6','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','9.8336e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','6',''\n"
                    "'@c-recursive-relation','9.777e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','2.9892e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','4','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','7.2635e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','4',''\n"
                    "'@c-recursive-relation','8.381e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','1.9835e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','2','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','6.0622e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','2',''\n"
                    "'@c-recursive-relation','7.263e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','1.5644e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','1','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','5.6152e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','1',''\n"
                    "'@c-recursive-relation','6.984e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','0','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','0','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl [4:7-0:0]','0','0','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','0',''\n"
                    "'@runtime','','0','','','','0','','0.00126608','',''\n";

            const std::string HEADERS_AND_QUOTES =
                    "'@','copy-time','end-time','relation','rule','src-locator','start-time','time','total-"
                    "time','tuples','version'\n"
                    "'@start-debug','','','','','','','','','',''\n"
                    "'@t-nonrecursive-rule','','0','RelName','RelName(x,y) :-     "
                    "link(_,x,y).','/path/file.dl [8:1-8:35]','0','2.4863e-05','','',''\n"
                    "'@n-nonrecursive-rule','','','RelName','RelName(x,y) :-     "
                    "link(_,x,y).','/path/file.dl [8:1-8:35]','','','','8',''\n"
                    "'@t-nonrecursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','0.000118451','','',''\n"
                    "'@n-nonrecursive-relation','','','RelName','','/path/file.dl "
                    "[4:7-0:0]','','','','8',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','5.0565e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','6','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','9.8336e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','6',''\n"
                    "'@c-recursive-relation','9.777e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','2.9892e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','4','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','7.2635e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','4',''\n"
                    "'@c-recursive-relation','8.381e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','1.9835e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','2','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','6.0622e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','2',''\n"
                    "'@c-recursive-relation','7.263e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','1.5644e-05','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','1','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','5.6152e-05','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','1',''\n"
                    "'@c-recursive-relation','6.984e-06','0','RelName','','/path/file.dl "
                    "[4:7-0:0]','0','','','',''\n"
                    "'@t-recursive-rule','','0','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','0','0','','','0'\n"
                    "'@n-recursive-rule','','','RelName','RelName(x,y) :-     RelName(x,z),    "
                    "link(_,z,y).','/path/file.dl [9:1-9:57]','','','','0','0'\n"
                    "'@t-recursive-relation','','0','RelName','','/path/file.dl [4:7-0:0]','0','0','','',''\n"
                    "'@n-recursive-relation','','','RelName','','/path/file.dl [4:7-0:0]','','','','0',''\n"
                    "'@runtime','','0','','','','0','','0.00126608','',''\n";
        } DATA_WITH;
    } CSV;

    // function to write a string to a given file
    const auto writeFileFromString = [](const std::string& filePath, const std::string& fileContent) {
        std::ofstream logFileStream(filePath);
        logFileStream << fileContent;
        logFileStream.close();
    };

    // function to read a given file into a string
    const auto readFileToString = [](const std::string& filePath) {
        std::ifstream fileStream(filePath);
        std::stringstream fileContent;
        std::string line;
        while (std::getline(fileStream, line)) {
            fileContent << line << '\n';
        }
        fileStream.close();
        return fileContent.str();
    };

    // function to run a single test case for a particular config
    const auto runTestCase = [&](const std::string& logFilePath, const std::string& csvFilePath,
            const std::map<std::string, std::string>& config, const std::string& expected) {
        FileFormatConverter::fromLogToCsv(logFilePath, csvFilePath, config);
        const auto actual = readFileToString(csvFilePath);
        EXPECT_EQ(actual, expected);
    };

    // the unit test itself
    {
        // write the log file first
        writeFileFromString(LOG.FILE_PATH, LOG.DATA);

        // test for default config with no headers as first row and no quotes around columns
        runTestCase(LOG.FILE_PATH, CSV.FILE_PATH, CSV.CONFIG_WITH.DEFAULTS, CSV.DATA_WITH.DEFAULTS);

        // test for config with headers as first csv, but no quotes around columns
        runTestCase(LOG.FILE_PATH, CSV.FILE_PATH, CSV.CONFIG_WITH.HEADERS, CSV.DATA_WITH.HEADERS);

        // test for config with quotes around columns, but no headers as first row
        runTestCase(LOG.FILE_PATH, CSV.FILE_PATH, CSV.CONFIG_WITH.QUOTES, CSV.DATA_WITH.QUOTES);

        // test for config with headers as first row and quotes around columns
        runTestCase(LOG.FILE_PATH, CSV.FILE_PATH, CSV.CONFIG_WITH.HEADERS_AND_QUOTES,
                CSV.DATA_WITH.HEADERS_AND_QUOTES);

        remove(LOG.FILE_PATH.c_str());
        remove(CSV.FILE_PATH.c_str());
    }
}

}  // end namespace test
}  // end namespace souffle
