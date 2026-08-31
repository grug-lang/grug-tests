# grug tests

This is [grug](https://github.com/grug-lang/grug) its official test suite.

## Running the smoke tests

1. Clone this repository, and `cd` into it.
2. Run CMake.
    - On Linux, run `cmake -S . -B build`
    - On Windows, run `cmake -S . -B build -G 'MinGW Makefile' -DCMAKE_C_COMPILER=gcc`
3. Build and run the smoke tests with `cmake --build build && build/smoketest`

## Command-line flags

`smoketest` accepts an optional test name (to run just that one test), plus:

- `--continue-on-fail`: By default grug-tests stops at the very first failing test. Pass this flag to keep running the rest of the suite instead, so a single run can surface every failing test at once.
- `--results-json-path <JSON path>`: By default grug-tests writes a `results.json` file (4-space indented) to the current working directory, containing a `summary` (pass percentage, passed/total counts) and a `tests` breakdown per category. Each test has a `"passed"` key; for the `ok` and `err_runtime` categories, which each get run twice, a test also has `"passed_run_1"` and `"passed_run_2"`, and `"passed"` is only `true` when both of those are. Pass this flag to write it somewhere else instead.

Example: `build/smoketest --continue-on-fail --results-json-path /tmp/results.json`

## Troubleshooting

If a test fails, you can reproduce it by passing `CFLAGS="-DSEED=<failing test's printed seed>"` to CMake when configuring the build.

If you're using a Debian-based distribution like Ubuntu 22.04, you might need to run `sudo sysctl vm.mmap_rnd_bits=28` if you're using address sanitizer. See [this GitHub thread](https://github.com/actions/runner-images/issues/9524#issuecomment-2002475952) for context.
