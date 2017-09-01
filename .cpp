#include "souffle/CompiledSouffle.h"

namespace souffle {
using namespace ram;
class Sf__ : public SouffleProgram {
private:
static inline bool regex_wrapper(const char *pattern, const char *text) {
   bool result = false; 
   try { result = std::regex_match(text, std::regex(pattern)); } catch(...) { 
     std::cerr << "warning: wrong pattern provided for match(\"" << pattern << "\",\"" << text << "\")\n";
}
   return result;
}
static inline std::string substr_wrapper(const char *str, size_t idx, size_t len) {
   std::string sub_str, result; 
   try { result = std::string(str).substr(idx,len); } catch(...) { 
     std::cerr << "warning: wrong index position provided by substr(\"";
     std::cerr << str << "\"," << idx << "," << len << ") functor.\n";
   } return result;
}
public:
SymbolTable symTable;
// -- Table: SymbolB
ram::Relation<Auto,2, ram::index<0,1>>* rel_5_SymbolB;
souffle::RelationWrapper<0,ram::Relation<Auto,2, ram::index<0,1>>,Tuple<RamDomain,2>,2,true,false> wrapper_rel_5_SymbolB;
// -- Table: SymbolC
ram::Relation<Auto,2, ram::index<0,1>>* rel_6_SymbolC;
souffle::RelationWrapper<1,ram::Relation<Auto,2, ram::index<0,1>>,Tuple<RamDomain,2>,2,true,false> wrapper_rel_6_SymbolC;
// -- Table: SymbolD
ram::Relation<Auto,2, ram::index<0,1>>* rel_7_SymbolD;
souffle::RelationWrapper<2,ram::Relation<Auto,2, ram::index<0,1>>,Tuple<RamDomain,2>,2,true,false> wrapper_rel_7_SymbolD;
// -- Table: SymbolA
ram::Relation<Auto,2>* rel_8_SymbolA;
souffle::RelationWrapper<3,ram::Relation<Auto,2>,Tuple<RamDomain,2>,2,false,true> wrapper_rel_8_SymbolA;
public:
Sf__() : rel_5_SymbolB(new ram::Relation<Auto,2, ram::index<0,1>>()),
wrapper_rel_5_SymbolB(*rel_5_SymbolB,symTable,"SymbolB",std::array<const char *,2>{"s:symbol","s:symbol"},std::array<const char *,2>{"x","y"}),
rel_6_SymbolC(new ram::Relation<Auto,2, ram::index<0,1>>()),
wrapper_rel_6_SymbolC(*rel_6_SymbolC,symTable,"SymbolC",std::array<const char *,2>{"s:symbol","s:symbol"},std::array<const char *,2>{"x","y"}),
rel_7_SymbolD(new ram::Relation<Auto,2, ram::index<0,1>>()),
wrapper_rel_7_SymbolD(*rel_7_SymbolD,symTable,"SymbolD",std::array<const char *,2>{"s:symbol","s:symbol"},std::array<const char *,2>{"x","y"}),
rel_8_SymbolA(new ram::Relation<Auto,2>()),
wrapper_rel_8_SymbolA(*rel_8_SymbolA,symTable,"SymbolA",std::array<const char *,2>{"s:symbol","s:symbol"},std::array<const char *,2>{"x","y"}){
addRelation("SymbolB",&wrapper_rel_5_SymbolB,1,0);
addRelation("SymbolC",&wrapper_rel_6_SymbolC,1,0);
addRelation("SymbolD",&wrapper_rel_7_SymbolD,1,0);
addRelation("SymbolA",&wrapper_rel_8_SymbolA,0,1);
// -- initialize symbol table --
static const char *symbols[]={
	R"(0)",
	R"(1)",
	R"(2)",
};
symTable.insert(symbols,3);

}
~Sf__() {
delete rel_5_SymbolB;
delete rel_6_SymbolC;
delete rel_7_SymbolD;
delete rel_8_SymbolA;
}
private:
template <bool performIO> void runFunction(std::string inputDirectory = ".", std::string outputDirectory = ".") {
// -- initialize counter --
std::atomic<RamDomain> ctr(0);

#if defined(__EMBEDDED_SOUFFLE__) && defined(_OPENMP)
omp_set_num_threads(1);
#endif

// -- query evaluation --
/* BEGIN visitSequence @RamExecutor.cpp:1430 */
/* BEGIN visitCreate @RamExecutor.cpp:1228 */
/* END visitCreate @RamExecutor.cpp:1229 */
/* BEGIN visitLoad @RamExecutor.cpp:1240 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolB.facts"},{"intermediate","true"},{"name","SymbolB"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({1, 1}), symTable, ioDirectives, 0)->readAll(*rel_5_SymbolB);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitLoad @RamExecutor.cpp:1259 */
/* BEGIN visitCreate @RamExecutor.cpp:1228 */
/* END visitCreate @RamExecutor.cpp:1229 */
/* BEGIN visitLoad @RamExecutor.cpp:1240 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolC.facts"},{"intermediate","true"},{"name","SymbolC"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({1, 1}), symTable, ioDirectives, 0)->readAll(*rel_6_SymbolC);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitLoad @RamExecutor.cpp:1259 */
/* BEGIN visitCreate @RamExecutor.cpp:1228 */
/* END visitCreate @RamExecutor.cpp:1229 */
/* BEGIN visitLoad @RamExecutor.cpp:1240 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolD.facts"},{"intermediate","true"},{"name","SymbolD"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({1, 1}), symTable, ioDirectives, 0)->readAll(*rel_7_SymbolD);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitLoad @RamExecutor.cpp:1259 */
/* BEGIN visitCreate @RamExecutor.cpp:1228 */
/* END visitCreate @RamExecutor.cpp:1229 */
/* BEGIN visitSequence @RamExecutor.cpp:1430 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,x) :- 
   SymbolB(x,x).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [70:1-70:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
if( /* BEGIN visitBinaryRelation @RamExecutor.cpp:1848 */
((/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
) == (/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
))/* END visitBinaryRelation @RamExecutor.cpp:1907 */
) {
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
}
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,x) :- 
   SymbolC(x,x).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [71:1-71:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
if( /* BEGIN visitBinaryRelation @RamExecutor.cpp:1848 */
((/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
) == (/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
))/* END visitBinaryRelation @RamExecutor.cpp:1907 */
) {
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
}
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,x) :- 
   SymbolD(x,x).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [72:1-72:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
if( /* BEGIN visitBinaryRelation @RamExecutor.cpp:1848 */
((/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
) == (/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
))/* END visitBinaryRelation @RamExecutor.cpp:1907 */
) {
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
}
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolB(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [74:1-74:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolC(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [75:1-75:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolD(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [76:1-76:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolB(x,y),
   SymbolC(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [78:1-78:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_6_SymbolC->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
,/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
});
auto range = rel_6_SymbolC->equalRange<0,1>(key,READ_OP_CONTEXT(rel_6_SymbolC_op_ctxt));
if(!range.empty()) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolC(x,y),
   SymbolD(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [79:1-79:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
,/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
});
auto range = rel_7_SymbolD->equalRange<0,1>(key,READ_OP_CONTEXT(rel_7_SymbolD_op_ctxt));
if(!range.empty()) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolD(x,y),
   SymbolB(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [80:1-80:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
,/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
});
auto range = rel_5_SymbolB->equalRange<0,1>(key,READ_OP_CONTEXT(rel_5_SymbolB_op_ctxt));
if(!range.empty()) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolB(x,y),
   SymbolC(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [82:1-82:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_6_SymbolC->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_6_SymbolC->equalRange<0>(key,READ_OP_CONTEXT(rel_6_SymbolC_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolC(x,y),
   SymbolD(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [83:1-83:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_7_SymbolD->equalRange<0>(key,READ_OP_CONTEXT(rel_7_SymbolD_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolD(x,y),
   SymbolB(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [84:1-84:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_5_SymbolB->equalRange<0>(key,READ_OP_CONTEXT(rel_5_SymbolB_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolC(x,y),
   SymbolB(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [86:1-86:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_6_SymbolC->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_5_SymbolB->equalRange<0>(key,READ_OP_CONTEXT(rel_5_SymbolB_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolD(x,y),
   SymbolC(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [87:1-87:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_6_SymbolC->equalRange<0>(key,READ_OP_CONTEXT(rel_6_SymbolC_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolB(x,y),
   SymbolD(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [88:1-88:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_7_SymbolD->equalRange<0>(key,READ_OP_CONTEXT(rel_7_SymbolD_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,x) :- 
   SymbolB(x,x).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [92:1-92:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
if( /* BEGIN visitBinaryRelation @RamExecutor.cpp:1848 */
((/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
) == (/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
))/* END visitBinaryRelation @RamExecutor.cpp:1907 */
) {
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
}
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,x) :- 
   SymbolC(x,x).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [93:1-93:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
if( /* BEGIN visitBinaryRelation @RamExecutor.cpp:1848 */
((/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
) == (/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
))/* END visitBinaryRelation @RamExecutor.cpp:1907 */
) {
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
}
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,x) :- 
   SymbolD(x,x).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [94:1-94:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
if( /* BEGIN visitBinaryRelation @RamExecutor.cpp:1848 */
((/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
) == (/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
))/* END visitBinaryRelation @RamExecutor.cpp:1907 */
) {
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
}
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolB(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [96:1-96:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolC(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [97:1-97:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolD(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [98:1-98:32])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolB(x,y),
   SymbolC(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [100:1-100:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_6_SymbolC->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
,/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
});
auto range = rel_6_SymbolC->equalRange<0,1>(key,READ_OP_CONTEXT(rel_6_SymbolC_op_ctxt));
if(!range.empty()) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolC(x,y),
   SymbolD(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [101:1-101:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
,/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
});
auto range = rel_7_SymbolD->equalRange<0,1>(key,READ_OP_CONTEXT(rel_7_SymbolD_op_ctxt));
if(!range.empty()) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,y) :- 
   SymbolD(x,y),
   SymbolB(x,y).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [102:1-102:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
,/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
});
auto range = rel_5_SymbolB->equalRange<0,1>(key,READ_OP_CONTEXT(rel_5_SymbolB_op_ctxt));
if(!range.empty()) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolB(x,y),
   SymbolC(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [104:1-104:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_6_SymbolC->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_6_SymbolC->equalRange<0>(key,READ_OP_CONTEXT(rel_6_SymbolC_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolC(x,y),
   SymbolD(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [105:1-105:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_7_SymbolD->equalRange<0>(key,READ_OP_CONTEXT(rel_7_SymbolD_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolD(x,y),
   SymbolB(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [106:1-106:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_5_SymbolB->equalRange<0>(key,READ_OP_CONTEXT(rel_5_SymbolB_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolC(x,y),
   SymbolB(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [108:1-108:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_6_SymbolC->empty()) {
auto part = rel_6_SymbolC->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_5_SymbolB->equalRange<0>(key,READ_OP_CONTEXT(rel_5_SymbolB_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolD(x,y),
   SymbolC(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [109:1-109:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_6_SymbolC->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_7_SymbolD->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_6_SymbolC_op_ctxt,rel_6_SymbolC->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_6_SymbolC->equalRange<0>(key,READ_OP_CONTEXT(rel_6_SymbolC_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1514 */
SignalHandler::instance()->setMsg(R"_(SymbolA(x,z) :- 
   SymbolB(x,y),
   SymbolD(y,z).
in file /home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl [110:1-110:47])_");
/* BEGIN visitInsert @RamExecutor.cpp:1285 */
if (!rel_5_SymbolB->empty()&&!rel_7_SymbolD->empty()) {
auto part = rel_5_SymbolB->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_8_SymbolA_op_ctxt,rel_8_SymbolA->createContext());
CREATE_OP_CONTEXT(rel_5_SymbolB_op_ctxt,rel_5_SymbolB->createContext());
CREATE_OP_CONTEXT(rel_7_SymbolD_op_ctxt,rel_7_SymbolD->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1541 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitScan @RamExecutor.cpp:1541 */
const Tuple<RamDomain,2> key({/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
,0});
auto range = rel_7_SymbolD->equalRange<0>(key,READ_OP_CONTEXT(rel_7_SymbolD_op_ctxt));
for(const auto& env1 : range) {
/* BEGIN visitSearch @RamExecutor.cpp:1527 */
/* BEGIN visitProject @RamExecutor.cpp:1781 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1960 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1958 */
env1[1]/* END visitElementAccess @RamExecutor.cpp:1960 */
)});
rel_8_SymbolA->insert(tuple,READ_OP_CONTEXT(rel_8_SymbolA_op_ctxt));
/* END visitProject @RamExecutor.cpp:1836 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
/* END visitScan @RamExecutor.cpp:1611 */
/* END visitSearch @RamExecutor.cpp:1537 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1376 */
/* END visitDebugInfo @RamExecutor.cpp:1521 */
/* END visitSequence @RamExecutor.cpp:1434 */
/* BEGIN visitDrop @RamExecutor.cpp:1395 */
if (performIO || 0) rel_5_SymbolB->purge();
/* END visitDrop @RamExecutor.cpp:1399 */
/* BEGIN visitDrop @RamExecutor.cpp:1395 */
if (performIO || 0) rel_6_SymbolC->purge();
/* END visitDrop @RamExecutor.cpp:1399 */
/* BEGIN visitDrop @RamExecutor.cpp:1395 */
if (performIO || 0) rel_7_SymbolD->purge();
/* END visitDrop @RamExecutor.cpp:1399 */
/* BEGIN visitStore @RamExecutor.cpp:1263 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolA.csv"},{"name","SymbolA"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({1, 1}), symTable, ioDirectives, 0)->writeAll(*rel_8_SymbolA);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitStore @RamExecutor.cpp:1281 */
/* BEGIN visitDrop @RamExecutor.cpp:1395 */
if (performIO || 0) rel_8_SymbolA->purge();
/* END visitDrop @RamExecutor.cpp:1399 */
/* END visitSequence @RamExecutor.cpp:1434 */
}
public:
void run() { runFunction<false>(); }
public:
void runAll(std::string inputDirectory = ".", std::string outputDirectory = ".") { runFunction<true>(inputDirectory, outputDirectory); }
public:
void printAll(std::string outputDirectory = ".") {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolA.csv"},{"name","SymbolA"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({1, 1}), symTable, ioDirectives, 0)->writeAll(*rel_8_SymbolA);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void loadAll(std::string inputDirectory = ".") {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolB.facts"},{"intermediate","true"},{"name","SymbolB"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({1, 1}), symTable, ioDirectives, 0)->readAll(*rel_5_SymbolB);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolC.facts"},{"intermediate","true"},{"name","SymbolC"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({1, 1}), symTable, ioDirectives, 0)->readAll(*rel_6_SymbolC);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/workspace/souffle/tests/testsuite.dir/syntactic/duplicates/id/SymbolD.facts"},{"intermediate","true"},{"name","SymbolD"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({1, 1}), symTable, ioDirectives, 0)->readAll(*rel_7_SymbolD);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void dumpInputs(std::ostream& out = std::cout) {
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_5_SymbolB");
IOSystem::getInstance().getWriter(SymbolMask({1, 1}), symTable, ioDirectives, 0)->writeAll(*rel_5_SymbolB);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_6_SymbolC");
IOSystem::getInstance().getWriter(SymbolMask({1, 1}), symTable, ioDirectives, 0)->writeAll(*rel_6_SymbolC);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_7_SymbolD");
IOSystem::getInstance().getWriter(SymbolMask({1, 1}), symTable, ioDirectives, 0)->writeAll(*rel_7_SymbolD);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void dumpOutputs(std::ostream& out = std::cout) {
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_8_SymbolA");
IOSystem::getInstance().getWriter(SymbolMask({1, 1}), symTable, ioDirectives, 0)->writeAll(*rel_8_SymbolA);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
const SymbolTable &getSymbolTable() const {
return symTable;
}
};
SouffleProgram *newInstance__(){return new Sf__;}
SymbolTable *getST__(SouffleProgram *p){return &reinterpret_cast<Sf__*>(p)->symTable;}
#ifdef __EMBEDDED_SOUFFLE__
class factory_Sf__: public souffle::ProgramFactory {
SouffleProgram *newInstance() {
return new Sf__();
};
public:
factory_Sf__() : ProgramFactory("_"){}
};
static factory_Sf__ __factory_Sf___instance;
}
#else
}
int main(int argc, char** argv)
{
try{
souffle::CmdOptions opt(R"(/home/lyndonhenry/workspace/souffle/tests/syntactic/duplicates/duplicates.dl)",
R"(.)",
R"(.)",
false,
R"()",
1
);
if (!opt.parse(argc,argv)) return 1;
#if defined(_OPENMP) 
omp_set_nested(true);
#endif
souffle::Sf__ obj;
obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
return 0;
} catch(std::exception &e) { souffle::SignalHandler::instance()->error(e.what());}
}
#endif
