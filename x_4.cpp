#include "souffle/CompiledSouffle.h"

namespace souffle {
using namespace ram;
class Sf_x_4 : public SouffleProgram {
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
public:
Sf_x_4() : rel_1_A(new ram::Relation<Auto,1>()){
}
~Sf_x_4() {
delete rel_1_A;
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
/* BEGIN visitSequence @RamExecutor.cpp:1432 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1516 */
SignalHandler::instance()->setMsg(R"_(A(11).
in file /home/lyndonhenry/Dropbox/workspace/souffle/tests/semantic/bitwise/bitwise.dl [2:1-2:7])_");
/* BEGIN visitFact @RamExecutor.cpp:1235 */
rel_1_A->insert(/* BEGIN visitNumber @RamExecutor.cpp:1954 */
11/* END visitNumber @RamExecutor.cpp:1956 */
);
/* END visitFact @RamExecutor.cpp:1238 */
/* END visitDebugInfo @RamExecutor.cpp:1523 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1516 */
SignalHandler::instance()->setMsg(R"_(A(4711).
in file /home/lyndonhenry/Dropbox/workspace/souffle/tests/semantic/bitwise/bitwise.dl [3:1-3:9])_");
/* BEGIN visitFact @RamExecutor.cpp:1235 */
rel_1_A->insert(/* BEGIN visitNumber @RamExecutor.cpp:1954 */
4711/* END visitNumber @RamExecutor.cpp:1956 */
);
/* END visitFact @RamExecutor.cpp:1238 */
/* END visitDebugInfo @RamExecutor.cpp:1523 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1516 */
SignalHandler::instance()->setMsg(R"_(A(121233).
in file /home/lyndonhenry/Dropbox/workspace/souffle/tests/semantic/bitwise/bitwise.dl [4:1-4:11])_");
/* BEGIN visitFact @RamExecutor.cpp:1235 */
rel_1_A->insert(/* BEGIN visitNumber @RamExecutor.cpp:1954 */
121233/* END visitNumber @RamExecutor.cpp:1956 */
);
/* END visitFact @RamExecutor.cpp:1238 */
/* END visitDebugInfo @RamExecutor.cpp:1523 */
/* BEGIN visitDebugInfo @RamExecutor.cpp:1516 */
SignalHandler::instance()->setMsg(R"_(A(1234).
in file /home/lyndonhenry/Dropbox/workspace/souffle/tests/semantic/bitwise/bitwise.dl [5:1-5:9])_");
/* BEGIN visitFact @RamExecutor.cpp:1235 */
rel_1_A->insert(/* BEGIN visitNumber @RamExecutor.cpp:1954 */
1234/* END visitNumber @RamExecutor.cpp:1956 */
);
/* END visitFact @RamExecutor.cpp:1238 */
/* END visitDebugInfo @RamExecutor.cpp:1523 */
/* END visitSequence @RamExecutor.cpp:1436 */
/* BEGIN visitStore @RamExecutor.cpp:1265 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/semantic/bitwise/id/A.facts"},{"name","A"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitStore @RamExecutor.cpp:1283 */
/* BEGIN visitDrop @RamExecutor.cpp:1397 */
if (performIO || 0) rel_1_A->purge();
/* END visitDrop @RamExecutor.cpp:1401 */
/* END visitSequence @RamExecutor.cpp:1436 */

// -- relation hint statistics --
if(isHintsProfilingEnabled()) {
std::cout << " -- Operation Hint Statistics --\n";
std::cout << "Relation rel_1_A:\n";
rel_1_A->printHintStatistics(std::cout,"  ");
std::cout << "\n";
}
}
public:
void run() { runFunction<false>(); }
public:
void runAll(std::string inputDirectory = ".", std::string outputDirectory = ".") { runFunction<true>(inputDirectory, outputDirectory); }
public:
void printAll(std::string outputDirectory = ".") {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/semantic/bitwise/id/A.facts"},{"name","A"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
void loadAll(std::string inputDirectory = ".") {
}
public:
void dumpInputs(std::ostream& out = std::cout) {
}
public:
void dumpOutputs(std::ostream& out = std::cout) {
try {IODirectives ioDirectives;
ioDirectives.setIOType("stdout");
ioDirectives.setRelationName("rel_1_A");
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
const SymbolTable &getSymbolTable() const {
return symTable;
}
};
SouffleProgram *newInstance_x(){return new Sf_x_4;}
SymbolTable *getST_x(SouffleProgram *p){return &reinterpret_cast<Sf_x_4*>(p)->symTable;}
#ifdef __EMBEDDED_SOUFFLE__
class factory_Sf_x_4: public souffle::ProgramFactory {
SouffleProgram *newInstance() {
return new Sf_x_4();
};
public:
factory_Sf_x_4() : ProgramFactory("x"){}
};
static factory_Sf_x_4 __factory_Sf_x_4_instance;
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
souffle::Sf_x_4 obj;
obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
return 0;
} catch(std::exception &e) { souffle::SignalHandler::instance()->error(e.what());}
}
#endif
