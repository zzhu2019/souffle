/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file EventProcessor.h
 *
 * Declares classes for event processor that parse profile events and 
 * populate the profile database
 *
 ***********************************************************************/

#pragma once 

#include <cassert>
#include <cstdarg>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ProfileDatabase.h"

namespace souffle {

/**
 * Abstract Class for EventProcessor
 */
class EventProcessor {
public:
    virtual ~EventProcessor() {}

    /** abstract interface for processing an profile event */
    virtual void process(ProfileDatabase &db, const std::vector<std::string>& signature, va_list& /* args */) {
        std::cerr << "Unknown profiling processing event: "; 
        for (const auto& cur : signature) {
            std::cerr << cur << " ";
        }
        abort();
    }
};

/**
 * Event Processor Singleton
 * 
 * Singleton that is the connection point for events 
 */
class EventProcessorSingleton {
    /** keyword / event processor mapping */ 
    std::map<std::string, EventProcessor*> m_registry;

    EventProcessorSingleton() : m_registry() { }

    /** split string */ 
    static std::vector<std::string> split(std::string str, std::string split_str) {
        // repeat value when splitting so "a   b" -> ["a","b"] not ["a","","","","b"]
        bool repeat = (split_str.compare(" ") == 0);

        std::vector<std::string> elems;

        std::string temp;
        std::string hold;
        for (size_t i = 0; i < str.size(); i++) {
            if (repeat) {
                if (str.at(i) == split_str.at(0)) {
                    while (str.at(++i) == split_str.at(0))
                        ;  // set i to be at the end of the search string
                    elems.push_back(temp);
                    temp = "";
                }
                temp += str.at(i);
            } else {
                temp += str.at(i);
                hold += str.at(i);
                for (size_t j = 0; j < hold.size(); j++) {
                    if (hold[j] != split_str[j]) {
                        hold = "";
                    }
                }
                if (hold.size() == split_str.size()) {
                    elems.push_back(temp.substr(0, temp.size() - hold.size()));
                    hold = "";
                    temp = "";
                }
            }
        }
        if (!temp.empty()) {
            elems.push_back(temp);
        }

        return elems;
    }

    /** split string separated by semi-colon */
    static std::vector<std::string> splitSignature(std::string str) {
        for (size_t i = 0; i < str.size(); i++) {
            if (i > 0 && str[i] == ';' && str[i - 1] == '\\') {
                // I'm assuming this isn't a thing that will be naturally found in souffle profiler files
                str[i - 1] = '\b';
                str.erase(i--, 1);
            }
        }
        bool changed = false;
        std::vector<std::string> result = split(str, ";");
        for (size_t i = 0; i < result.size(); i++) {
            for (size_t j = 0; j < result[i].size(); j++) {
                if (result[i][j] == '\b') {
                    result[i][j] = ';';
                    changed = true;
                }
            }
            if (changed) {
                changed = false;
            }
        }
        return result;
    }

public:
    /** get instance */  
    static EventProcessorSingleton& instance() {
        static EventProcessorSingleton singleton;
        return singleton;
    }

    /** register an event processor with its keyword */ 
    void registerEventProcessor(const std::string& keyword, EventProcessor* processor) {
        m_registry[keyword] = processor;
    }

    /** process a profile event */
    void process(ProfileDatabase &db, const char* txt, ...) {
        va_list args;
        va_start(args, txt);

        // obtain event signature by splitting event text 
        std::vector<std::string> eventSignature = splitSignature(txt);

        // invoke the event processor of the event
        const std::string& keyword = eventSignature[0];
        assert(eventSignature.size() > 0 && "no keyword in event description");
        for (auto const &cur : eventSignature) {
        std::cerr << cur  << " ";
        }
        std::cerr << "\n";
        assert(m_registry.find(keyword) != m_registry.end() && "EventProcessor not found!");
        m_registry[keyword]->process(db,eventSignature, args);

        // terminate access to variadic arguments
        va_end(args);
    }
};

/**
 * Non-Recursive Rule Timing Profile Event Processor 
 */
class NonRecursiveRuleTimingProcessor : public EventProcessor {
public:
    NonRecursiveRuleTimingProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@t-nonrecursive-rule", this);
    }
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        const std::string &rule = signature[3]; 
        milliseconds start = va_arg(args, milliseconds);
        milliseconds end = va_arg(args, milliseconds);
        db.addTextEntry({"program", "relation", relation, "non-recursive-rule", rule, "source-locator"}, srcLocator); 
        db.addDurationEntry({"program", "relation", relation, "non-recursive-rule", rule, "runtime"}, start, end); 
    }
} nonRecursiveRuleTimingProcessor;


/**
 * Non-Recursive Rule Number Profile Event Processor 
 */
class NonRecursiveRuleNumberProcessor : public EventProcessor {
public:
    NonRecursiveRuleNumberProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@n-nonrecursive-rule", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        const std::string &rule = signature[3]; 
        size_t num = va_arg(args, size_t);
        db.addTextEntry({"program", "relation", relation, "non-recursive-rule", rule, "source-locator"}, srcLocator); 
        db.addSizeEntry({"program", "relation", relation, "non-recursive-rule", rule, "num-tuples"}, num); 
    }
} nonRecursiveRuleNumberProcessor;

/**
 * Recursive Rule Timing Profile Event Processor 
 */
