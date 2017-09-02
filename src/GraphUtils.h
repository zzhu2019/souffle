/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file GraphUtils.h
 *
 * A simple utility graph for conducting simple, graph-based operations.
 *
 ***********************************************************************/

#pragma once

#include <map>
#include <ostream>
#include <set>

namespace souffle {

/**
 * A simple graph structure for graph-based operations.
 */
template <typename Vertex, typename Compare = std::less<Vertex>>
class Graph {
    // not a very efficient but simple graph representation
    std::set<Vertex, Compare> vertices;                     // all the vertices in the graph
    std::map<Vertex, std::set<Vertex, Compare>> forward;   // all edges forward directed
    std::map<Vertex, std::set<Vertex, Compare>> backward;  // all edges backward

public:
    /**
     * Adds a new edge from the given vertex to the target vertex.
     */
    void insert(const Vertex& from, const Vertex& to) {
        insert(from);
        insert(to);
        forward[from].insert(to);
        backward[to].insert(from);
    }

    /**
     * Adds a vertex.
     */
    void insert(const Vertex& vertex) {
        auto iter = vertices.insert(vertex);
        if (iter.second) {
            forward.insert(std::make_pair(vertex, std::set<Vertex, Compare>()));
            backward.insert(std::make_pair(vertex, std::set<Vertex, Compare>()));
        }
    }

    /** Obtains a reference to the set of all vertices */
    const std::set<Vertex, Compare>& vertices() const {
        return vertices;
    }

    /** Returns the set of vertices the given vertex has edges to */
    const std::set<Vertex, Compare>& successors(const Vertex& from) const {
        assert(contains(from));
        return forward.find(from)->second;
    }

    /** Returns the set of vertices the given vertex has edges from */
    const std::set<Vertex, Compare>& predecessors(const Vertex& to) const {
        assert(contains(to));
        return backward.find(to)->second;
    }

    /** Determines whether the given vertex is present */
    bool contains(const Vertex &vertex) const {
        return vertices.find(vertex) != vertices.end();
    }

    /** Determines whether the given edge is present */
    bool contains(const Vertex &from, const Vertex &to) const {
        auto pos = forward.find(from);
        if (pos == forward.end()) {
            return false;
        }
        auto p2 = pos->second.find(to);
        return p2 != pos->second.end();
    }

    /** Determines whether there is a directed path between the two vertices */
    bool reaches(const Vertex& from, const Vertex& to) const {
        // quick check
        if (!contains(from) || !contains(to)) {
            return false;
        }

        // conduct a depth-first search starting at from
        bool found = false;
        bool first = true;
        visitDepthFirst(from, [&](const Vertex& cur) {
            found = !first && (found || cur == to);
            first = false;
        });
        return found;
    }

    /** Obtains the set of all vertices in the same clique than the given vertex */
    const std::set<Vertex, Compare> clique(const Vertex& vertex) const {
        std::set<Vertex, Compare> res;
        res.insert(vertex);
        for (const auto& cur : vertices()) {
            if (reaches(vertex, cur) && reaches(cur, vertex)) {
                res.insert(cur);
            }
        }
        return res;
    }

    /** A generic utility for depth-first visits */
    template <typename Lambda>
    void visitDepthFirst(const Vertex& vertex, const Lambda& lambda) const {
        std::set<Vertex, Compare> visited;
        visitDepthFirst(vertex, lambda, visited);
    }

    /** Enables graphs to be printed (e.g. for debugging) */
    void print(std::ostream& out) const {
        bool first = true;
        out << "{";
        for (const auto& cur : forward) {
            for (const auto& trg : cur.second) {
                if (!first) {
                    out << ",";
                }
                out << cur.first << "->" << trg;
                first = false;
            }
        }
        out << "}";
    }

    friend std::ostream& operator<<(std::ostream& out, const Graph& g) {
        g.print(out);
        return out;
    }

private:
    /** The internal implementation of depth-first visits */
    template <typename Lambda>
    void visitDepthFirst(const Vertex& vertex, const Lambda& lambda, std::set<Vertex, Compare>& visited) const {
        lambda(vertex);
        auto pos = forward.find(vertex);
        if (pos == forward.end()) {
            return;
        }
        for (const auto& cur : pos->second) {
            if (visited.insert(cur).second) {
                visitDepthFirst(cur, lambda, visited);
            }
        }
    }
};

}  // end of namespace souffle
