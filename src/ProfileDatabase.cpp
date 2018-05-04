#include "ProfileDatabase.h"
#include "Util.h"
#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace souffle {

/**
 * DirectoryEntry method implementations
 */

// get keys
const std::set<std::string> DirectoryEntry::getKeys() const {
    std::set<std::string> result;
    for (auto const& cur : entries) {
        result.insert(cur.first);
    }
    return result;
}

// write entry
Entry* DirectoryEntry::writeEntry(std::unique_ptr<Entry> entry) {
    assert(entry != nullptr && "null entry");
    const std::string& key = entry->getKey();
    entries[key] = std::move(entry);
    return readEntry(key);
}

// read entry
Entry* DirectoryEntry::readEntry(const std::string& key) const {
    auto it = entries.find(key);
    if (it != entries.end()) {
        return (*it).second.get();
    } else {
        return nullptr;
    }
}

// read directory
DirectoryEntry* DirectoryEntry::readDirectoryEntry(const std::string& key) const {
    return dynamic_cast<DirectoryEntry*>(readEntry(key));
}

// accept visitor
void DirectoryEntry::accept(Visitor& v) {
    v.visit(*this);
}

// print directory
void DirectoryEntry::print(std::ostream& os, int tabpos) {
    os << std::string(tabpos, ' ') << getKey() << ":" << std::endl;
    for (auto const& cur : entries) {
        cur.second->print(os, tabpos + 1);
    }
}

/**
 * SizeEntry method implementations
 */

// accept visitor
void SizeEntry::accept(Visitor& v) {
    v.visit(*this);
}

// print entry
void SizeEntry::print(std::ostream& os, int tabpos) {
    os << std::string(tabpos, ' ') << getKey() << "->" << size << std::endl;
}

/**
 * TextEntry method implementations
 */

// accept visitor
void TextEntry::accept(Visitor& v) {
    v.visit(*this);
}

// write size entry
void TextEntry::print(std::ostream& os, int tabpos) {
    os << std::string(tabpos, ' ') << getKey() << "->" << text << std::endl;
}

/**
 * Duration Entry method implementations
 */

// accept visitor
void DurationEntry::accept(Visitor& v) {
    v.visit(*this);
}

// write size entry
void DurationEntry::print(std::ostream& os, int tabpos) {
    os << std::string(tabpos, ' ') << getKey() << "->[]" << std::endl;
}

/**
 * Hierarchical database
 */

// find path (if it does not exist -> create directory path)
DirectoryEntry* ProfileDatabase::lookupPath(std::vector<std::string> path) {
    DirectoryEntry* dir = root.get();
    for (const std::string& key : path) {
        DirectoryEntry* newDir = dir->readDirectoryEntry(key);
        if (newDir == nullptr) {
            newDir = dynamic_cast<DirectoryEntry*>(
                    dir->writeEntry(std::unique_ptr<DirectoryEntry>(new DirectoryEntry(key))));
        }
        dir = newDir;
    }
    return dir;
}

// add size entry
void ProfileDatabase::addSizeEntry(std::vector<std::string> qualifier, size_t size) {
    assert(qualifier.size() > 0 && "no qualifier");
    std::vector<std::string> path(qualifier.begin(), qualifier.end() - 1);
    DirectoryEntry* dir = lookupPath(path);

    const std::string& key = qualifier.back();
    std::unique_ptr<SizeEntry> entry = std::make_unique<SizeEntry>(key, size);
    dir->writeEntry(std::move(entry));
}

// add text entry
void ProfileDatabase::addTextEntry(std::vector<std::string> qualifier, const std::string& text) {
    assert(qualifier.size() > 0 && "no qualifier");
    std::vector<std::string> path(qualifier.begin(), qualifier.end() - 1);
    DirectoryEntry* dir = lookupPath(path);

    const std::string& key = qualifier.back();
    std::unique_ptr<TextEntry> entry = std::make_unique<TextEntry>(key, text);
    dir->writeEntry(std::move(entry));
}

// add duration entry
void ProfileDatabase::addDurationEntry(std::vector<std::string> qualifier, time_point start, time_point end) {
    assert(qualifier.size() > 0 && "no qualifier");
    std::vector<std::string> path(qualifier.begin(), qualifier.end() - 1);
    DirectoryEntry* dir = lookupPath(path);

    const std::string& key = qualifier.back();
    std::unique_ptr<DurationEntry> entry = std::make_unique<DurationEntry>(key, start, end);
    dir->writeEntry(std::move(entry));
}

// compute sum
size_t ProfileDatabase::computeSum(const std::vector<std::string>& qualifier) {
    assert(qualifier.size() > 0 && "no qualifier");
    std::vector<std::string> path(qualifier.begin(), qualifier.end() - 1);
    DirectoryEntry* dir = lookupPath(path);

    const std::string& key = qualifier.back();
    std::cout << "Key: " << key << std::endl;
    Counter ctr(key);
    dir->accept(ctr);
    return ctr.getCounter();
}

// print database
void ProfileDatabase::print(std::ostream& os) {
    root->print(os, 0);
};
}  // namespace souffle

int main() {
    souffle::ProfileDatabase x;
    x.addSizeEntry({"akey"}, 1);
    x.addSizeEntry({"a", "b", "bkey"}, 2);
    x.addSizeEntry({"a", "c", "bkey"}, 3);
    x.addTextEntry({"a", "x", "akey"}, "blabla");
    x.print(std::cout);
    std::cout << "Sum of bkey:" << x.computeSum({"a", "bkey"});

    return 0;
}
