#!/bin/bash

# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

# Install requirements of MAC OS X
#brew update #uncomment this if a new version of the database is needed.
brew install md5sha1sum bison libtool
brew link bison --force
