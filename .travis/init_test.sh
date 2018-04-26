#!/bin/bash


# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

# configure project

# create configure files
./bootstrap
CXXFLAGS=--coverage ./configure
make -j2

