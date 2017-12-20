/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RamInterpreter.h
 *
 * Declares the interpreter class for executing RAM programs.
 *
 ***********************************************************************/

#pragma once

#include "RamData.h"
#include "RamProgram.h"
#include "RamRelation.h"
#include "SymbolTable.h"

#include "RamStatement.h"

#include <ostream>
#include <vector>
#include <functional>

namespace souffle {


class InterpreterRelation {
    static const int BLOCK_SIZE = 1024;

    struct Block {
        size_t size;
        size_t used;
        std::unique_ptr<Block> next;
        std::unique_ptr<RamDomain[]> data;

        Block(size_t s = BLOCK_SIZE) : size(s), used(0), next(nullptr), data(new RamDomain[size]) {}

        size_t getFreeSpace() const {
            return size - used;
        }
    };

    /** The name / arity of this relation */
    RamRelation id;

    size_t num_tuples;

    std::unique_ptr<Block> head;
    Block* tail;

    /** keep an eye on the implicit tuples we create so that we can remove when dtor is called */
    std::list<RamDomain*> allocatedBlocks;

    mutable std::map<RamIndexOrder, std::unique_ptr<RamIndex>> indices;

    mutable RamIndex* totalIndex;

    /** lock for parallel execution */
    mutable Lock lock;

public:
    InterpreterRelation(const RamRelation& id)
            : id(id), num_tuples(0), head(std::unique_ptr<Block>(new Block())), tail(head.get()),
              totalIndex(nullptr) {}

    InterpreterRelation(const InterpreterRelation& other) = delete;

    InterpreterRelation(InterpreterRelation&& other)
            : id(std::move(other.id)), num_tuples(other.num_tuples), tail(other.tail),
              totalIndex(other.totalIndex) {
        // take over ownership
        head.swap(other.head);
        indices.swap(other.indices);

        allocatedBlocks.swap(other.allocatedBlocks);
    }

    ~InterpreterRelation() {
        for (auto x : allocatedBlocks) delete[] x;
    }

    InterpreterRelation& operator=(const InterpreterRelation& other) = delete;

    InterpreterRelation& operator=(InterpreterRelation&& other) {
        ASSERT(getArity() == other.getArity());

        id = other.id;
        num_tuples = other.num_tuples;
        tail = other.tail;
        totalIndex = other.totalIndex;

        // take over ownership
        head.swap(other.head);
        indices.swap(other.indices);

        return *this;
    }

    /** Obtains the identifier of this relation */
    const RamRelation& getID() const {
        return id;
    }

    /** get name of relation */
    const std::string& getName() const {
        return id.getName();
    }

    /** get arity of relation */
    size_t getArity() const {
        return id.getArity();
    }

    /** determines whether this relation is empty */
    bool empty() const {
        return num_tuples == 0;
    }

    /** Gets the number of contained tuples */
    size_t size() const {
        return num_tuples;
    }