class RecursiveRuleTimingProcessor : public EventProcessor {
public:
    RecursiveRuleTimingProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@t-recursive-rule", this);
    }
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &version = signature[2];
        const std::string &srcLocator = signature[3]; 
        const std::string &rule = signature[4]; 
        milliseconds start = va_arg(args, milliseconds);
        milliseconds end = va_arg(args, milliseconds);
        std::string iteration = std::to_string(va_arg(args, size_t));
        db.addTextEntry({"program", "relation", relation, "recursive-rule", rule, version,  "source-locator"}, srcLocator); 
        db.addDurationEntry({"program", "relation", relation, "recursive-rule", rule, version, "iterations", iteration, "runtime"}, start, end); 
    }
} RecursiveRuleTimingProcessor;

/**
 * Recursive Rule Number Profile Event Processor 
 */
class RecursiveRuleNumberProcessor : public EventProcessor {
public:
    RecursiveRuleNumberProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@n-recursive-rule", this);
    }
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &version = signature[2];
        const std::string &srcLocator = signature[3]; 
        const std::string &rule = signature[4]; 
        size_t number = va_arg(args, size_t);
        std::string iteration = std::to_string(va_arg(args, size_t));
        db.addTextEntry({"program", "relation", relation, "recursive-rule", rule, version, "source-locator"}, srcLocator); 
        db.addSizeEntry({"program", "relation", relation, "recursive-rule", rule, version, "iterations", iteration, "num-tuples"}, number);
    }
} RecursiveRuleNumberProcessor;

/**
 * Non-Recursive Relation Number Profile Event Processor 
 */
class NonRecursiveRelationTimingProcessor : public EventProcessor {
public:
    NonRecursiveRelationTimingProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@t-nonrecursive-relation", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        milliseconds start = va_arg(args, milliseconds);
        milliseconds end = va_arg(args, milliseconds);
        db.addTextEntry({"program", "relation", relation, "source-locator"}, srcLocator); 
        db.addDurationEntry({"program", "relation", relation, "runtime"}, start, end); 
    }
} nonRecursiveRelationTimingProcessor;


/**
 * Non-Recursive Relation Number Profile Event Processor 
 */
class NonRecursiveRelationNumberProcessor : public EventProcessor {
public:
    NonRecursiveRelationNumberProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@n-nonrecursive-relation", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        size_t num = va_arg(args, size_t);
        db.addTextEntry({"program", "relation", relation, "source-locator"}, srcLocator); 
        db.addSizeEntry({"program", "relation", relation, "num-tuples"}, num); 
    }
} nonRecursiveRelationNumberProcessor;

/**
 * Recursive Relation Timing Profile Event Processor 
 */
class RecursiveRelationTimingProcessor : public EventProcessor {
public:
    RecursiveRelationTimingProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@t-recursive-relation", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        milliseconds start = va_arg(args, milliseconds);
        milliseconds end = va_arg(args, milliseconds);
        std::string iteration = std::to_string(va_arg(args, size_t));
        db.addTextEntry({"program", "relation", relation, "source-locator"}, srcLocator); 
        db.addDurationEntry({"program", "relation", relation, "iterations", iteration, "runtime"}, start, end); 
    }
} recursiveRelationTimingProcessor;

/**
 * Recursive Relation Timing Profile Event Processor 
 */
class RecursiveRelationNumberProcessor : public EventProcessor {
public:
    RecursiveRelationNumberProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@n-recursive-relation", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        size_t number  = va_arg(args, size_t);
        std::string iteration = std::to_string(va_arg(args, size_t));
        db.addTextEntry({"program", "relation", relation, "source-locator"}, srcLocator); 
        db.addSizeEntry({"program", "relation", relation, "iterations", iteration, "num-tuples"}, number); 
    }
} recursiveRelationNumberProcessor;

/**
 * Recursive Relation Copy Timing Profile Event Processor 
 */
class RecursiveRelationCopyTimingProcessor : public EventProcessor {
public:
    RecursiveRelationCopyTimingProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@c-recursive-relation", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        const std::string &relation = signature[1];
        const std::string &srcLocator = signature[2]; 
        milliseconds start = va_arg(args, milliseconds);
        milliseconds end = va_arg(args, milliseconds);
        std::string iteration = std::to_string(va_arg(args, size_t));
        db.addTextEntry({"program", "relation", relation, "source-locator"}, srcLocator); 
        db.addDurationEntry({"program", "relation", relation, "iterations", iteration, "copytime"}, start, end); 
    }
} recursiveRelationCopyTimingProcessor;

/**
 * Program Run Event Processor 
 */
class ProgramRuntimeProcessor : public EventProcessor {
public:
    ProgramRuntimeProcessor() {
        EventProcessorSingleton::instance().registerEventProcessor("@runtime", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string> &signature, va_list& args) override {
        milliseconds start = va_arg(args, milliseconds);
        milliseconds end = va_arg(args, milliseconds);
        db.addDurationEntry({"program", "runtime"}, start, end); 
    }
} programRuntimeProcessor;

/**
 * EventProcessor B
 */
class EventProcessorB : public EventProcessor {
public:
    EventProcessorB() {
        std::cout << "Register EventProcessor B\n";
        EventProcessorSingleton::instance().registerEventProcessor("B", this);
    }
    /** process event input */
    void process(ProfileDatabase &db, const std::vector<std::string>& signature, va_list& args) override {
        std::cout << "Process B ";
        std::cout << va_arg(args, int) << " ";
        std::cout << va_arg(args, int) << "\n";
        for (const auto& cur : signature) {
            std::cout << cur << " ";
        }
        std::cout << std::endl;
    }
} pB;

}
