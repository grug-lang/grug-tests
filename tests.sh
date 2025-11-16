#!/bin/bash
set -euo pipefail

# TODO: Update the docs, since the grug repository isn't passed as an argument anymore

./build.sh

compiler_flags="-g -Wall -Wextra -Werror -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wuninitialized -Wunused-macros -Wfatal-errors"

# This makes compilation quite a bit slower
# compiler_flags+=' -Og'

if [[ ${ASAN+x} ]]
then
    # This makes compilation quite a bit slower
    echo "- ASAN was turned on"
    compiler_flags+=' -fsanitize=address,undefined'
fi

if [[ ${COVERAGE+x} ]]
then
    echo "- COVERAGE was turned on"
    compiler_flags+=' --coverage'

    # Prevents the error "cannot merge previous GCDA file: corrupt arc tag"
    rm -f *.gcda
fi

if [[ ${VALGRIND+x} ]]
then
    echo "- VALGRIND was turned on"
fi

if [[ ${ANALYZE+x} ]]
then
    echo "- ANALYZE was turned on"
    if [[ "$CC" = "gcc" ]]
    then
        compiler_flags+=' --analyzer'
    else
        compiler_flags+=' --analyze'
    fi
fi

# TODO: Can this be removed, or is it still relevant?
if [ "$(uname)" == "Darwin" ]; then # If Mac OS X
    echo "Detected macOS"
    compiler_flags+=' -I.' # For `#include <elf.h>`
fi

CC="${CC:=clang}"
echo "Compilation will use $CC"

if [[ "$CC" = "clang" ]]
then
    compiler_flags+=' -gdwarf-4' # build.yml requires this, for some reason
fi

if [[ smoketest.c -nt smoketest ]]
then
    echo "Recompiling smoketest..."
    "$CC" smoketest.c -o smoketest $compiler_flags || { echo 'Recompiling smoketest failed :('; exit 1; }
fi

echo "Running smoketest..."
# "$@" passes any whitelisted test names to smoketest
if [[ ${VALGRIND+x} ]]
then
    # This makes compilation quite a bit slower
    valgrind --quiet ./smoketest "${@:1}"
else
    ./smoketest "${@:1}"
fi

if [ $? -ne 0 ]; then
    exit 1
fi

if [[ -v COVERAGE ]]
then
    gcovr --gcov-executable "llvm-cov gcov" --html-details coverage.html --html-theme github.green
fi
