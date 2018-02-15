#!/bin/bash

set -e -u -o pipefail

if [ $(which clang-format-4.0) ]
then

clang-format-4.0 \
            -i \
            -style=file \
            ./src/*.cpp \
            ./src/*.h \
            ./src/test/*.cpp \
            ./src/test/*.h

fi
