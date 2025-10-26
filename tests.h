#pragma once

// The goal is for every language's grug bindings to check their correctness using tests.c.
//
// It should be made as easy as possible for bindings to use tests.c, which means:
// 1. None of the bindings nor backend code should contain anything related to tests.c.
// 2. All test game function definitions should be kept in tests.c,
//    so they don't need to be copied into every language's bindings.
// 3. The tests in tests.c should only assert the values of its own static global variables.
//    This means that rather than having tests.c inspect the globals pointer directly,
//    the grug file needs to pass the global variable into a game function of tests.c,
//    which will save the value into its own static global.
//
// The README and CI of every language's grug bindings repository should explain how
// the grug-tests repository needs to be cloned next to the bindings repository.
// Every language's grug bindings repository then should contain a small bindings_tester.c
// that passes the path of the grug-tests repository its tests/ directory to grug_init().
// This allows tests to be treated as mods.
// The bindings_tester.c should then call grug_trace_run_tests() from grug-tests its tests.h.

// bindings_tester.c will be responsible for providing an on fn dispatcher,
// by matching on_fn_name to the appropriate on fn in the bindings.
// This *does* strongly couple bindings_tester.c with every on fn
// called by grug-tests its tests.c, but that seems acceptable.
typedef void (*on_fn_dispatcher_t)(const char *on_fn_name);

// Loops over all .grug files in tests/err/, tests/err_runtime/ and tests/ok/.
void grug_trace_run_tests(on_fn_dispatcher_t on_fn_dispatcher);
