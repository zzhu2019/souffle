#include "souffle/CompiledSouffle.h"

namespace souffle {
using namespace ram;
class Sf_x_2 : public SouffleProgram {
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
// -- Table: A
ram::Relation<Auto,1>* rel_1_A;
souffle::RelationWrapper<0,ram::Relation<Auto,1>,Tuple<RamDomain,1>,1,true,false> wrapper_rel_1_A;
// -- Table: r_bnot
ram::Relation<Auto,2>* rel_4_r_bnot;
souffle::RelationWrapper<1,ram::Relation<Auto,2>,Tuple<RamDomain,2>,2,false,true> wrapper_rel_4_r_bnot;
public:
Sf_x_2() : rel_1_A(new ram::Relation<Auto,1>()),
wrapper_rel_1_A(*rel_1_A,symTable,"A",std::array<const char *,1>{"i:number"},std::array<const char *,1>{"x"}),
rel_4_r_bnot(new ram::Relation<Auto,2>()),
wrapper_rel_4_r_bnot(*rel_4_r_bnot,symTable,"r_bnot",std::array<const char *,2>{"i:number","i:number"},std::array<const char *,2>{"x","y"}){
addRelation("A",&wrapper_rel_1_A,1,0);
addRelation("r_bnot",&wrapper_rel_4_r_bnot,0,1);
}
~Sf_x_2() {
delete rel_1_A;
delete rel_4_r_bnot;
}
private:
template <bool performIO> void runFunction(std::string inputDirectory = ".", std::string outputDirectory = ".") {
// -- initialize counter --
std::atomic<RamDomain> ctr(0);

#if defined(__EMBEDDED_SOUFFLE__) && defined(_OPENMP)
omp_set_num_threads(1);
#endif

// -- query evaluation --
/* BEGIN visitSequence @RamExecutor.cpp:1432 */
/* BEGIN visitCreate @RamExecutor.cpp:1230 */
/* END visitCreate @RamExecutor.cpp:1231 */
/* BEGIN visitLoad @RamExecutor.cpp:1242 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/semantic/bitwise/id/A.facts"},{"intermediate","true"},{"name","A"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({0}), symTable, ioDirectives, 0)->readAll(*rel_1_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitLoad @RamExecutor.cpp:1261 */
/* BEGIN visitCreate @RamExecutor.cpp:1230 */
/* END visitCreate @RamExecutor.cpp:1231 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1516 */
SignalHandler::instance()->setMsg(R"_(r_bnot(bnot(x),x) :- 
   A(x).
in file /home/lyndonhenry/Dropbox/workspace/souffle/tests/semantic/bitwise/bitwise.dl [21:1-21:27])_");
/* BEGIN visitInsert @RamExecutor.cpp:1287 */
if (!rel_1_A->empty()) {
auto part = rel_1_A->partition();
PARALLEL_START;
CREATE_OP_CONTEXT(rel_1_A_op_ctxt,rel_1_A->createContext());
CREATE_OP_CONTEXT(rel_4_r_bnot_op_ctxt,rel_4_r_bnot->createContext());
/* BEGIN visitScan @RamExecutor.cpp:1543 */
pfor(auto it = part.begin(); it<part.end(); ++it) 
try{for(const auto& env0 : *it) {
/* BEGIN visitSearch @RamExecutor.cpp:1529 */
/* BEGIN visitProject @RamExecutor.cpp:1783 */
Tuple<RamDomain,2> tuple({(RamDomain)(/* BEGIN visitUnaryOperator @RamExecutor.cpp:1972 */
(~(/* BEGIN visitElementAccess @RamExecutor.cpp:1960 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1962 */
))/* END visitUnaryOperator @RamExecutor.cpp:2035 */
),(RamDomain)(/* BEGIN visitElementAccess @RamExecutor.cpp:1960 */
env0[0]/* END visitElementAccess @RamExecutor.cpp:1962 */
)});
rel_4_r_bnot->insert(tuple,READ_OP_CONTEXT(rel_4_r_bnot_op_ctxt));
/* END visitProject @RamExecutor.cpp:1838 */
/* END visitSearch @RamExecutor.cpp:1539 */
}
} catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
PARALLEL_END;
}
/* END visitInsert @RamExecutor.cpp:1378 */
/* END visitDebugInfo @RamExecutor.cpp:1523 */
/* BEGIN visitDrop @RamExecutor.cpp:1397 */
if (performIO || 0) rel_1_A->purge();
/* END visitDrop @RamExecutor.cpp:1401 */
/* BEGIN visitStore @RamExecutor.cpp:1265 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/semantic/bitwise/id/r_bnot.csv"},{"name","r_bnot"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_4_r_bnot);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitStore @RamExecutor.cpp:1283 */
/* BEGIN visitDrop @RamExecutor.cpp:1397 */
if (performIO || 0) rel_4_r_bnot->purge();
/* END visitDrop @RamExecutor.cpp:1401 */
/* END visitSequence @RamExecutor.cpp:1436 */

// -- relation hint statistics --
if(isHintsProfilingEnabled()) {
std::cout << " -- Operation Hint Statistics --\n";
std::cout << "Relation rel_1_A:\n";
rel_1_A->printHintStatistics(std::cout,"  ");
std::cout << "\n";
std::cout << "Relation rel_4_r_bnot:\n";
rel_4_r_bnot->printHintStatistics(std::cout,"  ");
std::cout << "\n";
}
}
public:
void run() { runFunction<false>(); }
public:
void runAll(std::string inputDirectory = ".", std::string outputDirectory = ".") { runFunction<true>(inputDirectory, outputDirectory); }
public:
void printAll(std::string outputDirectory = ".") {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/semantic/bitwise/id/r_bnot.csv"},{"name","r_bnot"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_4_r_bnot);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void loadAll(std::string inputDirectory = ".") {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"delimiter","\t"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/semantic/bitwise/id/A.facts"},{"intermediate","true"},{"name","A"}});
if (!inputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = inputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getReader(SymbolMask({0}), symTable, ioDirectives, 0)->readAll(*rel_1_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void dumpInputs(std::ostream& out = std::cout) {
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_1_A");
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void dumpOutputs(std::ostream& out = std::cout) {
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_4_r_bnot");
IOSystem::getInstance().getWriter(SymbolMask({0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_4_r_bnot);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
const SymbolTable &getSymbolTable() const {
return symTable;
}
};
SouffleProgram *newInstance_x(){return new Sf_x_2;}
SymbolTable *getST_x(SouffleProgram *p){return &reinterpret_cast<Sf_x_2*>(p)->symTable;}
#ifdef __EMBEDDED_SOUFFLE__
class factory_Sf_x_2: public souffle::ProgramFactory {
SouffleProgram *newInstance() {
return new Sf_x_2();
};
public:
factory_Sf_x_2() : ProgramFactory("x"){}
};
static factory_Sf_x_2 __factory_Sf_x_2_instance;
}
#else
}
int main(int argc, char** argv)
{
try{
souffle::CmdOptions opt(R"(/home/lyndonhenry/Dropbox/workspace/souffle/tests/semantic/bitwise/bitwise.dl)",
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
souffle::Sf_x_2 obj;
obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
return 0;
} catch(std::exception &e) { souffle::SignalHandler::instance()->error(e.what());}
}
#endif
