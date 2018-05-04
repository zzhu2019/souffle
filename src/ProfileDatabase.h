#pragma once

#include <assert.h>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

typedef std::chrono::system_clock::time_point time_point;

class Visitor;

/**
 * Entry class
 *
 * abstract class for a key/value entry in a hierarchical database
 */
class Entry {
private:
  // entry key
  std::string m_key;

public:
  Entry(const std::string &key) : m_key(key) {}
  virtual ~Entry(){};

  // get key
  std::string &getKey() { return m_key; };

  // accept visitor
  virtual void accept(Visitor &v) = 0;

  // print
  virtual void print(std::ostream &os, int tabpos) = 0;
};

/**
 * DirectoryEntry entry
 */
class DirectoryEntry : public Entry {
private:
  std::map<std::string, std::unique_ptr<Entry>> m_entries;

public:
  DirectoryEntry(const std::string &name) : Entry(name) {}

  // get keys in directory
  const std::set<std::string> getKeys() const;

  // write entry
  Entry *writeEntry(std::unique_ptr<Entry> entry);

  // read entry
  Entry *readEntry(const std::string &key) const;

  // read directory
  DirectoryEntry *readDirectoryEntry(const std::string &key) const;

  // accept visitor
  void accept(Visitor &v) override;

  // print directory
  void print(std::ostream &os, int tabpos) override;
};

/**
 * SizeEntry
 */
class SizeEntry : public Entry {
private:
  size_t m_size; // size
public:
  SizeEntry(const std::string &key, size_t size) : Entry(key), m_size(size) {}

  // get size
  size_t getSize() const { return m_size; }

  // accept visitor
  void accept(Visitor &v) override;

  // print size entry
  void print(std::ostream &os, int tabpos) override;
};

/**
 * TextEntry
 */
class TextEntry : public Entry {
private:
  // entry text
  std::string m_text;

public:
  TextEntry(const std::string &key, const std::string &text)
      : Entry(key), m_text(text) {}

  // get text
  const std::string &getText() const { return m_text; }

  // accept visitor
  void accept(Visitor &v) override;

  // print
  void print(std::ostream &os, int tabpos) override;
};

/**
 * Duration Entry
 */
class DurationEntry : public Entry {
private:
  // duration start
  time_point m_start;

  // duration end
  time_point m_end;

public:
  DurationEntry(const std::string &key, time_point start, time_point end)
      : Entry(key), m_start(start), m_end(end) {}

  // get start
  time_point getStart() const { return m_start; }

  // get end
  time_point getEnd() const { return m_end; }

  // accept visitor
  void accept(Visitor &v) override;

  // write size entry
  void print(std::ostream &os, int tabpos) override;
};

/**
 * Hierarchical databas
 */
class ProfileDatabase {
private:
  std::unique_ptr<DirectoryEntry> m_root;

protected:
  /**
   * Find path: if directories along the path do not exist, create them.
   */
  DirectoryEntry *lookupPath(std::vector<std::string> path);

public:
  ProfileDatabase()
      : m_root(std::unique_ptr<DirectoryEntry>(new DirectoryEntry("root"))) {}

  // add size entry
  void addSizeEntry(std::vector<std::string> qualifier, size_t size);

  // add text entry
  void addTextEntry(std::vector<std::string> qualifier,
                    const std::string &text);

  // add duration entry
  void addDurationEntry(std::vector<std::string> qualifier, time_point start,
                        time_point end);

  // query sum of a key
  size_t computeSum(const std::vector<std::string> &qualifier);

  // print database
  void print(std::ostream &os);
};

/**
 * Visitor Interface
 */
class Visitor {
public:
  virtual ~Visitor() { }

  // visit entries in a directory
  virtual void visit(DirectoryEntry &e) {
      std::cout << "Dir " << e.getKey() << "\n";
      for (const auto &cur: e.getKeys()) {
           std::cout << "\t :" << cur << "\n"; 
           e.readEntry(cur)->accept(*this);
      }
  }

  // visit entries
  virtual void visit(SizeEntry &e) {}
  virtual void visit(TextEntry &e) {}
  virtual void visit(DurationEntry &e) {}
};

class Counter : public Visitor { 
private:
   size_t m_ctr;
   std::string m_key;
public:
   Counter(const std::string &key) : m_key(key), m_ctr(0) { }
   void visit(SizeEntry &e) override {
       std::cout << "Size entry : " << e.getKey() << " " << e.getSize() << "\n";
       if (e.getKey() == m_key){
          m_ctr += e.getSize(); 
       } 
   }
   size_t getCounter() const { 
       return m_ctr;
   }
};
