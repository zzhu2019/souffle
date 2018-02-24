/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Interpreter.cpp
 *
 * Implementation of the RAM interpreter.
 *
 ***********************************************************************/

#include "Interpreter.h"
#include "AstLogStatement.h"
#include "AstRelation.h"
#include "AstTranslator.h"
#include "AstVisitor.h"
#include "BinaryConstraintOps.h"
#include "BinaryFunctorOps.h"
#include "Global.h"
#include "IOSystem.h"
#include "InterpreterRecords.h"
#include "Logger.h"
#include "Macro.h"
#include "RamVisitor.h"
#include "SignalHandler.h"
#include "TypeSystem.h"
#include "UnaryFunctorOps.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <regex>
#include <utility>

#include <unistd.h>

#ifdef _OPENMP
#include <omp.h>
#endif


namespace souffle {

/** Evaluate RAM Value */
RamDomain Interpreter::eval(const RamValue& value, const EvalContext& ctxt) 
{
    class ValueEvaluator : public RamVisitor<RamDomain> {
        InterpreterEnvironment& env;
        const EvalContext& ctxt;

    public:
        ValueEvaluator(InterpreterEnvironment& env, const EvalContext& ctxt) : env(env), ctxt(ctxt) {}

        // -- basics --
        RamDomain visitNumber(const RamNumber& num) override {
            return num.getConstant();
        }

        RamDomain visitElementAccess(const RamElementAccess& access) override {
            return ctxt[access.getLevel()][access.getElement()];
        }

        RamDomain visitAutoIncrement(const RamAutoIncrement& /*inc*/) override {
            return env.incCounter();
        }

        // unary functions

        RamDomain visitUnaryOperator(const RamUnaryOperator& op) override {
            switch (op.getOperator()) {
                case UnaryOp::NEG:
                    return -visit(op.getValue());
                case UnaryOp::BNOT:
                    return ~visit(op.getValue());
                case UnaryOp::LNOT:
                    return !visit(op.getValue());
                case UnaryOp::ORD:
                    return visit(op.getValue());
                case UnaryOp::STRLEN:
                    return strlen(env.getSymbolTable().resolve(visit(op.getValue())));
                case UnaryOp::SIN:
                    return sin(visit(op.getValue()));
                case UnaryOp::COS:
                    return cos(visit(op.getValue()));
                case UnaryOp::TAN:
                    return tan(visit(op.getValue()));
                case UnaryOp::ASIN:
                    return asin(visit(op.getValue()));
                case UnaryOp::ACOS:
                    return acos(visit(op.getValue()));
                case UnaryOp::ATAN:
                    return atan(visit(op.getValue()));
                case UnaryOp::SINH:
                    return sinh(visit(op.getValue()));
                case UnaryOp::COSH:
                    return cosh(visit(op.getValue()));
                case UnaryOp::TANH:
                    return tanh(visit(op.getValue()));
                case UnaryOp::ASINH:
                    return asinh(visit(op.getValue()));
                case UnaryOp::ACOSH:
                    return acosh(visit(op.getValue()));
                case UnaryOp::ATANH:
                    return atanh(visit(op.getValue()));
                case UnaryOp::LOG:
                    return log(visit(op.getValue()));
                case UnaryOp::EXP:
                    return exp(visit(op.getValue()));
                default:
                    assert(0 && "unsupported operator");
                    return 0;
            }
        }

        // binary functions

        RamDomain visitBinaryOperator(const RamBinaryOperator& op) override {
            switch (op.getOperator()) {
                // arithmetic
                case BinaryOp::ADD: {
                    return visit(op.getLHS()) + visit(op.getRHS());
                }
                case BinaryOp::SUB: {
                    return visit(op.getLHS()) - visit(op.getRHS());
                }
                case BinaryOp::MUL: {
                    return visit(op.getLHS()) * visit(op.getRHS());
                }
                case BinaryOp::DIV: {
                    RamDomain rhs = visit(op.getRHS());
                    return visit(op.getLHS()) / rhs;
                }
                case BinaryOp::EXP: {
                    return std::pow(visit(op.getLHS()), visit(op.getRHS()));
                }
                case BinaryOp::MOD: {
                    RamDomain rhs = visit(op.getRHS());
                    return visit(op.getLHS()) % rhs;
                }
                case BinaryOp::BAND: {
                    return visit(op.getLHS()) & visit(op.getRHS());
                }
                case BinaryOp::BOR: {
                    return visit(op.getLHS()) | visit(op.getRHS());
                }
                case BinaryOp::BXOR: {
                    return visit(op.getLHS()) ^ visit(op.getRHS());
                }
                case BinaryOp::LAND: {
                    return visit(op.getLHS()) && visit(op.getRHS());
                }
                case BinaryOp::LOR: {
                    return visit(op.getLHS()) || visit(op.getRHS());
                }
                case BinaryOp::MAX: {
                    return std::max(visit(op.getLHS()), visit(op.getRHS()));
                }
                case BinaryOp::MIN: {
                    return std::min(visit(op.getLHS()), visit(op.getRHS()));
                }

                // strings
                case BinaryOp::CAT: {
                    return env.getSymbolTable().lookup((
                            std::string(env.getSymbolTable().resolve(visit(op.getLHS()))) +
                            std::string(env.getSymbolTable().resolve(
                                    visit(op.getRHS())))).c_str());
                }
                default:
                    assert(0 && "unsupported operator");
                    return 0;
            }
        }

        // ternary functions

        RamDomain visitTernaryOperator(const RamTernaryOperator& op) override {
            switch (op.getOperator()) {
                case TernaryOp::SUBSTR: {
                    auto symbol = visit(op.getArg(0));
                    std::string str = env.getSymbolTable().resolve(symbol);
                    auto idx = visit(op.getArg(1));
                    auto len = visit(op.getArg(2));
                    std::string sub_str;
                    try {
                        sub_str = str.substr(idx, len);
                    } catch (...) {
                        std::cerr << "warning: wrong index position provided by substr(\"";
                        std::cerr << str << "\"," << (int32_t)idx << "," << (int32_t)len << ") functor.\n";
                    }
                    return env.getSymbolTable().lookup(sub_str.c_str());
                }
                default:
                    assert(0 && "unsupported operator");
                    return 0;
            }
        }

        // -- records --

        RamDomain visitPack(const RamPack& op) override {
            auto values = op.getValues();
            auto arity = values.size();
            RamDomain data[arity];
            for (size_t i = 0; i < arity; ++i) {
                data[i] = visit(values[i]);
            }
            return pack(data, arity);
        }

        // -- subroutine argument

        RamDomain visitArgument(const RamArgument& arg) override {
            return ctxt.getArgument(arg.getArgNumber());
        }

        // -- safety net --

        RamDomain visitNode(const RamNode& node) override {
            std::cerr << "Unsupported node type: " << typeid(node).name() << "\n";
            assert(false && "Unsupported Node Type!");
            return 0;
        }
    };

    // create and run evaluator
    return ValueEvaluator(env, ctxt)(value);
}

/** Evaluate RAM Condition */ 
bool Interpreter::eval(const RamCondition& cond, const EvalContext& ctxt) {
    class ConditionEvaluator : public RamVisitor<bool> {
        InterpreterEnvironment& env;
        const EvalContext& ctxt;
        Interpreter &interpreter;

    public:
        ConditionEvaluator(InterpreterEnvironment& env, const EvalContext& ctxt, Interpreter &intp) : env(env), ctxt(ctxt), interpreter(intp) {}

        // -- connectors operators --

        bool visitAnd(const RamAnd& a) override {
            return visit(a.getLHS()) && visit(a.getRHS());
        }

        // -- relation operations --

        bool visitEmpty(const RamEmpty& empty) override {
            return env.getRelation(empty.getRelation()).empty();
        }

        bool visitNotExists(const RamNotExists& ne) override {
            const InterpreterRelation& rel = env.getRelation(ne.getRelation());

            // construct the pattern tuple
            auto arity = rel.getArity();
            auto values = ne.getValues();

            // for total we use the exists test
            if (ne.isTotal()) {
                RamDomain tuple[arity];
                for (size_t i = 0; i < arity; i++) {
                    tuple[i] = (values[i]) ? interpreter.eval(values[i], ctxt) : MIN_RAM_DOMAIN;
                }

                return !rel.exists(tuple);
            }

            // for partial we search for lower and upper boundaries
            RamDomain low[arity];
            RamDomain high[arity];
            for (size_t i = 0; i < arity; i++) {
                low[i] = (values[i]) ? interpreter.eval(values[i], ctxt) : MIN_RAM_DOMAIN;
                high[i] = (values[i]) ? low[i] : MAX_RAM_DOMAIN;
            }

            // obtain index
            auto idx = rel.getIndex(ne.getKey());
            auto range = idx->lowerUpperBound(low, high);
            return range.first == range.second;  // if there are none => done
        }

        // -- comparison operators --

        bool visitBinaryRelation(const RamBinaryRelation& relOp) override {
            switch (relOp.getOperator()) {
                // comparison operators
                case BinaryConstraintOp::EQ:
                    return interpreter.eval(relOp.getLHS(), ctxt) == interpreter.eval(relOp.getRHS(), ctxt);
                case BinaryConstraintOp::NE:
                    return interpreter.eval(relOp.getLHS(), ctxt) != interpreter.eval(relOp.getRHS(), ctxt);
                case BinaryConstraintOp::LT:
                    return interpreter.eval(relOp.getLHS(), ctxt) < interpreter.eval(relOp.getRHS(), ctxt);
                case BinaryConstraintOp::LE:
                    return interpreter.eval(relOp.getLHS(), ctxt) <= interpreter.eval(relOp.getRHS(), ctxt);
                case BinaryConstraintOp::GT:
                    return interpreter.eval(relOp.getLHS(), ctxt) > interpreter.eval(relOp.getRHS(), ctxt);
                case BinaryConstraintOp::GE:
                    return interpreter.eval(relOp.getLHS(), ctxt) >= interpreter.eval(relOp.getRHS(), ctxt);

                // strings
                case BinaryConstraintOp::MATCH: {
                    RamDomain l = interpreter.eval(relOp.getLHS(), ctxt);
                    RamDomain r = interpreter.eval(relOp.getRHS(), ctxt);
                    const std::string& pattern = env.getSymbolTable().resolve(l);
                    const std::string& text = env.getSymbolTable().resolve(r);
                    bool result = false;
                    try {
                        result = std::regex_match(text, std::regex(pattern));
                    } catch (...) {
                        std::cerr << "warning: wrong pattern provided for match(\"" << pattern << "\",\""
                                  << text << "\")\n";
                    }
                    return result;
                }
                case BinaryConstraintOp::NOT_MATCH: {
                    RamDomain l = interpreter.eval(relOp.getLHS(), ctxt);
                    RamDomain r = interpreter.eval(relOp.getRHS(), ctxt);
                    const std::string& pattern = env.getSymbolTable().resolve(l);
                    const std::string& text = env.getSymbolTable().resolve(r);
                    bool result = false;
                    try {
                        result = !std::regex_match(text, std::regex(pattern));
                    } catch (...) {
                        std::cerr << "warning: wrong pattern provided for !match(\"" << pattern << "\",\""
                                  << text << "\")\n";
                    }
                    return result;
                }
                case BinaryConstraintOp::CONTAINS: {
                    RamDomain l = interpreter.eval(relOp.getLHS(), ctxt);
                    RamDomain r = interpreter.eval(relOp.getRHS(), ctxt);
                    const std::string& pattern = env.getSymbolTable().resolve(l);
                    const std::string& text = env.getSymbolTable().resolve(r);
                    return text.find(pattern) != std::string::npos;
                }
                case BinaryConstraintOp::NOT_CONTAINS: {
                    RamDomain l = interpreter.eval(relOp.getLHS(), ctxt);
                    RamDomain r = interpreter.eval(relOp.getRHS(), ctxt);
                    const std::string& pattern = env.getSymbolTable().resolve(l);
                    const std::string& text = env.getSymbolTable().resolve(r);
                    return text.find(pattern) == std::string::npos;
                }
                default:
                    assert(0 && "unsupported operator");
                    return 0;
            }
        }

        // -- safety net --

        bool visitNode(const RamNode& node) override {
            std::cerr << "Unsupported node type: " << typeid(node).name() << "\n";
            assert(false && "Unsupported Node Type!");
            return 0;
        }
    };

    // run evaluator
    return ConditionEvaluator(env, ctxt, *this)(cond);
}

/** Evaluate RAM operation */
void Interpreter::eval(const RamOperation& op, const EvalContext& args ) 
{
    class OperationEvaluator : public RamVisitor<void> {
        InterpreterEnvironment& env;
        EvalContext& ctxt;
        Interpreter& interpreter;

    public:
        OperationEvaluator(InterpreterEnvironment& env, EvalContext& ctxt, Interpreter &interp) : env(env), ctxt(ctxt), interpreter(interp) {}

        // -- Operations -----------------------------

        void visitSearch(const RamSearch& search) override {
            // check condition
            auto condition = search.getCondition();
            if (condition && !interpreter.eval(*condition, ctxt)) {
                return;  // condition not valid => skip nested
            }

            // process nested
            visit(*search.getNestedOperation());
        }

        void visitScan(const RamScan& scan) override {
            // get the targeted relation
            const InterpreterRelation& rel = env.getRelation(scan.getRelation());

            // process full scan if no index is given
            if (scan.getRangeQueryColumns() == 0) {
                // if scan is not binding anything => check for emptiness
                if (scan.isPureExistenceCheck() && !rel.empty()) {
                    visitSearch(scan);
                    return;
                }

                // if scan is unrestricted => use simple iterator
                for (const RamDomain* cur : rel) {
                    ctxt[scan.getLevel()] = cur;
                    visitSearch(scan);
                }
                return;
            }

            // create pattern tuple for range query
            auto arity = rel.getArity();
            RamDomain low[arity];
            RamDomain hig[arity];
            auto pattern = scan.getRangePattern();
            for (size_t i = 0; i < arity; i++) {
                if (pattern[i] != nullptr) {
                    low[i] = interpreter.eval(pattern[i], ctxt);
                    hig[i] = low[i];
                } else {
                    low[i] = MIN_RAM_DOMAIN;
                    hig[i] = MAX_RAM_DOMAIN;
                }
            }

            // obtain index
            auto idx = rel.getIndex(scan.getRangeQueryColumns(), nullptr);

            // get iterator range
            auto range = idx->lowerUpperBound(low, hig);

            // if this scan is not binding anything ...
            if (scan.isPureExistenceCheck()) {
                if (range.first != range.second) {
                    visitSearch(scan);
                }
                return;
            }

            // conduct range query
            for (auto ip = range.first; ip != range.second; ++ip) {
                const RamDomain* data = *(ip);
                ctxt[scan.getLevel()] = data;
                visitSearch(scan);
            }
        }

        void visitLookup(const RamLookup& lookup) override {
            // get reference
            RamDomain ref = ctxt[lookup.getReferenceLevel()][lookup.getReferencePosition()];

            // check for null
            if (isNull(ref)) {
                return;
            }

            // update environment variable
            auto arity = lookup.getArity();
            const RamDomain* tuple = unpack(ref, arity);

            // save reference to temporary value
            ctxt[lookup.getLevel()] = tuple;

            // run nested part - using base class visitor
            visitSearch(lookup);
        }

        void visitAggregate(const RamAggregate& aggregate) override {
            // get the targeted relation
            const InterpreterRelation& rel = env.getRelation(aggregate.getRelation());

            // initialize result
            RamDomain res = 0;
            switch (aggregate.getFunction()) {
                case RamAggregate::MIN:
                    res = MAX_RAM_DOMAIN;
                    break;
                case RamAggregate::MAX:
                    res = MIN_RAM_DOMAIN;
                    break;
                case RamAggregate::COUNT:
                    res = 0;
                    break;
                case RamAggregate::SUM:
                    res = 0;
                    break;
            }

            // init temporary tuple for this level
            auto arity = rel.getArity();

            // get lower and upper boundaries for iteration
            const auto& pattern = aggregate.getPattern();
            RamDomain low[arity];
            RamDomain hig[arity];

            for (size_t i = 0; i < arity; i++) {
                if (pattern[i] != nullptr) {
                    low[i] = interpreter.eval(pattern[i], ctxt);
                    hig[i] = low[i];
                } else {
                    low[i] = MIN_RAM_DOMAIN;
                    hig[i] = MAX_RAM_DOMAIN;
                }
            }

            // obtain index
            auto idx = rel.getIndex(aggregate.getRangeQueryColumns());

            // get iterator range
            auto range = idx->lowerUpperBound(low, hig);

            // check for emptiness
            if (aggregate.getFunction() != RamAggregate::COUNT) {
                if (range.first == range.second) {
                    return;  // no elements => no min/max
                }
            }

            // iterate through values
            for (auto ip = range.first; ip != range.second; ++ip) {
                // link tuple
                const RamDomain* data = *(ip);
                ctxt[aggregate.getLevel()] = data;

                // count is easy
                if (aggregate.getFunction() == RamAggregate::COUNT) {
                    res++;
                    continue;
                }

                // aggregation is a bit more difficult

                // eval target expression
                RamDomain cur = interpreter.eval(aggregate.getTargetExpression(), ctxt);

                switch (aggregate.getFunction()) {
                    case RamAggregate::MIN:
                        res = std::min(res, cur);
                        break;
                    case RamAggregate::MAX:
                        res = std::max(res, cur);
                        break;
                    case RamAggregate::COUNT:
                        res = 0;
                        break;
                    case RamAggregate::SUM:
                        res += cur;
                        break;
                }
            }

            // write result to environment
            RamDomain tuple[1];
            tuple[0] = res;
            ctxt[aggregate.getLevel()] = tuple;

            // check whether result is used in a condition
            auto condition = aggregate.getCondition();
            if (condition && !interpreter.eval(*condition, ctxt)) {
                return;  // condition not valid => skip nested
            }

            // run nested part - using base class visitor
            visitSearch(aggregate);
        }

        void visitProject(const RamProject& project) override {
            // check constraints
            RamCondition* condition = project.getCondition();
            if (condition && !interpreter.eval(*condition, ctxt)) {
                return;  // condition violated => skip insert
            }

            // create a tuple of the proper arity (also supports arity 0)
            auto arity = project.getRelation().getArity();
            const auto& values = project.getValues();
            RamDomain tuple[arity];
            for (size_t i = 0; i < arity; i++) {
                tuple[i] = interpreter.eval(values[i], ctxt);
            }

            // check filter relation
            if (project.hasFilter() && env.getRelation(project.getFilter()).exists(tuple)) {
                return;
            }

            // insert in target relation
            env.getRelation(project.getRelation()).insert(tuple);
        }

        // -- return from subroutine --
        void visitReturn(const RamReturn& ret) override {
            for (auto val : ret.getValues()) {
                if (val == nullptr) {
                    ctxt.addReturnValue(0, true);
                } else {
                    ctxt.addReturnValue(interpreter.eval(val, ctxt));
                }
            }
        }

        // -- safety net --
        void visitNode(const RamNode& node) override {
            std::cerr << "Unsupported node type: " << typeid(node).name() << "\n";
            assert(false && "Unsupported Node Type!");
        }
    };

    // create and run interpreter for operations 
    EvalContext ctxt(op.getDepth());
    ctxt.setReturnValues(args.getReturnValues());
    ctxt.setReturnErrors(args.getReturnErrors());
    ctxt.setArguments(args.getArguments());
    OperationEvaluator(env, ctxt, *this).visit(op);
}

/** Evaluate RAM statement */ 
void Interpreter::eval(const RamStatement& stmt, std::ostream *profile) 
{
    class StatementEvaluator : public RamVisitor<bool> {
        InterpreterEnvironment& env;
        std::ostream* profile;
        Interpreter& interpreter;
    public:
        StatementEvaluator(
                InterpreterEnvironment& env, std::ostream* profile, Interpreter& interp)
                : env(env), profile(profile), interpreter(interp) {}

        // -- Statements -----------------------------

        bool visitSequence(const RamSequence& seq) override {
            // process all statements in sequence
            for (const auto& cur : seq.getStatements()) {
                if (!visit(cur)) {
                    return false;
                }
            }

            // all processed successfully
            return true;
        }

        bool visitParallel(const RamParallel& parallel) override {
            // get statements to be processed in parallel
            const auto& stmts = parallel.getStatements();

            // special case: empty
            if (stmts.empty()) {
                return true;
            }

            // special handling for a single child
            if (stmts.size() == 1) {
                return visit(stmts[0]);
            }

            // parallel execution
            bool cond = true;
#pragma omp parallel for reduction(&& : cond)
            for (size_t i = 0; i < stmts.size(); i++) {
                cond = cond && visit(stmts[i]);
            }
            return cond;
        }

        bool visitLoop(const RamLoop& loop) override {
            while (visit(loop.getBody())) {
            }
            return true;
        }

        bool visitExit(const RamExit& exit) override {
            return !interpreter.eval(exit.getCondition());
        }

        bool visitLogTimer(const RamLogTimer& timer) override {
            Logger logger(
                    timer.getMessage().c_str(), *profile, fileExtension(Global::config().get("profile")));
            return visit(timer.getStatement());
        }

        bool visitDebugInfo(const RamDebugInfo& dbg) override {
            SignalHandler::instance()->setMsg(dbg.getMessage().c_str());
            return visit(dbg.getStatement());
        }

        bool visitCreate(const RamCreate& create) override {
            env.getRelation(create.getRelation());
            return true;
        }

        bool visitClear(const RamClear& clear) override {
            env.getRelation(clear.getRelation()).purge();
            return true;
        }

        bool visitDrop(const RamDrop& drop) override {
            env.dropRelation(drop.getRelation());
            return true;
        }

        bool visitPrintSize(const RamPrintSize& print) override {
            auto lease = getOutputLock().acquire();
            (void)lease;
            std::cout << print.getMessage() << env.getRelation(print.getRelation()).size() << "\n";
            return true;
        }

        bool visitLogSize(const RamLogSize& print) override {
            auto lease = getOutputLock().acquire();
            (void)lease;
            *profile << print.getMessage() << env.getRelation(print.getRelation()).size();
            const std::string ext = fileExtension(Global::config().get("profile"));
            if (ext == "json") {
                *profile << "},";
            }
            *profile << "\n";
            return true;
        }

        bool visitLoad(const RamLoad& load) override {
            try {
                InterpreterRelation& relation = env.getRelation(load.getRelation());
                std::unique_ptr<ReadStream> reader = IOSystem::getInstance().getReader(
                        load.getRelation().getSymbolMask(), env.getSymbolTable(), load.getIODirectives(),
                        Global::config().has("provenance"));
                reader->readAll(relation);
            } catch (std::exception& e) {
                std::cerr << e.what();
                return false;
            }
            return true;
        }

        bool visitStore(const RamStore& store) override {
            for (IODirectives ioDirectives : store.getIODirectives()) {
                try {
                    IOSystem::getInstance()
                            .getWriter(store.getRelation().getSymbolMask(), env.getSymbolTable(),
                                    ioDirectives, Global::config().has("provenance"))
                            ->writeAll(env.getRelation(store.getRelation()));
                } catch (std::exception& e) {
                    std::cerr << e.what();
                    exit(1);
                }
            }
            return true;
        }

        bool visitFact(const RamFact& fact) override {
            auto arity = fact.getRelation().getArity();
            RamDomain tuple[arity];
            auto values = fact.getValues();

            for (size_t i = 0; i < arity; ++i) {
                tuple[i] = interpreter.eval(values[i]);
            }

            env.getRelation(fact.getRelation()).insert(tuple);
            return true;
        }

        bool visitInsert(const RamInsert& insert) override {
            // run generic query executor
            interpreter.eval(insert.getOperation());
            return true;
        }

        bool visitMerge(const RamMerge& merge) override {
            // get involved relation
            InterpreterRelation& src = env.getRelation(merge.getSourceRelation());
            InterpreterRelation& trg = env.getRelation(merge.getTargetRelation());

            if (dynamic_cast<InterpreterEqRelation*>(&trg)) {
                // expand src with the new knowledge generated by insertion.
                src.extend(trg);
            }
            // merge in all elements
            trg.insert(src);

            // done
            return true;
        }

        bool visitSwap(const RamSwap& swap) override {
            std::swap(env.getRelation(swap.getFirstRelation()), env.getRelation(swap.getSecondRelation()));
            return true;
        }

        // -- safety net --

        bool visitNode(const RamNode& node) override {
            auto lease = getOutputLock().acquire();
            (void)lease;
            std::cerr << "Unsupported node type: " << typeid(node).name() << "\n";
            assert(false && "Unsupported Node Type!");
            return false;
        }
    };

    // create and run interpreter for statements 
    StatementEvaluator(env, profile, *this).visit(stmt);
}

/** Execute main program of a translation unit */
void Interpreter::execute() 
{
    SignalHandler::instance()->set();
    const RamStatement &main = *translationUnit.getP().getMain(); 

    if (Global::config().has("profile")) {
        std::string fname = Global::config().get("profile");
        // open output stream
        std::ofstream os(fname);
        if (!os.is_open()) {
            throw std::invalid_argument("Cannot open profile log file <" + fname + ">");
        }
        os << AstLogStatement::startDebug() << std::endl;
        eval(main, &os);
    } else {
        eval(main);
    }
    SignalHandler::instance()->reset();
}

/** Execute subroutine */
void Interpreter::executeSubroutine(const RamStatement& stmt,
        const std::vector<RamDomain>& arguments, std::vector<RamDomain>& returnValues,
        std::vector<bool>& returnErrors){
    EvalContext ctxt;
    ctxt.setReturnValues(returnValues);
    ctxt.setReturnErrors(returnErrors);
    ctxt.setArguments(arguments);

    // run subroutine
    const RamOperation& op = static_cast<const RamInsert&>(stmt).getOperation();
    eval(op, ctxt);
}

}  // end of namespace souffle
