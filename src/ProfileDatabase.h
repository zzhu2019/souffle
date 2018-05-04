#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

using time_point = std::chrono::system_clock::time_point;

class Visitor;

/**
 * Entry class
 *
 * abstract class for a key/value entry in a hierarchical database
 */
class Entry {
private:
    // entry key
    std::string key;

public:
    Entry(std::string key) : key(std::move(key)) {}
    virtual ~Entry() = default;

    // get key
    std::string& getKey() {
        return key;
    };

    // accept visitor
    virtual void accept(Visitor& v) = 0;

    // print
    virtual void print(std::ostream& os, int tabpos) = 0;
};

/**
 * DirectoryEntry entry
 */
class DirectoryEntry : public Entry {
private:
    std::map<std::string, std::unique_ptr<Entry>> entries;

public:
    DirectoryEntry(const std::string& name) : Entry(name) {}

    // get keys in directory
    const std::set<std::string> getKeys() const;

    // write entry
    Entry* writeEntry(std::unique_ptr<Entry> entry);

    // read entry
    Entry* readEntry(const std::string& key) const;

    // read directory
    DirectoryEntry* readDirectoryEntry(const std::string& key) const;

    // accept visitor
    void accept(Visitor& v) override;

    // print directory
    void print(std::ostream& os, int tabpos) override;
};

/**
 * SizeEntry
 */
class SizeEntry : public Entry {
private:
    size_t size;  // size
public:
    SizeEntry(const std::string& key, size_t size) : Entry(key), size(size) {}

    // get size
    size_t getSize() const {
        return size;
    }

    // accept visitor
    void accept(Visitor& v) override;

    // print size entry
    void print(std::ostream& os, int tabpos) override;
};

/**
 * TextEntry
 */
class TextEntry : public Entry {
private:
    // entry text
    std::string text;

public:
    TextEntry(const std::string& key, std::string text) : Entry(key), text(std::move(text)) {}

    // get text
    const std::string& getText() const {
        return text;
    }

    // accept visitor
    void accept(Visitor& v) override;

    // print
    void print(std::ostream& os, int tabpos) override;
};

/**
 * Duration Entry
 */
class DurationEntry : public Entry {
private:
    // duration start
    time_point start;

    // duration end
    time_point end;

public:
    DurationEntry(const std::string& key, time_point start, time_point end)
            : Entry(key), start(start), end(end) {}

    // get start
    time_point getStart() const {
        return start;
    }

    // get end
    time_point getEnd() const {
        return end;
    }

    // accept visitor
    void accept(Visitor& v) override;

    // write size entry
    void print(std::ostream& os, int tabpos) override;
};

/**
 * Hierarchical databas
 */
class ProfileDatabase {
private:
    std::unique_ptr<DirectoryEntry> root;

protected:
    /**
     * Find path: if directories along the path do not exist, create them.
     */
    DirectoryEntry* lookupPath(std::vector<std::string> path);

public:
    ProfileDatabase() : root(std::unique_ptr<DirectoryEntry>(new DirectoryEntry("root"))) {}

    // add size entry
    void addSizeEntry(std::vector<std::string> qualifier, size_t size);

    // add text entry
    void addTextEntry(std::vector<std::string> qualifier, const std::string& text);

    // add duration entry
    void addDurationEntry(std::vector<std::string> qualifier, time_point start, time_point end);

    // query sum of a key
    size_t computeSum(const std::vector<std::string>& qualifier);

    // print database
    void print(std::ostream& os);
};

/**
 * Visitor Interface
 */
class Visitor {
public:
    virtual ~Visitor() = default;

    // visit entries in a directory
    virtual void visit(DirectoryEntry& e) {
        std::cout << "Dir " << e.getKey() << "\n";
        for (const auto& cur : e.getKeys()) {
            std::cout << "\t :" << cur << "\n";
            e.readEntry(cur)->accept(*this);
        }
    }

    // visit entries
    virtual void visit(SizeEntry& e) {}
    virtual void visit(TextEntry& e) {}
    virtual void visit(DurationEntry& e) {}
};

class Counter : public Visitor {
private:
    size_t ctr{0};
    std::string key;

public:
    Counter(std::string key) : key(std::move(key)) {}
    void visit(SizeEntry& e) override {
        std::cout << "Size entry : " << e.getKey() << " " << e.getSize() << "\n";
        if (e.getKey() == key) {
            ctr += e.getSize();
        }
    }
    size_t getCounter() const {
        return ctr;
    }
};
} //namespace souffle
