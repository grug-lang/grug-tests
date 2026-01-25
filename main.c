// main.c

#include "fuzz.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void my_callback(const uint8_t *data, size_t size) {
    if (size == 2 && data[0] == 'a' && data[1] == 'b') {
        fprintf(stderr, "Crash from main.c!\n");
        abort();
    }
}

int main() {
    void *lib = dlopen("./libfuzz.so", RTLD_NOW);
    if (!lib) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    void (*grug_fuzz_fn)(grug_fuzz_run_fn) =
        dlsym(lib, "grug_fuzz");

    if (!grug_fuzz_fn) {
        fprintf(stderr, "dlsym failed\n");
        return 1;
    }

    grug_fuzz_fn(my_callback);
    return 0;
}
