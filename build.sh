#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

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

if [[ ${ANALYZE+x} ]]; then
    echo "- ANALYZE was turned on"
    if [[ "${CC:-clang}" = "gcc" ]]; then
        compiler_flags+=( --analyzer )
    else
        compiler_flags+=( --analyze )
    fi
fi

if [[ "$(uname)" == "Darwin" ]]; then
    echo "Detected macOS"
    compiler_flags+=( -I. )
fi

CC="${CC:=clang}"
echo "Compilation will use $CC"

if [[ "$CC" = "clang" ]]; then
    compiler_flags+=( -gdwarf-4 )
fi

echo "Building tests.so..."
"$CC" tests.c -shared -fPIC -o tests.so "${compiler_flags[@]}" -rdynamic -lm

echo "✓ Built tests.so"
