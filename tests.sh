#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

# TODO: Update the docs, since the grug repository isn't passed as an argument anymore

# -----------------------------
# Compiler flags
# -----------------------------
compiler_flags=(
    -g
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wshadow
    -Wuninitialized
    -Wunused-macros
    -Wfatal-errors
)

# -----------------------------
# Feature flags
# -----------------------------
if [[ ${ASAN+x} ]]; then
    echo "- ASAN was turned on"
    compiler_flags+=( -fsanitize=address,undefined )
fi

if [[ ${COVERAGE+x} ]]; then
    echo "- COVERAGE was turned on"
    compiler_flags+=( --coverage )
    rm -f *.gcda
fi

if [[ ${SHUFFLES+x} ]]; then
    echo "- SHUFFLES=${SHUFFLES} was passed"
    compiler_flags+=( "-DSHUFFLES=${SHUFFLES}" )
fi

if [[ ${VALGRIND+x} ]]; then
    echo "- VALGRIND was turned on"
fi

if [[ ${ANALYZE+x} ]]; then
    echo "- ANALYZE was turned on"
    if [[ "${CC:-clang}" = "gcc" ]]; then
        compiler_flags+=( --analyzer )
    else
        compiler_flags+=( --analyze )
    fi
fi

# -----------------------------
# Compiler selection
# -----------------------------
CC="${CC:=clang}"
echo "Compilation will use $CC"

if [[ "$CC" = "clang" ]]; then
    compiler_flags+=( -gdwarf-4 ) # build.yml requires this, for some reason
fi

# -----------------------------
# Build tests.so
# -----------------------------
if [[ tests.c -nt tests.so ]]; then
    echo "Recompiling tests.so..."
    "$CC" tests.c \
        -shared -fPIC -o tests.so \
        "${compiler_flags[@]}" \
        -rdynamic -lm
fi

# -----------------------------
# Build smoketest
# -----------------------------
if [[ smoketest.c -nt smoketest ]]; then
    echo "Recompiling smoketest..."
    "$CC" smoketest.c -o smoketest $compiler_flags || { echo 'Recompiling smoketest failed :('; exit 1; }
fi

# -----------------------------
# Run smoketest
# -----------------------------
printf "\n"
if [[ ${VALGRIND+x} ]]; then
    valgrind --quiet ./smoketest "${@:1}"
else
    ./smoketest "${@:1}"
fi

if [ $? -ne 0 ]; then
    exit 1
fi

# -----------------------------
# Coverage report
# -----------------------------
if [[ ${COVERAGE+x} ]]; then
    gcovr \
        --gcov-executable "llvm-cov gcov" \
        --html-details coverage.html \
        --html-theme github.green
fi
