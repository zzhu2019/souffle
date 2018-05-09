/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ProfileEvent.h
 *
 * Declares classes for profile events
 *
 ***********************************************************************/

#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <sys/resource.h>
#include <sys/time.h>

#include "ProfileDatabase.h"
#include "EventProcessor.h"

namespace souffle {

/**
 * Profile Event Singleton
 */
class ProfileEventSingleton {
    /** profile database */
    ProfileDatabase m_database;

    ProfileEventSingleton() : m_database() { }
public:
    ~ProfileEventSingleton() {
    }

    /** get instance */ 
    static ProfileEventSingleton& instance() {
        static ProfileEventSingleton singleton;
        return singleton;
    }

    /** create timing event */ 
    void makeTimingEvent(const std::string& txt, time_point start, time_point end, size_t iteration) {
        milliseconds start_ms = std::chrono::duration_cast<milliseconds>(start.time_since_epoch());
        milliseconds end_ms = std::chrono::duration_cast<milliseconds>(end.time_since_epoch());
        EventProcessorSingleton::instance().process(m_database,txt.c_str(), start_ms, end_ms, iteration); 
    }

    /** create quantity event */
    void makeQuantityEvent(const std::string& txt, size_t number, int iteration) {
        EventProcessorSingleton::instance().process(m_database,txt.c_str(), number, iteration); 
    }

    /** create utilisation event */
    void makeUtilisationEvent(const std::string& txt) {
        EventProcessorSingleton::instance().process(m_database,txt.c_str()); 
    }

    /** Dump all events */
    void dump(std::ostream& os) {
        m_database.print(os);
    }

    /** Start timer */
    void startTimer() {
    }

    /** Stop timer */
    void stopTimer() {
    }

    /** Set profile log file */
    void setLog(std::ostream *os) {
    }
};


}  // namespace souffle

