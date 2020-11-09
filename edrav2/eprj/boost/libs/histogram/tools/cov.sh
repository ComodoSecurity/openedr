#!/bin/sh
# must be executed in project root folder
if [ -z $GCOV ]; then
  GCOV=gcov
fi

LCOV_EXE="tools/lcov-1.13/bin/lcov"

if [ ! -e $LCOV_EXE ]; then
  cd tools
  wget -O - https://github.com/linux-test-project/lcov/releases/download/v1.13/lcov-1.13.tar.gz | tar zxf -
  cd ..
fi

# LCOV="$LCOV_EXE --gcov-tool=${GCOV} --rc lcov_branch_coverage=1"
LCOV="$LCOV_EXE --gcov-tool=${GCOV}" # no branch coverage

if [ ! -e coverage.info ]; then
  # collect raw data
  $LCOV --base-directory `pwd`/test --directory `pwd`/../../bin.v2/libs/histogram/test --capture --output-file test.info
  $LCOV --base-directory `pwd`/examples --directory `pwd`/../../bin.v2/libs/histogram/examples --capture --output-file examples.info

  # merge files
  $LCOV -a test.info -a examples.info -o all.info

  # remove uninteresting entries
  $LCOV --extract all.info "*/boost/histogram/*" --output-file coverage.info
fi

if [ $CI ]; then
  # upload if on CI
  curl -s https://codecov.io/bash | bash -s - -f coverage.info -X gcov -x $GCOV
else
  # otherwise just print
  $LCOV --list coverage.info
fi