    /** only insert exactly one tuple, maintaining order **/
    void quickInsert(const RamDomain* tuple) {
        // check for null-arity
        auto arity = getArity();
        if (arity == 0) {
            // set number of tuples to one -- that's it
            num_tuples = 1;
            return;
        }

        // make existence check
        if (exists(tuple)) {
            return;
        }

        // prepare tail
        if (tail->getFreeSpace() < arity || arity == 0) {
            tail->next = std::unique_ptr<Block>(new Block());
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

    void extend(const InterpreterRelation& rel) {
        if (!id.isEqRel()) {
            return;
        }
        // TODO: future optimisation would require this as a member datatype
        // brave soul required to pass this quest
        // // specialisation for eqrel defs
        // std::unique_ptr<binaryrelation> eqreltuples;
        // in addition, it requires insert functions to insert into that, and functions
        // which allow reading of stored values must be changed to accommodate.
        // e.g. insert =>  eqRelTuples->insert(tuple[0], tuple[1]);

        // for now, we just have a naive & extremely slow version, otherwise known as a O(n^2) insertion
        // ):

        std::list<RamDomain*> dtorLooks;
        // store all values that will be implicitly relevant to the two that we will insert
        for (const auto* tuple : rel) {
            std::vector<const RamDomain*> relevantStored;
            for (const RamDomain* vals : *this) {
                if (vals[0] == tuple[0] || vals[0] == tuple[1] || vals[1] == tuple[0] ||
                        vals[1] == tuple[1]) {
                    relevantStored.push_back(vals);
                }
            }

            for (const auto vals : relevantStored) {
                // insert all possible pairings between these and existing elements
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[0];
                dtorLooks.back()[1] = tuple[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[0];
                dtorLooks.back()[1] = tuple[1];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[1];
                dtorLooks.back()[1] = tuple[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[1];
                dtorLooks.back()[1] = tuple[1];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[0];
                dtorLooks.back()[1] = vals[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[0];
                dtorLooks.back()[1] = vals[1];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[1];
                dtorLooks.back()[1] = vals[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[1];
                dtorLooks.back()[1] = vals[1];
            }

            // and of course we need to actually insert this pair
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[1];
            dtorLooks.back()[1] = tuple[0];
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[0];
            dtorLooks.back()[1] = tuple[1];
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[0];
            dtorLooks.back()[1] = tuple[0];
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[1];
            dtorLooks.back()[1] = tuple[1];
        }
        for (const auto* x : dtorLooks) {
            quickInsert(x);
            delete x;
        }
    }
    /** insert a new tuple to table, possibly more than one tuple depending on relation type */
    void insert(const RamDomain* tuple) {
        // TODO: (pnappa) an eqrel check here is all that appears to be needed for implicit additions
        if (id.isEqRel()) {
            // TODO: future optimisation would require this as a member datatype
            // brave soul required to pass this quest
            // // specialisation for eqrel defs
            // std::unique_ptr<binaryrelation> eqreltuples;
            // in addition, it requires insert functions to insert into that, and functions
            // which allow reading of stored values must be changed to accommodate.
            // e.g. insert =>  eqRelTuples->insert(tuple[0], tuple[1]);

            // for now, we just have a naive & extremely slow version, otherwise known as a O(n^2) insertion
            // ):

            // store all values that will be implicitly relevant to the two that we will insert
            std::vector<const RamDomain*> relevantStored;
            for (const RamDomain* vals : *this) {
                if (vals[0] == tuple[0] || vals[0] == tuple[1] || vals[1] == tuple[0] ||
                        vals[1] == tuple[1]) {
                    relevantStored.push_back(vals);
                }
            }

            // we also need to keep a list of all tuples stored s.t. we can free on destruction
            std::list<RamDomain*> dtorLooks;

            for (const auto vals : relevantStored) {
                // insert all possible pairings between these and existing elements

                // ew, temp code
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[0];
                dtorLooks.back()[1] = tuple[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[0];
                dtorLooks.back()[1] = tuple[1];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[1];
                dtorLooks.back()[1] = tuple[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = vals[1];
                dtorLooks.back()[1] = tuple[1];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[0];
                dtorLooks.back()[1] = vals[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[0];
                dtorLooks.back()[1] = vals[1];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[1];
                dtorLooks.back()[1] = vals[0];
                dtorLooks.push_back(new RamDomain[2]);
                dtorLooks.back()[0] = tuple[1];
                dtorLooks.back()[1] = vals[1];
            }

            // and of course we need to actually insert this pair
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[1];
            dtorLooks.back()[1] = tuple[0];
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[0];
            dtorLooks.back()[1] = tuple[1];
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[0];
            dtorLooks.back()[1] = tuple[0];
            dtorLooks.push_back(new RamDomain[2]);
            dtorLooks.back()[0] = tuple[1];
            dtorLooks.back()[1] = tuple[1];

            for (const auto x : dtorLooks) quickInsert(x);

            allocatedBlocks.insert(allocatedBlocks.end(), dtorLooks.begin(), dtorLooks.end());

        } else {
            quickInsert(tuple);
        }
    }

    /** a convenience function for inserting tuples */
    template <typename... Args>
    void insert(RamDomain first, Args... rest) {
        RamDomain tuple[] = {first, RamDomain(rest)...};
        insert(tuple);
    }

    /** Merges all elements of the given relation into this relation */
    void insert(const InterpreterRelation& other) {
        assert(getArity() == other.getArity());
        for (const auto& cur : other) {
            insert(cur);
        }
    }

    /** purge table */
    void purge() {
        std::unique_ptr<Block> newHead(new Block());
        head.swap(newHead);
        tail = head.get();
        for (const auto& cur : indices) {
            cur.second->purge();
        }
        num_tuples = 0;
    }

    /** get index for a given set of keys using a cached index as a helper. Keys are encoded as bits for each
     * column */
    RamIndex* getIndex(const SearchColumns& key, RamIndex* cachedIndex) const {
        if (!cachedIndex) {
            return getIndex(key);
        }
        return getIndex(cachedIndex->order());
    }

    /** get index for a given set of keys. Keys are encoded as bits for each column */
    RamIndex* getIndex(const SearchColumns& key) const {
        // suffix for order, if no matching prefix exists
        std::vector<unsigned char> suffix;
        suffix.reserve(getArity());

        // convert to order
        RamIndexOrder order;
        for (size_t k = 1, i = 0; i < getArity(); i++, k *= 2) {
            if (key & k) {
                order.append(i);
            } else {
                suffix.push_back(i);
            }
        }

        // see whether there is an order with a matching prefix
        RamIndex* res = nullptr;
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
    RamIndex* getIndex(const RamIndexOrder& order) const {
        // TODO: improve index usage by re-using indices with common prefix
        RamIndex* res = nullptr;
        {
            auto lease = lock.acquire();
            (void)lease;
            auto pos = indices.find(order);
            if (pos == indices.end()) {
                std::unique_ptr<RamIndex>& newIndex = indices[order];
                newIndex = std::make_unique<RamIndex>(order);
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
};

/**
 * An environment encapsulates all the context information required for
 * processing a RAM program.
 */
class InterpreterEnvironment {
    /** The type utilized for storing relations */
    typedef std::map<std::string, InterpreterRelation> relation_map;

    /** The symbol table to be utilized by an evaluation */
    SymbolTable& symbolTable;

    /** The relations manipulated by a ram program */
    relation_map data;

    /** The increment counter utilized by some RAM language constructs */
    int counter;

public:
    InterpreterEnvironment(SymbolTable& symbolTable) : symbolTable(symbolTable), counter(0) {}

    /**
     * Obtains a reference to the enclosed symbol table.
     */
    SymbolTable& getSymbolTable() {
        return symbolTable;
    }

    /**
     * Obtains the current value of the internal counter.
     */
    int getCounter() const {
        return counter;
    }

    /**
     * Increments the internal counter and obtains the
     * old value.
     */
    int incCounter() {
        return counter++;
    }

    /**
     * Obtains a mutable reference to one of the relations maintained
     * by this environment. If the addressed relation does not exist,
     * a new, empty relation will be created.
     */
    InterpreterRelation& getRelation(const RamRelation& id) {

        InterpreterRelation* res = nullptr;
        auto pos = data.find(id.getName());
        if (pos != data.end()) {
            res = &(pos->second);
        } else {
            res = &(data.emplace(id.getName(), id).first->second);
        }

        // return result
        return *res;
    }

    /**
     * Obtains an immutable reference to the relation identified by
     * the given identifier. If no such relation exist, a reference
     * to an empty relation will be returned (not exhibiting the proper
     * id, but the correct content).
     */
    const InterpreterRelation& getRelation(const RamRelation& id) const {
        // look up relation
        auto pos = data.find(id.getName());
        assert(pos != data.end());

        // cache result
        return pos->second;
    }

    /**
     * Obtains an immutable reference to the relation identified by
     * the given identifier. If no such relation exist, a reference
     * to an empty relation will be returned (not exhibiting the proper
     * id, but the correct content).
     */
    const InterpreterRelation& getRelation(const std::string& name) const {
        auto pos = data.find(name);
        assert(pos != data.end());
        return pos->second;
    }

    /**
     * Returns the relation map
     */
    relation_map& getRelationMap() const {
        return const_cast<relation_map&>(data);
    }

    /**
     * Tests whether a relation with the given name is present.
     */
    bool hasRelation(const std::string& name) const {
        return data.find(name) != data.end();
    }

    /**
     * Deletes the referenced relation from this environment.
     */
    void dropRelation(const RamRelation& id) {
        data.erase(id.getName());
    }
};

/**
 * A class representing the order of predicates in the body of a rule
 */
class Order {
    /** The covered order */
    std::vector<unsigned> order;

public:
    static Order getIdentity(unsigned size) {
        Order res;
        for (unsigned i = 0; i < size; i++) {
            res.append(i);
        }
        return res;
    }

    void append(unsigned pos) {
        order.push_back(pos);
    }

    unsigned operator[](unsigned index) const {
        return order[index];
    }

    std::size_t size() const {
        return order.size();
    }

    bool isComplete() const {
        for (size_t i = 0; i < order.size(); i++) {
            if (!contains(order, i)) {
                return false;
            }
        }
        return true;
    }

    const std::vector<unsigned>& getOrder() const {
        return order;
    }

    void print(std::ostream& out) const {
        out << order;
    }

    friend std::ostream& operator<<(std::ostream& out, const Order& order) {
        order.print(out);
        return out;
    }
};

/**
 * The summary to be returned from a statement 
 */
struct ExecutionSummary {
    Order order;
    long time;
};

/** Defines the type of execution strategies for interpreter */
typedef std::function<ExecutionSummary(const RamInsert&, InterpreterEnvironment& env, std::ostream*)>
        QueryExecutionStrategy;

/** With this strategy queries will be processed without profiling */
extern const QueryExecutionStrategy DirectExecution;

/** With this strategy queries will be dynamically with profiling */
extern const QueryExecutionStrategy ScheduledExecution;

/**
 * A RAM interpreter. The RAM program will
 * be processed within the callers process. Before every query operation, an
 * optional scheduling step will be conducted.
 */
class RamInterpreter {
protected:
    /** An optional stream to print logging information to an output stream */
    std::ostream* report;

public:
    /**
     * Update logging stream
     */
    void setReportTarget(std::ostream& report) {
        this->report = &report;
    }

    /**
     * Runs the given RAM statement on an empty environment and returns
     * this environment after the completion of the execution.
     */
    std::unique_ptr<InterpreterEnvironment> execute(SymbolTable& table, const RamProgram& prog) const {
        auto env = std::make_unique<InterpreterEnvironment>(table);
        applyOn(prog, *env, nullptr);
        return env;
    }

    /**
     * Runs the given RAM statement on an empty environment and input data and returns
     * this environment after the completion of the execution.
     */
    std::unique_ptr<InterpreterEnvironment> execute(SymbolTable& table, const RamProgram& prog, RamData* data) const {
        // Ram env managed by the interface
        auto env = std::make_unique<InterpreterEnvironment>(table);
        applyOn(prog, *env, data);
        return env;
    }

    /** An execution strategy for the interpreter */
    QueryExecutionStrategy queryStrategy;

public:
    /** A constructor accepting a query strategy */
    RamInterpreter(const QueryExecutionStrategy& queryStrategy) : report(nullptr), queryStrategy(queryStrategy) {}

    /**
     * The implementation of the interpreter applying the given program
     * on the given environment.
     */
    void applyOn(const RamProgram& prog, InterpreterEnvironment& env, RamData* data) const;

    /**
     * Runs a subroutine of a RamProgram
     */
    virtual void executeSubroutine(InterpreterEnvironment& env, const RamStatement& stmt,
            const std::vector<RamDomain>& arguments, std::vector<RamDomain>& returnValues,
            std::vector<bool>& returnErrors) const;
};

}  // end of namespace souffle
