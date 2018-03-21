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

#include <atomic>
#include <chrono>
#include <ctime>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <sys/resource.h>
#include <sys/time.h>
//#include "stdlib.h"
//#include "stdio.h"
//#include "string.h"

#pragma once

namespace souffle {

typedef uint32_t ProfileKey;
typedef uint32_t Interval;
typedef std::chrono::system_clock::time_point ProfileTimePoint;

/**
 * ProfileKey Class
 *
 * Class converts text to keys for profile messages
 * to reduce memory overheads.
 */
class ProfileKeySingleton {
    std::unordered_map<std::string, ProfileKey> map;
    std::vector<std::string> text;
    std::mutex keyMutex;
    ProfileKey last;

private:
    ProfileKeySingleton() : last(0){};

public:
    /** Get instance */
    static ProfileKeySingleton& instance() {
        static ProfileKeySingleton singleton;
        return singleton;
    }

    /** Lookup text */
    ProfileKey lookup(const std::string& txt) {
        auto it = map.find(txt);
        if (it != map.end()) {
            return it->second;
        } else {
            std::lock_guard<std::mutex> guard(keyMutex);
            map[txt] = last;
            text.push_back(txt);
            return last++;
        }
    }

    /** Get text */
    const std::string getText(ProfileKey key) {
        std::lock_guard<std::mutex> guard(keyMutex);
        assert(key < last && "corrupted key");
        return text[key];
    }
};

/** Abstract class for a profile event */
class ProfileEvent {
private:
    /** Key of profile event */
    ProfileKey key;

    /** Start time of event */
    ProfileTimePoint start;

protected:
    /** Convert ProfileTimePoint(time) to string */
    const std::string timeToString(ProfileTimePoint& time) {
        std::chrono::milliseconds ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch());

        std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ms);
        std::time_t t = s.count();
        std::size_t fractional_seconds = ms.count() % 1000;

        char buffer[80];
        struct tm* timeinfo = localtime(&t);
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S", timeinfo);
        std::string timeString(buffer);
        timeString += ":" + std::to_string(fractional_seconds);

        return timeString;
    }

public:
    ProfileEvent(const std::string& txt)
            : key(ProfileKeySingleton::instance().lookup(txt)), start(std::chrono::system_clock::now()) {}
    virtual ~ProfileEvent() = default;

    /** Get key */
    ProfileKey getKey() {
        return key;
    }

    /** Get start time */
    ProfileTimePoint getStart() {
        return start;
    }

    /** Print event */
    virtual void print(std::ostream& os) = 0;

    const std::string getStartString() {
        return timeToString(start);
    }
};

/** Timing Event */
class ProfileTimingEvent : public ProfileEvent {
private:
    /** end point */
    ProfileTimePoint end;

public:
    ProfileTimingEvent(const std::string& txt) : ProfileEvent(txt) {}

    /** Stop timing event */
    void stop() {
        end = std::chrono::system_clock::now();
    }

    const std::string getEndString() {
        return timeToString(end);
    }

    /** Print event */
    void print(std::ostream& os) override {
        auto duration = end - getStart();
        os << ProfileKeySingleton::instance().getText(getKey());
        os << getStartString() << ",";
        os << getEndString() << ",";
        os << std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        os << std::endl;
    }
};

/** Quantity event */
class ProfileQuantityEvent : public ProfileEvent {
private:
    /** number */
    size_t number;

public:
    ProfileQuantityEvent(const std::string& txt, size_t n) : ProfileEvent(txt), number(n) {}

    /** Get number */
    size_t getNumber() {
        return number;
    }

    /** Print event */
    void print(std::ostream& os) override {
        os << ProfileKeySingleton::instance().getText(getKey());
        os << getStartString() << ",";
        os << number;
        os << std::endl;
    }
};
/**
 * Profile Utilisation Event
 */
class ProfileUtilisationEvent : public ProfileEvent {
private:
    /** resouce statistics */
    struct rusage ru;
    // static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

public:
    ProfileUtilisationEvent(const std::string& txt) : ProfileEvent(txt) {
        getrusage(RUSAGE_SELF, &ru);
    }

