/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file InterpreterRelation.h
 *
 * Defines Interpreter Relations
 *
 ***********************************************************************/

#pragma once

#include "InterpreterIndex.h"
#include "ParallelUtils.h"
#include "RamTypes.h"

#include <list>
#include <memory>
#include <vector>

namespace souffle {

/**
 * Interpreter Relation
 */
class InterpreterRelation {
private:
    /** Arity of relation */
    size_t arity;

    /** Size of blocks containing tuples */
    static const int BLOCK_SIZE = 1024;

    /** Block data structure for storing tuples */
    struct Block {
        size_t size;
        size_t used;
        // TODO (#541): replace linked list by STL linked list
        // block becomes payload of STL linked list only
        std::unique_ptr<Block> next;
        std::unique_ptr<RamDomain[]> data;

        Block(size_t s = BLOCK_SIZE) : size(s), used(0), next(nullptr), data(new RamDomain[size]) {}

        size_t getFreeSpace() const {
            return size - used;
        }
    };

    /** Number of tuples in relation */
    size_t num_tuples;

    /** Head of block list */
    std::unique_ptr<Block> head;

    /** Tail of block list */
    Block* tail;

    /** List of all allocated blocks */
    std::list<RamDomain*> allocatedBlocks;

    /** List of indices */
    mutable std::map<InterpreterIndexOrder, std::unique_ptr<InterpreterIndex>> indices;

    /** Total index for existence checks */
    mutable InterpreterIndex* totalIndex;

    /** Lock for parallel execution */
    mutable Lock lock;

public:
    InterpreterRelation(size_t relArity)
            : arity(relArity), num_tuples(0), head(std::make_unique<Block>()), tail(head.get()),
              totalIndex(nullptr) {}

    InterpreterRelation(const InterpreterRelation& other) = delete;

    InterpreterRelation(InterpreterRelation&& other)
            : arity(other.arity), num_tuples(other.num_tuples), tail(other.tail),
              totalIndex(other.totalIndex) {
        // take over ownership
        head.swap(other.head);
        indices.swap(other.indices);

        allocatedBlocks.swap(other.allocatedBlocks);
    }

    virtual ~InterpreterRelation() {
        for (auto x : allocatedBlocks) delete[] x;
    }

    // TODO (#421): check whether still required
    InterpreterRelation& operator=(const InterpreterRelation& other) = delete;

    InterpreterRelation& operator=(InterpreterRelation&& other) {
        ASSERT(getArity() == other.getArity());

        num_tuples = other.num_tuples;
        tail = other.tail;
        totalIndex = other.totalIndex;

        // take over ownership
        head.swap(other.head);
        indices.swap(other.indices);

        return *this;
    }

    /** Get arity of relation */
    size_t getArity() const {
        return arity;
    }

    /** Check whether relation is empty */
    bool empty() const {
        return num_tuples == 0;
    }

    /** Gets the number of contained tuples */
    size_t size() const {
        return num_tuples;
    }

    /** Insert tuple */
    virtual void insert(const RamDomain* tuple) {
        // check for null-arity
        if (arity == 0) {
            // set number of tuples to one -- that's it
            num_tuples = 1;
            return;
        }

        ASSERT(tuple);

        // make existence check
        if (exists(tuple)) {
            return;
        }

        // prepare tail
        if (tail->getFreeSpace() < arity || arity == 0) {
            tail->next = std::make_unique<Block>();
            tail = tail->next.get();
        }

        // insert element into tail
        RamDomain* newTuple = &tail->data[tail->used];
        for (size_t i = 0; i < arity; ++i) {
            newTuple[i] = tuple[i];
        }
        tail->used += arity;

        // update all indexes with new tuple
        for (const auto& cur : indices) {
            cur.second->insert(newTuple);
        }

        // increment relation size
        num_tuples++;
    }

    /** Insert tuple via arguments */
    template <typename... Args>
    void insert(RamDomain first, Args... rest) {
        RamDomain tuple[] = {first, RamDomain(rest)...};
        insert(tuple);
    }

