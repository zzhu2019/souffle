#!/bin/bash

set -e -u -o pipefail

git clone https://github.com/${SOUFFLE_GITHUB_USER}/souffle /souffle
cd /souffle
git pull
git checkout ${SOUFFLE_GIT_BRANCH}
git reset --hard ${SOUFFLE_GIT_REVISION}
git clean -xdf
./bootstrap
./configure ${SOUFFLE_OPTIONS}
make
/souffle/src/souffle
make install
souffle
