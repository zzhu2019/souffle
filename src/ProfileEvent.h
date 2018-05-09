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

namespace souffle {

/**
 * Profile Event Singleton
 */
class ProfileEventSingleton {
public:
    ~ProfileEventSingleton() {
    }
    static ProfileEventSingleton& instance() {
        static ProfileEventSingleton singleton;
        return singleton;
    }

    /** Make new time event */
    void makeTimingEvent(const std::string& txt, time_point start, time_point end, size_t iteration) {
    }

    /** Make new quantity event */
    void makeQuantityEvent(const std::string& txt, size_t number, int iteration) {
    }

    /** Make new utilisation event */
    void makeUtilisationEvent(const std::string& txt) {
    }

    /** Dump all events */
    void dump(std::ostream& os) {
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

