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

#include "EventProcessor.h"
#include "ProfileDatabase.h"
#include "Util.h"
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
    /** profile database */
    profile::ProfileDatabase database;

    ProfileEventSingleton() = default;

public:
    ~ProfileEventSingleton() = default;

    /** get instance */
    static ProfileEventSingleton& instance() {
        static ProfileEventSingleton singleton;
        return singleton;
    }

    /** create timing event */
    void makeTimingEvent(const std::string& txt, time_point start, time_point end, size_t iteration) {
        milliseconds start_ms = std::chrono::duration_cast<milliseconds>(start.time_since_epoch());
        milliseconds end_ms = std::chrono::duration_cast<milliseconds>(end.time_since_epoch());
        profile::EventProcessorSingleton::instance().process(
                database, txt.c_str(), start_ms, end_ms, iteration);
    }

    /** create quantity event */
    void makeQuantityEvent(const std::string& txt, size_t number, int iteration) {
        profile::EventProcessorSingleton::instance().process(database, txt.c_str(), number, iteration);
    }

    /** create utilisation event */
    void makeUtilisationEvent(const std::string& txt) {
        profile::EventProcessorSingleton::instance().process(database, txt.c_str());
    }

    /** Dump all events */
    void dump(std::ostream& os) {
        database.print(os);
    }

    /** Start timer */
    void startTimer() {}

    /** Stop timer */
    void stopTimer() {}

    const profile::ProfileDatabase& getDB() const {
        return database;
    }

    void setDBFromFile(const std::string& filename) {
        database = profile::ProfileDatabase(filename);
    }
};

}  // namespace souffle
