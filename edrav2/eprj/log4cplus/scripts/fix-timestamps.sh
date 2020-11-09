#!/bin/sh

set -x

touch aclocal.m4
find . -name 'Makefile.in' -print -exec touch '{}' ';'
touch configure
touch tests/testsuite
