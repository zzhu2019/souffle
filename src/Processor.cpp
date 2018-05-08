#include <iostream>
#include <map>
#include <cassert>

/**
 * Abstract Class for Processor
 */ 
class Processor {
public:
 virtual ~Processor() { }
 virtual void process() = 0;
};

/**
 * Processor Singleton
 */ 
class ProcessorSingleton {
 /** store various processors */ 
 std::map<std::string, Processor *> m_registry;
 ProcessorSingleton() : m_registry() {
     std::cout << "Singleton is happening\n";
 }
public:
 static ProcessorSingleton &instance() {
  static ProcessorSingleton singleton;
  return singleton;
 }
 void registerProcessor(const std::string &name, Processor *processor) {
  m_registry[name]=processor;
 }
 void process(const std::string &name) {  
  assert(m_registry.find(name) != m_registry.end() && "Processor not found!");
  m_registry[name]->process();
 }
};

/**
 * Processor A
 */ 
class ProcessorA : public Processor {
public:
 ProcessorA() { 
  std::cout << "Register Processor A\n";
  ProcessorSingleton::instance().registerProcessor("A",this); 
 }
 void process() override {
  std::cout << "Process A\n";
 }
} pA;


/**
 * Processor B
 */ 
class ProcessorB : public Processor {
public:
 ProcessorB() {
  std::cout << "Register Processor B\n";
  ProcessorSingleton::instance().registerProcessor("B",this); 
 }
 void process() override {
  std::cout << "Process B\n";
 }
} pB;

int main() 
{
 ProcessorSingleton::instance().process("A");
 ProcessorSingleton::instance().process("B");
 ProcessorSingleton::instance().process("C");
}

