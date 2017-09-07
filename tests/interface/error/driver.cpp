/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file driver.cpp 
 *
 * Driver program to check whether signal handlers are restored
 * after invocation of Souffle's program.
 *
 ***********************************************************************/

#include <array>
#include <vector>
#include <string>
#include <signal.h>
#include "souffle/SouffleInterface.h"

using namespace souffle; 

/**
 * Error handler
 */ 
void error(std::string txt) 
{
   std::cerr << "error: " << txt << "\n"; 
   exit(1); 
} 

/**
 * Signal handler
 */
void handler(int n)
{
   std::cerr << "handler invoked." << std::endl;
   exit(1);
}

/**
 * Main program
 */
int main(int argc, char **argv)
{
   signal(SIGINT, handler); 
   // check number of arguments 
   if(argc != 2) error("wrong number of arguments!"); 

   // create instance of program "load_print"
   if(SouffleProgram *prog = ProgramFactory::newInstance("error")) {

      // load all input relations from current directory
      prog->loadAll(argv[1]);
 
      // run program 
      prog->run(); 

      // free program 
      delete prog; 

      // raise signal to check that signal handler is restored
      raise(SIGINT);

   } else {
      error("cannot find program load_print");        
   }
}