    /** Merge another relation into this relation */
    void insert(const InterpreterRelation& other) {
        assert(getArity() == other.getArity());
        for (const auto& cur : other) {
            insert(cur);
        }
    }

    /** Purge table */
    void purge() {
        std::unique_ptr<Block> newHead = std::make_unique<Block>();
        head.swap(newHead);
        tail = head.get();
        for (const auto& cur : indices) {
            cur.second->purge();
        }
        num_tuples = 0;
    }

    /** get index for a given set of keys using a cached index as a helper. Keys are encoded as bits for each
     * column */
    InterpreterIndex* getIndex(const SearchColumns& key, InterpreterIndex* cachedIndex) const {
        if (!cachedIndex) {
            return getIndex(key);
        }
        return getIndex(cachedIndex->order());
    }

    /** get index for a given set of keys. Keys are encoded as bits for each column */
    InterpreterIndex* getIndex(const SearchColumns& key) const {
        // suffix for order, if no matching prefix exists
        std::vector<unsigned char> suffix;
        suffix.reserve(getArity());

        // convert to order
        InterpreterIndexOrder order;
        for (size_t k = 1, i = 0; i < getArity(); i++, k *= 2) {
            if (key & k) {
                order.append(i);
            } else {
                suffix.push_back(i);
            }
        }

        // see whether there is an order with a matching prefix
        InterpreterIndex* res = nullptr;
        {
            auto lease = lock.acquire();
            (void)lease;
            for (auto it = indices.begin(); !res && it != indices.end(); ++it) {
                if (order.isCompatible(it->first)) {
                    res = it->second.get();
                }
            }
        }
        // if found, use compatible index
        if (res) {
            return res;
        }

        // extend index to full index
        for (auto cur : suffix) {
            order.append(cur);
        }
        assert(order.isComplete());

        // get a new index
        return getIndex(order);
    }

    /** get index for a given order. Keys are encoded as bits for each column */
    InterpreterIndex* getIndex(const InterpreterIndexOrder& order) const {
        // TODO: improve index usage by re-using indices with common prefix
        InterpreterIndex* res = nullptr;
        {
            auto lease = lock.acquire();
            (void)lease;
            auto pos = indices.find(order);
            if (pos == indices.end()) {
                std::unique_ptr<InterpreterIndex>& newIndex = indices[order];
                newIndex = std::make_unique<InterpreterIndex>(order);
                newIndex->insert(this->begin(), this->end());
                res = newIndex.get();
            } else {
                res = pos->second.get();
            }
        }
        return res;
    }

    /** Obtains a full index-key for this relation */
    SearchColumns getTotalIndexKey() const {
        return (1 << (getArity())) - 1;
    }

    /** check whether a tuple exists in the relation */
    bool exists(const RamDomain* tuple) const {
        // handle arity 0
        if (getArity() == 0) {
            return !empty();
        }

        // handle all other arities
        if (!totalIndex) {
            totalIndex = getIndex(getTotalIndexKey());
        }
        return totalIndex->exists(tuple);
    }

    // --- iterator ---

    /** Iterator for relation */
    class iterator : public std::iterator<std::forward_iterator_tag, RamDomain*> {
        Block* cur;
        RamDomain* tuple;
        size_t arity;

    public:
        iterator() : cur(nullptr), tuple(nullptr), arity(0) {}

        iterator(Block* c, RamDomain* t, size_t a) : cur(c), tuple(t), arity(a) {}

        const RamDomain* operator*() {
            return tuple;
        }

        bool operator==(const iterator& other) const {
            return tuple == other.tuple;
        }

        bool operator!=(const iterator& other) const {
            return (tuple != other.tuple);
        }

        iterator& operator++() {
            // check for end
            if (!cur) {
                return *this;
            }

            // support 0-arity
            if (arity == 0) {
                // move to end
                *this = iterator();
                return *this;
            }

            // support all other arities
            tuple += arity;
            if (tuple >= &cur->data[cur->used]) {
                cur = cur->next.get();
                tuple = (cur) ? cur->data.get() : nullptr;
            }
            return *this;
        }
    };

