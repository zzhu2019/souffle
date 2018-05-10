/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Logger.h
 *
 * A logger is the utility utilized by RAM programs to create logs and
 * traces.
 *
 ***********************************************************************/

#pragma once

#include "ParallelUtils.h"
#include "ProfileEvent.h"

#include <chrono>
#include <iostream>
#include <utility>

namespace souffle {
/**
 * Obtains a reference to the lock synchronizing output operations.
 */
inline Lock& getOutputLock() {
    static Lock output_lock;
    return output_lock;
}

/**
 * The class utilized to times for the souffle profiling tool. This class
 * is utilized by both -- the interpreted and compiled version -- to conduct
 * the corresponding measurements.
 *
 * To far, only execution times are logged. More events, e.g. the number of
 * processed tuples may be added in the future.
 */
class Logger {
private:
    std::string m_label;
    time_point m_start;
    size_t m_iteration;

public:
    Logger(std::string label, size_t iteration)
            : m_label(std::move(label)), m_start(now()), m_iteration(iteration) {}
    ~Logger() {
        ProfileEventSingleton::instance().makeTimingEvent(m_label, m_start, now(), m_iteration);
    }
};
}  // end of namespace souffle
