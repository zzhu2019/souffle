#include <iostream>
#include <map>
#include <cassert>
#include <cstdarg>
#include <string>
#include <vector>

/**
 * Abstract Class for EventProcessor
 */ 
class EventProcessor {
public:
 virtual ~EventProcessor() { }
 virtual void process(const std::vector<std::string> &, va_list &) = 0;
};

/**
 * EventProcessor Singleton
 */ 
class EventProcessorSingleton {
 /** store various processors */ 
 std::map<std::string, EventProcessor *> m_registry;
 EventProcessorSingleton() : m_registry() {
     std::cout << "Singleton is happening\n";
 }
public:
 static EventProcessorSingleton &instance() {
  static EventProcessorSingleton singleton;
  return singleton;
 }
 void registerEventProcessor(const std::string &name, EventProcessor *processor) {
  m_registry[name]=processor;
 }

 /** process a profile event */ 
 void process(const char *txt, ...) {   
  va_list args;
  va_start(args, txt);
  std::vector<std::string> eventSignature;

  // do parsing here 
  eventSignature.push_back(txt);

  // invoke the event processor of the event
  const std::string &name = eventSignature[0];
  assert(eventSignature.size()>0 && "no keyword in event description");
  assert(m_registry.find(name) != m_registry.end() && "EventProcessor not found!");
  m_registry[name]->process(eventSignature, args);

  // terminate access to variadiac arguments 
  va_end(args);
 }
};

/**
 * EventProcessor A
 */ 
class EventProcessorA : public EventProcessor {
public:
 EventProcessorA() { 
  std::cout << "Register EventProcessor A\n";
  EventProcessorSingleton::instance().registerEventProcessor("A",this); 
 }
 void process(const std::vector<std::string> &, va_list &args) override {
  std::cout << "Process A\n";
  std::cout <<  va_arg(args,int) << "\n";
 }
} pA;


/**
 * EventProcessor B
 */ 
class EventProcessorB : public EventProcessor {
public:
 EventProcessorB() {
  std::cout << "Register EventProcessor B\n";
  EventProcessorSingleton::instance().registerEventProcessor("B",this); 
 }
 void process(const std::vector<std::string> &, va_list &args) override {
  std::cout << "Process B ";
  std::cout <<  va_arg(args,int) << " ";
  std::cout <<  va_arg(args,int) << "\n";
 }
} pB;

int main() 
{
 EventProcessorSingleton::instance().process("A",10);
 EventProcessorSingleton::instance().process("B",20,30);
 EventProcessorSingleton::instance().process("C");
}

