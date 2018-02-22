#!/bin/bash

# capture coverage info
lcov --directory . --capture --output-file coverage.info
# filter out system and test code
lcov --remove coverage.info 'tests/*' '/usr/*' 'src/test/*' --output-file coverage.info
# debug before upload
lcov --list coverage.info
# uploads to coveralls
coveralls-lcov --repo-token ${COVERALLS_TOKEN} coverage.info
