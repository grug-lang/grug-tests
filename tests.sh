#!/bin/bash

# Require grug path as the first argument
if [[ -z "$1" ]]; then
    echo "Usage: $0 <path-to-grug> [test-args...]"
    exit 1
fi
GRUG_PATH="$1"

compiler_flags="-I${GRUG_PATH} -g -Wall -Wextra -Werror -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wuninitialized -Wunused-macros -Wfatal-errors"

# This makes compilation quite a bit slower
# compiler_flags+=' -Og'

if [[ ${OUTPUT_DLL_INFO+x} ]]
then
    echo "- OUTPUT_DLL_INFO was turned on"
    compiler_flags+=' -DOUTPUT_DLL_INFO'
fi

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

if [[ ${OLD_LD+x} ]]
then
    echo "- OLD_LD was turned on"
    compiler_flags+=' -DOLD_LD'
fi

if [[ ${LOGGING+x} ]]
then
    echo "- LOGGING was turned on"
    compiler_flags+=' -DLOGGING'
fi

if [[ ${VALGRIND+x} ]]
then
    echo "- VALGRIND was turned on"
fi

if [[ ${SHUFFLES+x} ]]
then
    echo "- SHUFFLES=${SHUFFLES} was passed"
    compiler_flags+=" -DSHUFFLES=${SHUFFLES}"
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

if [[ ${DEBUG_EXPECTED_NASM+x} ]]
then
    echo "- DEBUG_EXPECTED_NASM was turned on"
    compiler_flags+=' -DDEBUG_EXPECTED_NASM'
fi

# TODO: Can this be removed, or is it still relevant?
if [ "$(uname)" == "Darwin" ]; then # If Mac OS X
    echo "Detected macOS"
    compiler_flags+=' -I.' # For `#include <elf.h>`
fi

compiler_flags+=' -DCRASH_ON_UNREACHABLE'

CC="${CC:=clang}"
echo "Compilation will use $CC"

if [[ "$CC" = "gcc" ]]
then
    compiler_flags+=' -Wno-pragmas'
elif [[ "$CC" = "clang" ]]
then
    compiler_flags+=' -gdwarf-4' # build.yml requires this, for some reason
fi

if [[ $(find "$GRUG_PATH"/src -type f -newer grug.o) ]] \
   || [[ "$GRUG_PATH"/grug.h -nt grug.o ]] \
   || [[ tests.sh -nt grug.o ]]
then
    echo "Recompiling grug.o..."
    "$CC" "${GRUG_PATH}/src/14_hot_reloading.c" -c -o grug.o $compiler_flags || { echo 'Recompiling grug.o failed :('; exit 1; }
fi

if (! [[ tests.c -ot tests.o ]]) || (! [[ tests.sh -ot tests.o ]])
then
    echo "Recompiling tests.o..."
    "$CC" tests.c -c -o tests.o $compiler_flags || { echo 'Recompiling tests.o failed :('; exit 1; }
fi

# `-rdynamic` allows the .so to call functions from test.c
# `-lm` links against libm.so/libm.a to get access to math functions
linker_flags='-rdynamic -lm'

if (! [[ tests.o -ot tests.out ]]) || (! [[ grug.o -ot tests.out ]]) || (! [[ tests.sh -ot tests.out ]])
then
    # TODO: Try using the mold linker here, and add README instructions back if it's faster
    echo "Linking tests.out..."
    # TODO: Try removing $compiler_flags here
    "$CC" tests.o grug.o -o tests.out $compiler_flags $linker_flags || { echo 'Linking tests.out failed :('; exit 1; }
fi

echo "Running tests.out..."
# "$@" passes any whitelisted test names to tests.out
if [[ ${VALGRIND+x} ]]
then
    # This makes compilation quite a bit slower
    valgrind --quiet ./tests.out "${@:2}"
else
    ./tests.out "${@:2}"
fi

if [ $? -ne 0 ]; then
    exit 1
fi

if [[ -v COVERAGE ]]
then
    gcovr --gcov-executable "llvm-cov gcov" --html-details coverage.html --html-theme github.green
fi
