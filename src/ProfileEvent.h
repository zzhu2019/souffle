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

#include <list>
#include <chrono>
#include <string>
#include <assert.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>

#pragma once 

namespace souffle {

typedef uint32_t ProfileKey; 
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
    ProfileKeySingleton() : last(0) { 
    };
public:
    /** Get instance */
    static ProfileKeySingleton &instance() {
        static ProfileKeySingleton singleton;
        return singleton;
    }

    /** Lookup text */ 
    ProfileKey lookup(const std::string &txt) { 
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
        assert(key < last && "corrupted key");
        return text[key];
    }
};

/** Abstract class for a profile event */
class ProfileEvent
{
    private:
        /** Key of profile event */
        ProfileKey key;

        /** Start time of event */
        ProfileTimePoint start;

    public:
        ProfileEvent(const std::string &txt) 
		: key(ProfileKeySingleton::instance().lookup(txt)), 
                  start(std::chrono::system_clock::now())
        {}
        virtual ~ProfileEvent() = default;

        /** Get key */
	ProfileKey getKey()
	{
		return key;
	}

        /** Get start time */
        ProfileTimePoint getStart() {
            return start;
        }

        /** Print event */
        virtual void print(std::ostream &os) = 0;
};

/** Timing Event */
class ProfileTimingEvent : public ProfileEvent
{
    private:
        /** end point */ 
        ProfileTimePoint end;

    public:
    	ProfileTimingEvent(const std::string &txt) : ProfileEvent(txt) {}

        /** Stop timing event */
        void stop() {
            end = std::chrono::system_clock::now();
        }

        /** Print event */
        void print(std::ostream &os) override { 
            auto duration = end - getStart();
            os << ProfileKeySingleton::instance().getText(getKey());
            os << std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
            os << std::endl;
        }
};

/** Quantity event */
class ProfileQuantityEvent : public ProfileEvent
{
    private:
        /** number */ 
    	size_t number;

    public:
    	ProfileQuantityEvent(const std::string &txt, size_t n) : ProfileEvent(txt), number(n) {}

        /** Get number */
        size_t getNumber() {
            return number;
        }

        /** Print event */
        void print(std::ostream &os) override { 
            os << ProfileKeySingleton::instance().getText(getKey());
            os << number;
            os << std::endl;
        }
};

/**
 * Profile Event Singleton
 */
class ProfileEventSingleton {
private:
   ProfileEventSingleton() = default;
   std::list<ProfileEvent *> events;
   std::mutex eventMutex;
public:
   ~ProfileEventSingleton() { 
      for (auto const &cur : events) { 
          delete cur;
      }
   }
   static ProfileEventSingleton &instance() {
       static ProfileEventSingleton singleton;
       return singleton;
   }

   /** Make new time event */
   ProfileTimingEvent *makeTimingEvent(const std::string &txt) { 
       ProfileTimingEvent *e = new ProfileTimingEvent(txt); 
       std::lock_guard<std::mutex> guard(eventMutex); 
       events.push_back(e);
       return e;
   }

   /** Make new quantity event */
   ProfileQuantityEvent *makeQuantityEvent(const std::string &txt, size_t n) { 
       ProfileQuantityEvent *e = new ProfileQuantityEvent(txt,n); 
       std::lock_guard<std::mutex> guard(eventMutex); 
       events.push_back(e);
       return e;
   }

   /** Dump all events */
   void dump(std::ostream &os) { 
       for(auto const &cur : events) { 
           cur->print(os);
       }
   }
};

}
