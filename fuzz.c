// fuzz.c

#include "fuzz.h"

#include <stdlib.h>
#include <unistd.h>

void grug_fuzz(grug_fuzz_run_fn run_fn) {
    uint8_t buf[4096];

    ssize_t size = read(STDIN_FILENO, buf, sizeof(buf));
    if (size <= 0)
        return;

    run_fn(buf, (size_t)size);
}