    /** Print event */
    void print(std::ostream& os) override {
        os << ProfileKeySingleton::instance().getText(getKey());
        /* system CPU time used */
        double t = (double)ru.ru_stime.tv_sec * 1000000.0 + (double)ru.ru_stime.tv_usec;
        os << getStartString() << ",";
        os << std::to_string(t);
        // os << "," << getCurrentValue();
        os << std::endl;
    }
};

/**
 * Profile Memory Event
 */
class ProfileMemoryEvent : public ProfileEvent {
private:
    /** resource statistics */
    struct rusage ru;

public:
    ProfileMemoryEvent(const std::string& txt) : ProfileEvent(txt) {
        getrusage(RUSAGE_SELF, &ru);
    }

    /** Print event */
    void print(std::ostream& os) override {
        os << ProfileKeySingleton::instance().getText(getKey());
        os << getStartString() << ",";
        os << std::to_string(ru.ru_maxrss); /* maximum resident set size */
        os << std::endl;
    }
};

/**
 * Profile Event Singleton
 */
class ProfileEventSingleton {
private:
    /**  Profile Timer */
    class ProfileTimer {
    private:
        /** time interval between per utilisation read */
        Interval t;

        /** timer is running */
        bool running;

        /** thread timer runs on */
        std::thread th;

        /** run method for thread th */
        void run() {
            ProfileEventSingleton::instance().makeUtilisationEvent("utilisation");
            ProfileEventSingleton::instance().makeMemoryEvent("memory");
        }

        Interval getInterval() {
            return t;
        }

        bool getRunning() {
            return running;
        }

    public:
        ProfileTimer(Interval in = 1) : t(in), running(false) {}

        /** start timer on the thread th */
        void start() {
            running = true;

            th = std::thread([this]() {
                while (this->getRunning()) {
                    this->run();
                    auto x =
                            std::chrono::steady_clock::now() + std::chrono::milliseconds(this->getInterval());
                    std::this_thread::sleep_until(x);
                }
            });
        }

        /** stop timer on the thread th */
        void stop() {
            running = false;
            th.join();
        }
    };

private:
    ProfileEventSingleton() = default;
    std::list<ProfileEvent*> events;
    std::mutex eventMutex;
    ProfileTimer timer;

public:
    ~ProfileEventSingleton() {
        for (auto const& cur : events) {
            delete cur;
        }
    }
    static ProfileEventSingleton& instance() {
        static ProfileEventSingleton singleton;
        return singleton;
    }

    /** Make new time event */
    ProfileTimingEvent* makeTimingEvent(const std::string& txt) {
        ProfileTimingEvent* e = new ProfileTimingEvent(txt);
        std::lock_guard<std::mutex> guard(eventMutex);
        events.push_back(e);
        return e;
    }

    /** Make new quantity event */
    ProfileQuantityEvent* makeQuantityEvent(const std::string& txt, size_t n) {
        ProfileQuantityEvent* e = new ProfileQuantityEvent(txt, n);
        std::lock_guard<std::mutex> guard(eventMutex);
        events.push_back(e);
        return e;
    }

    /** Make new utilisation event */
    ProfileUtilisationEvent* makeUtilisationEvent(const std::string& txt) {
        ProfileUtilisationEvent* e = new ProfileUtilisationEvent(txt);
        std::lock_guard<std::mutex> guard(eventMutex);
        events.push_back(e);
        return e;
    }

    /** Make new memory event */
    ProfileMemoryEvent* makeMemoryEvent(const std::string& txt) {
        ProfileMemoryEvent* e = new ProfileMemoryEvent(txt);
        std::lock_guard<std::mutex> guard(eventMutex);
        events.push_back(e);
        return e;
    }

    /** Dump all events */
    void dump(std::ostream& os) {
        for (auto const& cur : events) {
            if (ProfileKeySingleton::instance().getText(cur->getKey()).compare("memory") == 0 || ProfileKeySingleton::instance().getText(cur->getKey()).compare("utilisation") == 0) {
                continue;
            }
            cur->print(os);
        }
    }

    /** Start timer */
    void startTimer() {
        timer.start();
    }

    /** Stop timer */
    void stopTimer() {
        timer.stop();
    }
};
}
