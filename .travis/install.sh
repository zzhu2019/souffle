#!/bin/bash

# Enable debugging and logging of shell operations
# that are executed.
set -e
set -x

#########
# Linux #
#########

if [[ "$CC" == "clang" ]]
then
  # Install libomp
  sudo add-apt-repository -y "deb http://ftp.us.debian.org/debian unstable main contrib non-free"
  sudo apt-get -qq update
  sudo apt-get -y --force-yes install libomp-dev
fi

############
# MAC OS X #
############

# Install requirements of MAC OS X
if [ $TRAVIS_OS_NAME == osx ]
then
  #brew update #Uncomment this if brew database is out of date
  brew install md5sha1sum bison libtool
  brew link bison --force
fi
