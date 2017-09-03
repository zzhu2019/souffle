#include "souffle/CompiledSouffle.h"

namespace souffle {
using namespace ram;
class Sf_comment2_1 : public SouffleProgram {
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
ram::Relation<Auto,1>* rel_2_A;
souffle::RelationWrapper<0,ram::Relation<Auto,1>,Tuple<RamDomain,1>,1,false,true> wrapper_rel_2_A;
public:
Sf_comment2_1() : rel_2_A(new ram::Relation<Auto,1>()),
wrapper_rel_2_A(*rel_2_A,symTable,"A",std::array<const char *,1>{"r:myrecord{x#i:number,y#r:myrecord}"},std::array<const char *,1>{"x"}){
addRelation("A",&wrapper_rel_2_A,0,1);
}
~Sf_comment2_1() {
delete rel_2_A;
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
/* BEGIN visitStore @RamExecutor.cpp:1265 */
if (performIO) {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/syntactic/comment2/id/A.csv"},{"name","A"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_2_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
/* END visitStore @RamExecutor.cpp:1283 */
/* BEGIN visitDrop @RamExecutor.cpp:1397 */
if (performIO || 0) rel_2_A->purge();
/* END visitDrop @RamExecutor.cpp:1401 */
/* END visitSequence @RamExecutor.cpp:1436 */

// -- relation hint statistics --
if(isHintsProfilingEnabled()) {
std::cout << " -- Operation Hint Statistics --\n";
std::cout << "Relation rel_2_A:\n";
rel_2_A->printHintStatistics(std::cout,"  ");
std::cout << "\n";
}
}
public:
void run() { runFunction<false>(); }
public:
void runAll(std::string inputDirectory = ".", std::string outputDirectory = ".") { runFunction<true>(inputDirectory, outputDirectory); }
public:
void printAll(std::string outputDirectory = ".") {
try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"filename","/home/lyndonhenry/Dropbox/workspace/souffle/tests/testsuite.dir/syntactic/comment2/id/A.csv"},{"name","A"}});
if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
IODirectives ioDirectives(directiveMap);
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_2_A);
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
ioDirectives.setRelationName("rel_2_A");
IOSystem::getInstance().getWriter(SymbolMask({0}), symTable, ioDirectives, 0)->writeAll(*rel_2_A);
} catch (std::exception& e) {std::cerr << e.what();exit(1);}
}
public:
const SymbolTable &getSymbolTable() const {
return symTable;
}
};
SouffleProgram *newInstance_comment2(){return new Sf_comment2_1;}
SymbolTable *getST_comment2(SouffleProgram *p){return &reinterpret_cast<Sf_comment2_1*>(p)->symTable;}
#ifdef __EMBEDDED_SOUFFLE__
class factory_Sf_comment2_1: public souffle::ProgramFactory {
SouffleProgram *newInstance() {
return new Sf_comment2_1();
};
public:
factory_Sf_comment2_1() : ProgramFactory("comment2"){}
};
static factory_Sf_comment2_1 __factory_Sf_comment2_1_instance;
}
#else
}
int main(int argc, char** argv)
{
try{
souffle::CmdOptions opt(R"(/home/lyndonhenry/Dropbox/workspace/souffle/tests/syntactic/comment2/comment2.dl)",
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
souffle::Sf_comment2_1 obj;
obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
return 0;
} catch(std::exception &e) { souffle::SignalHandler::instance()->error(e.what());}
}
#endif