    /** get iterator begin of relation */
    inline iterator begin() const {
        // check for emptiness
        if (empty()) {
            return end();
        }

        // support 0-arity
        auto arity = getArity();
        if (arity == 0) {
            Block dummyBlock;
            RamDomain dummyTuple;
            return iterator(&dummyBlock, &dummyTuple, 0);
        }

        // support non-empty non-zero arity relation
        return iterator(head.get(), &head->data[0], arity);
    }

    /** get iterator begin of relation */
    inline iterator end() const {
        return iterator();
    }

    /** Extend tuple */
    virtual std::vector<RamDomain*> extend(const RamDomain* tuple) {
        std::vector<RamDomain*> newTuples;

        // A standard relation does not generate extra new knowledge on insertion.
        newTuples.push_back(new RamDomain[2]{tuple[0], tuple[1]});

        return newTuples;
    }

    /** Extend relation */
    virtual void extend(const InterpreterRelation& rel) {}
};

/**
 * Interpreter Equivalence Relation
 */

class InterpreterEqRelation : public InterpreterRelation {
public:
    InterpreterEqRelation(size_t relArity) : InterpreterRelation(relArity) {}

    /** Insert tuple */
    void insert(const RamDomain* tuple) override {
        // TODO: (pnappa) an eqrel check here is all that appears to be needed for implicit additions
        // TODO: future optimisation would require this as a member datatype
        // brave soul required to pass this quest
        // // specialisation for eqrel defs
        // std::unique_ptr<binaryrelation> eqreltuples;
        // in addition, it requires insert functions to insert into that, and functions
        // which allow reading of stored values must be changed to accommodate.
        // e.g. insert =>  eqRelTuples->insert(tuple[0], tuple[1]);

        // for now, we just have a naive & extremely slow version, otherwise known as a O(n^2) insertion
        // ):

        for (auto* newTuple : extend(tuple)) {
            InterpreterRelation::insert(newTuple);
            delete[] newTuple;
        }
    }

    /** Find the new knowledge generated by inserting a tuple */
    std::vector<RamDomain*> extend(const RamDomain* tuple) override {
        std::vector<RamDomain*> newTuples;

        newTuples.push_back(new RamDomain[2]{tuple[0], tuple[0]});
        newTuples.push_back(new RamDomain[2]{tuple[0], tuple[1]});
        newTuples.push_back(new RamDomain[2]{tuple[1], tuple[0]});
        newTuples.push_back(new RamDomain[2]{tuple[1], tuple[1]});

        std::vector<const RamDomain*> relevantStored;
        for (const RamDomain* vals : *this) {
            if (vals[0] == tuple[0] || vals[0] == tuple[1] || vals[1] == tuple[0] || vals[1] == tuple[1]) {
                relevantStored.push_back(vals);
            }
        }

        for (const auto vals : relevantStored) {
            newTuples.push_back(new RamDomain[2]{vals[0], tuple[0]});
            newTuples.push_back(new RamDomain[2]{vals[0], tuple[1]});
            newTuples.push_back(new RamDomain[2]{vals[1], tuple[0]});
            newTuples.push_back(new RamDomain[2]{vals[1], tuple[1]});
            newTuples.push_back(new RamDomain[2]{tuple[0], vals[0]});
            newTuples.push_back(new RamDomain[2]{tuple[0], vals[1]});
            newTuples.push_back(new RamDomain[2]{tuple[1], vals[0]});
            newTuples.push_back(new RamDomain[2]{tuple[1], vals[1]});
        }

        return newTuples;
    }
    /** Extend this relation with new knowledge generated by inserting all tuples from a relation */
    void extend(const InterpreterRelation& rel) override {
        std::vector<RamDomain*> newTuples;
        // store all values that will be implicitly relevant to the those that we will insert
        for (const auto* tuple : rel) {
            for (auto* newTuple : extend(tuple)) {
                newTuples.push_back(newTuple);
            }
        }
        for (const auto* newTuple : newTuples) {
            InterpreterRelation::insert(newTuple);
            delete[] newTuple;
        }
    }
};

}  // end of namespace souffle
