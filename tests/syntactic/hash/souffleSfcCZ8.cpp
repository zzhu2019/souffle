#include "souffle/CompiledSouffle.h"

namespace souffle {
        using namespace ram;
        class Sf___souffleSfcCZ8 : public SouffleProgram {
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
                                        std::cerr << str << "\"," << (int32_t)idx << "," << (int32_t)len << ") functor.\n";
                                } return result;
                        }
                public:
                        SymbolTable symTable;
                        // -- Table: A
                        ram::Relation<Rbtset,3, ram::index<2,0,1>>* rel_1_A;
                        souffle::RelationWrapper<0,ram::Relation<Rbtset,3, ram::index<2,0,1>>,Tuple<RamDomain,3>,3,false,true> wrapper_rel_1_A;
                        // -- Table: @delta_A
                        ram::Relation<Rbtset,3, ram::index<2,0,1>>* rel_2_delta_A;
                        // -- Table: @new_A
                        ram::Relation<Rbtset,3, ram::index<2,0,1>>* rel_3_new_A;
                        // -- Table: B
                        ram::Relation<Hashset,3, ram::index<2>, ram::index<0,1,2>>* rel_4_B;
                        souffle::RelationWrapper<1,ram::Relation<Hashset,3, ram::index<2>, ram::index<0,1,2>>,Tuple<RamDomain,3>,3,false,true> wrapper_rel_4_B;
                        // -- Table: @delta_B
                        ram::Relation<Hashset,3, ram::index<2,0,1>>* rel_5_delta_B;
                        // -- Table: @new_B
                        ram::Relation<Hashset,3, ram::index<2,0,1>>* rel_6_new_B;
                public:
                        Sf___souffleSfcCZ8() : rel_1_A(new ram::Relation<Rbtset,3, ram::index<2,0,1>>()),
                        wrapper_rel_1_A(*rel_1_A,symTable,"A",std::array<const char *,3>{"i:number","i:number","i:number"},std::array<const char *,3>{"x","y","z"}),
                        rel_2_delta_A(new ram::Relation<Rbtset,3, ram::index<2,0,1>>()),
                        rel_3_new_A(new ram::Relation<Rbtset,3, ram::index<2,0,1>>()),
                        rel_4_B(new ram::Relation<Hashset,3, ram::index<2>, ram::index<0,1,2>>()),
                        wrapper_rel_4_B(*rel_4_B,symTable,"B",std::array<const char *,3>{"i:number","i:number","i:number"},std::array<const char *,3>{"x","y","z"}),
                        rel_5_delta_B(new ram::Relation<Hashset,3, ram::index<2,0,1>>()),
                        rel_6_new_B(new ram::Relation<Hashset,3, ram::index<2,0,1>>()){
                                addRelation("A",&wrapper_rel_1_A,0,1);
                                addRelation("B",&wrapper_rel_4_B,0,1);
                        }
                        ~Sf___souffleSfcCZ8() {
                                delete rel_1_A;
                                delete rel_2_delta_A;
                                delete rel_3_new_A;
                                delete rel_4_B;
                                delete rel_5_delta_B;
                                delete rel_6_new_B;
                        }
                private:
                        template <bool performIO> void runFunction(std::string inputDirectory = ".", std::string outputDirectory = ".") {
                                SignalHandler::instance()->set();
                                // -- initialize counter --
                                std::atomic<RamDomain> ctr(0);

#if defined(__EMBEDDED_SOUFFLE__) && defined(_OPENMP)
                                omp_set_num_threads(1);
#endif

                                // -- query evaluation --
                                SignalHandler::instance()->setMsg(R"_(A(1,1,1).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [10:1-10:10])_");
                                        rel_1_A->insert(RamDomain(1),RamDomain(1),RamDomain(1));
                                SignalHandler::instance()->setMsg(R"_(A(1,1,2).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [11:1-11:10])_");
                                        rel_1_A->insert(RamDomain(1),RamDomain(1),RamDomain(2));
                                rel_2_delta_A->insertAll(*rel_1_A);
                                for(;;) {
                                        SignalHandler::instance()->setMsg(R"_(A(((x+z)+a),((y-z)+b),(z+1)) :- 
   A(x,y,z),
   A(a,b, _tmp_0),
   z < 10,
    _tmp_0 = (z-1).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [12:1-12:52])_");
                                                if (!rel_2_delta_A->empty()&&!rel_1_A->empty()) {
                                                        auto part = rel_2_delta_A->partition();
                                                        PARALLEL_START;
                                                        CREATE_OP_CONTEXT(rel_2_delta_A_op_ctxt,rel_2_delta_A->createContext());
                                                        CREATE_OP_CONTEXT(rel_3_new_A_op_ctxt,rel_3_new_A->createContext());
                                                        CREATE_OP_CONTEXT(rel_1_A_op_ctxt,rel_1_A->createContext());
                                                        pfor(auto it = part.begin(); it<part.end(); ++it) 
                                                                try{for(const auto& env0 : *it) {
                                                                        if( ((env0[2]) < (RamDomain(10)))) {
                                                                                const Tuple<RamDomain,3> key({0,0,(env0[2]) - (RamDomain(1))});
                                                                                auto range = rel_1_A->equalRange<2>(key,READ_OP_CONTEXT(rel_1_A_op_ctxt));
                                                                                for(const auto& env1 : range) {
                                                                                        if( ((!rel_1_A->contains(Tuple<RamDomain,3>({((env0[0]) + (env0[2])) + (env1[0]),((env0[1]) - (env0[2])) + (env1[1]),(env0[2]) + (RamDomain(1))}),READ_OP_CONTEXT(rel_1_A_op_ctxt))) && (!rel_2_delta_A->contains(Tuple<RamDomain,3>({env1[0],env1[1],env1[2]}),READ_OP_CONTEXT(rel_2_delta_A_op_ctxt))))) {
                                                                                                Tuple<RamDomain,3> tuple({(RamDomain)(((env0[0]) + (env0[2])) + (env1[0])),(RamDomain)(((env0[1]) - (env0[2])) + (env1[1])),(RamDomain)((env0[2]) + (RamDomain(1)))});
                                                                                                rel_3_new_A->insert(tuple,READ_OP_CONTEXT(rel_3_new_A_op_ctxt));
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                                } catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
                                                        PARALLEL_END;
                                                }
                                        SignalHandler::instance()->setMsg(R"_(A(((x+z)+a),((y-z)+b),(z+1)) :- 
   A(x,y,z),
   A(a,b, _tmp_0),
   z < 10,
    _tmp_0 = (z-1).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [12:1-12:52])_");
                                                if (!rel_2_delta_A->empty()&&!rel_1_A->empty()) {
                                                        auto part = rel_1_A->partition();
                                                        PARALLEL_START;
                                                        CREATE_OP_CONTEXT(rel_2_delta_A_op_ctxt,rel_2_delta_A->createContext());
                                                        CREATE_OP_CONTEXT(rel_3_new_A_op_ctxt,rel_3_new_A->createContext());
                                                        CREATE_OP_CONTEXT(rel_1_A_op_ctxt,rel_1_A->createContext());
                                                        pfor(auto it = part.begin(); it<part.end(); ++it) 
                                                                try{for(const auto& env0 : *it) {
                                                                        if( ((env0[2]) < (RamDomain(10)))) {
                                                                                const Tuple<RamDomain,3> key({0,0,(env0[2]) - (RamDomain(1))});
                                                                                auto range = rel_2_delta_A->equalRange<2>(key,READ_OP_CONTEXT(rel_2_delta_A_op_ctxt));
                                                                                for(const auto& env1 : range) {
                                                                                        if( !rel_1_A->contains(Tuple<RamDomain,3>({((env0[0]) + (env0[2])) + (env1[0]),((env0[1]) - (env0[2])) + (env1[1]),(env0[2]) + (RamDomain(1))}),READ_OP_CONTEXT(rel_1_A_op_ctxt))) {
                                                                                                Tuple<RamDomain,3> tuple({(RamDomain)(((env0[0]) + (env0[2])) + (env1[0])),(RamDomain)(((env0[1]) - (env0[2])) + (env1[1])),(RamDomain)((env0[2]) + (RamDomain(1)))});
                                                                                                rel_3_new_A->insert(tuple,READ_OP_CONTEXT(rel_3_new_A_op_ctxt));
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                                } catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
                                                        PARALLEL_END;
                                                }
                                        if(rel_3_new_A->empty()) break;
                                        rel_1_A->insertAll(*rel_3_new_A);
                                        {
                                                auto rel_0 = rel_2_delta_A;
                                                rel_2_delta_A = rel_3_new_A;
                                                rel_3_new_A = rel_0;
                                        }
                                        rel_3_new_A->purge();
                                }
                                if (!isHintsProfilingEnabled() && (performIO || 1)) rel_2_delta_A->purge();
                                if (!isHintsProfilingEnabled() && (performIO || 1)) rel_3_new_A->purge();
                                if (performIO) {
                                        try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"attributeNames","x\ty\tz"},{"filename","./A.csv"},{"name","A"}});
                                                if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
                                                IODirectives ioDirectives(directiveMap);
                                                IOSystem::getInstance().getWriter(SymbolMask({0, 0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
                                        } catch (std::exception& e) {std::cerr << e.what();exit(1);}
                                }
                                SignalHandler::instance()->setMsg(R"_(B(1,1,1).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [14:1-14:10])_");
                                        rel_4_B->insert(RamDomain(1),RamDomain(1),RamDomain(1));
                                SignalHandler::instance()->setMsg(R"_(B(1,1,2).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [15:1-15:10])_");
                                        rel_4_B->insert(RamDomain(1),RamDomain(1),RamDomain(2));
                                rel_5_delta_B->insertAll(*rel_4_B);
                                for(;;) {
                                        SignalHandler::instance()->setMsg(R"_(B(((x+z)+a),((y-z)+b),(z+1)) :- 
   B(x,y,z),
   B(a,b, _tmp_0),
   z < 10,
    _tmp_0 = (z-1).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [16:1-16:52])_");
                                                if (!rel_5_delta_B->empty()&&!rel_4_B->empty()) {
                                                        auto part = rel_5_delta_B->partition();
                                                        PARALLEL_START;
                                                        CREATE_OP_CONTEXT(rel_5_delta_B_op_ctxt,rel_5_delta_B->createContext());
                                                        CREATE_OP_CONTEXT(rel_6_new_B_op_ctxt,rel_6_new_B->createContext());
                                                        CREATE_OP_CONTEXT(rel_4_B_op_ctxt,rel_4_B->createContext());
                                                        pfor(auto it = part.begin(); it<part.end(); ++it) 
                                                                try{for(const auto& env0 : *it) {
                                                                        if( ((env0[2]) < (RamDomain(10)))) {
                                                                                const Tuple<RamDomain,3> key({0,0,(env0[2]) - (RamDomain(1))});
                                                                                auto range = rel_4_B->equalRange<2>(key,READ_OP_CONTEXT(rel_4_B_op_ctxt));
                                                                                for(const auto& env1 : range) {
                                                                                        if( ((!rel_4_B->contains(Tuple<RamDomain,3>({((env0[0]) + (env0[2])) + (env1[0]),((env0[1]) - (env0[2])) + (env1[1]),(env0[2]) + (RamDomain(1))}),READ_OP_CONTEXT(rel_4_B_op_ctxt))) && (!rel_5_delta_B->contains(Tuple<RamDomain,3>({env1[0],env1[1],env1[2]}),READ_OP_CONTEXT(rel_5_delta_B_op_ctxt))))) {
                                                                                                Tuple<RamDomain,3> tuple({(RamDomain)(((env0[0]) + (env0[2])) + (env1[0])),(RamDomain)(((env0[1]) - (env0[2])) + (env1[1])),(RamDomain)((env0[2]) + (RamDomain(1)))});
                                                                                                rel_6_new_B->insert(tuple,READ_OP_CONTEXT(rel_6_new_B_op_ctxt));
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                                } catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
                                                        PARALLEL_END;
                                                }
                                        SignalHandler::instance()->setMsg(R"_(B(((x+z)+a),((y-z)+b),(z+1)) :- 
   B(x,y,z),
   B(a,b, _tmp_0),
   z < 10,
    _tmp_0 = (z-1).
in file /home/jonathan/souffle-fork/souffle/tests/syntactic/hash/hash.dl [16:1-16:52])_");
                                                if (!rel_5_delta_B->empty()&&!rel_4_B->empty()) {
                                                        auto part = rel_4_B->partition();
                                                        PARALLEL_START;
                                                        CREATE_OP_CONTEXT(rel_5_delta_B_op_ctxt,rel_5_delta_B->createContext());
                                                        CREATE_OP_CONTEXT(rel_6_new_B_op_ctxt,rel_6_new_B->createContext());
                                                        CREATE_OP_CONTEXT(rel_4_B_op_ctxt,rel_4_B->createContext());
                                                        pfor(auto it = part.begin(); it<part.end(); ++it) 
                                                                try{for(const auto& env0 : *it) {
                                                                        if( ((env0[2]) < (RamDomain(10)))) {
                                                                                const Tuple<RamDomain,3> key({0,0,(env0[2]) - (RamDomain(1))});
                                                                                auto range = rel_5_delta_B->equalRange<2>(key,READ_OP_CONTEXT(rel_5_delta_B_op_ctxt));
                                                                                for(const auto& env1 : range) {
                                                                                        if( !rel_4_B->contains(Tuple<RamDomain,3>({((env0[0]) + (env0[2])) + (env1[0]),((env0[1]) - (env0[2])) + (env1[1]),(env0[2]) + (RamDomain(1))}),READ_OP_CONTEXT(rel_4_B_op_ctxt))) {
                                                                                                Tuple<RamDomain,3> tuple({(RamDomain)(((env0[0]) + (env0[2])) + (env1[0])),(RamDomain)(((env0[1]) - (env0[2])) + (env1[1])),(RamDomain)((env0[2]) + (RamDomain(1)))});
                                                                                                rel_6_new_B->insert(tuple,READ_OP_CONTEXT(rel_6_new_B_op_ctxt));
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                                } catch(std::exception &e) { SignalHandler::instance()->error(e.what());}
                                                        PARALLEL_END;
                                                }
                                        if(rel_6_new_B->empty()) break;
                                        rel_4_B->insertAll(*rel_6_new_B);
                                        {
                                                auto rel_0 = rel_5_delta_B;
                                                rel_5_delta_B = rel_6_new_B;
                                                rel_6_new_B = rel_0;
                                        }
                                        rel_6_new_B->purge();
                                }
                                if (!isHintsProfilingEnabled() && (performIO || 1)) rel_5_delta_B->purge();
                                if (!isHintsProfilingEnabled() && (performIO || 1)) rel_6_new_B->purge();
                                if (performIO) {
                                        try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"attributeNames","x\ty\tz"},{"filename","./B.csv"},{"name","B"}});
                                                if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
                                                IODirectives ioDirectives(directiveMap);
                                                IOSystem::getInstance().getWriter(SymbolMask({0, 0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_4_B);
                                        } catch (std::exception& e) {std::cerr << e.what();exit(1);}
                                }
                                if (!isHintsProfilingEnabled() && (performIO || 0)) rel_4_B->purge();
                                if (!isHintsProfilingEnabled() && (performIO || 0)) rel_4_B->purge();

                                // -- relation hint statistics --
                                if(isHintsProfilingEnabled()) {
                                        std::cout << " -- Operation Hint Statistics --\n";
                                        std::cout << "Relation rel_1_A:\n";
                                        rel_1_A->printHintStatistics(std::cout,"  ");
                                        std::cout << "\n";
                                        std::cout << "Relation rel_2_delta_A:\n";
                                        rel_2_delta_A->printHintStatistics(std::cout,"  ");
                                        std::cout << "\n";
                                        std::cout << "Relation rel_3_new_A:\n";
                                        rel_3_new_A->printHintStatistics(std::cout,"  ");
                                        std::cout << "\n";
                                        std::cout << "Relation rel_4_B:\n";
                                        rel_4_B->printHintStatistics(std::cout,"  ");
                                        std::cout << "\n";
                                        std::cout << "Relation rel_5_delta_B:\n";
                                        rel_5_delta_B->printHintStatistics(std::cout,"  ");
                                        std::cout << "\n";
                                        std::cout << "Relation rel_6_new_B:\n";
                                        rel_6_new_B->printHintStatistics(std::cout,"  ");
                                        std::cout << "\n";
                                }
                                SignalHandler::instance()->reset();
                        }
                public:
                        void run() { runFunction<false>(); }
                public:
                        void runAll(std::string inputDirectory = ".", std::string outputDirectory = ".") { runFunction<true>(inputDirectory, outputDirectory); }
                public:
                        void printAll(std::string outputDirectory = ".") {
                                try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"attributeNames","x\ty\tz"},{"filename","./A.csv"},{"name","A"}});
                                        if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
                                        IODirectives ioDirectives(directiveMap);
                                        IOSystem::getInstance().getWriter(SymbolMask({0, 0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
                                } catch (std::exception& e) {std::cerr << e.what();exit(1);}
                                try {std::map<std::string, std::string> directiveMap({{"IO","file"},{"attributeNames","x\ty\tz"},{"filename","./B.csv"},{"name","B"}});
                                        if (!outputDirectory.empty() && directiveMap["IO"] == "file" && directiveMap["filename"].front() != '/') {directiveMap["filename"] = outputDirectory + "/" + directiveMap["filename"];}
                                        IODirectives ioDirectives(directiveMap);
                                        IOSystem::getInstance().getWriter(SymbolMask({0, 0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_4_B);
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
                                        IOSystem::getInstance().getWriter(SymbolMask({0, 0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_1_A);
                                } catch (std::exception& e) {std::cerr << e.what();exit(1);}
                                try {IODirectives ioDirectives;
                                        ioDirectives.setIOType("stdout");
                                        ioDirectives.setRelationName("rel_4_B");
                                        IOSystem::getInstance().getWriter(SymbolMask({0, 0, 0}), symTable, ioDirectives, 0)->writeAll(*rel_4_B);
                                } catch (std::exception& e) {std::cerr << e.what();exit(1);}
                        }
                public:
                        const SymbolTable &getSymbolTable() const {
                                return symTable;
                        }
        };
        SouffleProgram *newInstance___souffleSfcCZ8(){return new Sf___souffleSfcCZ8;}
        SymbolTable *getST___souffleSfcCZ8(SouffleProgram *p){return &reinterpret_cast<Sf___souffleSfcCZ8*>(p)->symTable;}
#ifdef __EMBEDDED_SOUFFLE__
        class factory_Sf___souffleSfcCZ8: public souffle::ProgramFactory {
                SouffleProgram *newInstance() {
                        return new Sf___souffleSfcCZ8();
                };
                public:
                factory_Sf___souffleSfcCZ8() : ProgramFactory("__souffleSfcCZ8"){}
        };
        static factory_Sf___souffleSfcCZ8 __factory_Sf___souffleSfcCZ8_instance;
}
#else
}
int main(int argc, char** argv)
{
        try{
                souffle::CmdOptions opt(R"(hash.dl)",
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
                souffle::Sf___souffleSfcCZ8 obj;
                obj.runAll(opt.getInputFileDir(), opt.getOutputFileDir());
                return 0;
        } catch(std::exception &e) { souffle::SignalHandler::instance()->error(e.what());}
}
#endif
