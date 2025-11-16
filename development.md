# Development

## Notes

- If you want the tests to be run in a shuffled order twice (within the same process), run `SHUFFLES=10 ./tests.sh ../grug`.
- If you want to allow your compiler to optimize `grug.c` hard, run `OPTIMIZED= ./tests.sh ../grug`.
- If you want to allow your compiler to statically analyze `grug.c` for problems, run `ANALYZE= ./tests.sh ../grug`.

## Generating coverage

> [!IMPORTANT]
> Make sure you have [gcovr](https://gcovr.com/en/stable/installation.html) (`pip install gcovr`) and [llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) (`apt install llvm`)

Run `COVERAGE= ./tests.sh ../grug`, and view the generated `coverage.html` with your browser.

You should see that the program has nearly 100% line coverage.

## Fuzzing

This uses [libFuzzer](https://llvm.org/docs/LibFuzzer.html), which requires [Clang](https://en.wikipedia.org/wiki/Clang) to be installed.

First you compile the fuzzer, and create its input corpus:

```bash
clear && \
clang grug/grug.c fuzz.c -Igrug -std=gnu2x -Wall -Wextra -Werror -Wpedantic -Wstrict-prototypes -Wshadow -Wuninitialized -Wfatal-errors -Wno-language-extension-token -Wno-unused-parameter -g -rdynamic -fsanitize=address,undefined,fuzzer --coverage -Og -DCRASH_ON_UNREACHABLE && \
mkdir -p test_corpus && \
for d in tests/err/* tests/err_runtime/* tests/ok/*; do name=${d##*/}; cp $d/*.grug test_corpus/$name.grug; done && \
mkdir -p corpus && \
./a.out -merge=1 corpus test_corpus
```

In my case, replacing `-fsanitize=address,undefined,fuzzer -Og` with `-fsanitize=fuzzer -Ofast -march=native -DNDEBUG` in the above command ups the numbers of executions per second from 5000 to 15000 on my computer.

You then run the fuzzer like so:

```bash
./a.out corpus -use_value_profile=1 -timeout=1
```

Here is how you minimize any crash reproducer:

```bash
./a.out -minimize_crash=1 -runs=1000 crash-*
```

### Generating fuzzing coverage

> [!IMPORTANT]
> Make sure you have [gcovr](https://gcovr.com/en/stable/installation.html) (`pip install gcovr`) and [llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) (`apt install llvm`)

To get an overview of the fuzzing progress, you use Ctrl+C to stop the fuzzing for a moment (since running gcovr simultaneously crashes the fuzzer).

You then run this to generate a `coverage.html` file that you can open in your browser:

```bash
./a.out -runs=0 corpus && \
gcovr --gcov-executable "llvm-cov gcov" --html-details coverage.html --html-theme github.green
```

You can then resume fuzzing with the `./a.out corpus -use_value_profile=1 -timeout=1` command.

## Static code analysis with cppchecker

See its [Wikipedia article](https://en.wikipedia.org/wiki/Cppcheck) for more information.

```bash
time cppcheck --check-level=exhaustive --enable=all --suppress=missingIncludeSystem --suppress=constParameterPointer --suppress=constVariablePointer --suppress=constStatement grug/grug.c >cppcheck_2.log 2>&1
```

## Static code analysis with PVS-Studio

See [my blog post about PVS-Studio](https://mynameistrez.github.io/2024/08/19/static-c-analysis-with-pvs-studio.html) for the what, why, and how.

```bash
pvs-studio-analyzer trace -- clang run.c grug/grug.c -Igrug -Wall -Wextra -Werror -Wpedantic -Wstrict-prototypes -Wshadow -Wuninitialized -Wfatal-errors -g -fsanitize=address,undefined -Og &&\
pvs-studio-analyzer analyze -o pvs.log &&\
plog-converter -a GA:1,2 -t json -o pvs.json pvs.log
```

## Linker map

In order to visualize what grug.c contains when linked, follow these steps:

1. Download [amap](https://www.sikorskiy.net/info/prj/amap/index.html)
2. Run `LINKER_MAP= ./tests.sh ../grug`
3. Run `./amap` in the cloned/unzipped directory of amap.
4. Click the `File` button in the top-left corner, and then `Open file`
5. Select the generated `output.map` using the file explorer popup

## gdb

```bash
gdb --args smoketest and_false_1
```
