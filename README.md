# grug tests · [![CI Build](https://github.com/MyNameIsTrez/grug-tests/actions/workflows/build.yml/badge.svg)](https://github.com/MyNameIsTrez/grug-tests/actions/workflows/build.yml)

This is [grug](https://github.com/grug-lang/grug) its official test suite.

## Running the smoke tests

1. Clone this repository, and `cd` into it.
2. Run CMake.
    - On Linux, run `cmake -S . -B build`
    - On Windows, run `cmake -S . -B build -G 'MinGW Makefile' -DCMAKE_C_COMPILER=gcc`
3. Build the smoke tests with `cmake --build build`
4. Run the smoke tests with `build/smoketest`

## Troubleshooting

If a test fails, you can reproduce it by passing `CFLAGS="-DSEED=<failing test's printed seed>"` to CMake when configuring the build.

If you're using a Debian-based distribution like Ubuntu 22.04, you might need to run `sudo sysctl vm.mmap_rnd_bits=28` if you're using address sanitizer. See [this GitHub thread](https://github.com/actions/runner-images/issues/9524#issuecomment-2002475952) for context.
