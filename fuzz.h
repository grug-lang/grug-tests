// fuzz.h

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void (*grug_fuzz_run_fn)(const uint8_t *data, size_t size);

void grug_fuzz(grug_fuzz_run_fn run_fn);
