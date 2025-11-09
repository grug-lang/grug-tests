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
// The README and CI of every language's grug bindings repository should state it's tested
// by cloning the grug-tests repository next to it.
// Every language's grug bindings repository should contain a small bindings_tester.c
// that passes the path of the grug-tests repository its tests/ directory
// to the bindings its init(). This allows tests to be treated as mods.
// bindings_tester.c should then call grug_tests_run() from grug-tests its tests.h.

#pragma once

#include "grug_backend.h"

/**
 * @typedef compile_grug_file_t
 * @brief Function pointer type for compiling a grug file.
 *
 * This function is provided by `bindings_tester.c`.  
 *
 * @param grug_file_path Path to the grug source file to compile.
 * @return `NULL` on success, or an error message string on failure.
 */
typedef const char *(*compile_grug_file_t)(const char *grug_file_path);

/**
 * @typedef init_globals_fn_dispatcher_t
 * @brief Function pointer type for initializing global variables for a grug file.
 *
 * This function is provided by `bindings_tester.c`.
 *
 * @param grug_file_path Path to the grug source file whose globals should be initialized.
 */
typedef void (*init_globals_fn_dispatcher_t)(const char *grug_file_path);

/**
 * @typedef on_fn_dispatcher_t
 * @brief Function pointer type for invoking a grug function handler.
 *
 * This function is provided by `bindings_tester.c`.  
 * It should call the specified function `on_fn_name` in the grug file at `grug_file_path`,
 * passing along the provided array of `values`.
 *
 * @param on_fn_name Name of the grug function to invoke.
 * @param grug_file_path Path to the grug source file containing the function.
 * @param values Array of `grug_value` arguments to pass to the function.
 */
typedef void (*on_fn_dispatcher_t)(const char *on_fn_name, const char *grug_file_path, struct grug_value values[]);

/**
 * @brief Runs all grug tests.
 *
 * Called by the bindings to execute all available grug tests.  
 * This function iterates over all `.grug` files in the following directories:
 * - `tests/err/`
 * - `tests/err_runtime/`
 * - `tests/ok/`
 *
 * @param compile_grug_file Function to compile a grug file.
 * @param init_globals_fn_dispatcher Function to initialize globals for a grug file.
 * @param on_fn_dispatcher Function to invoke grug functions during testing.
 * @param whitelisted_test A specific test name to run. Pass `NULL` if all tests should be run.
 */
void grug_tests_run(compile_grug_file_t compile_grug_file,
                    init_globals_fn_dispatcher_t init_globals_fn_dispatcher,
                    on_fn_dispatcher_t on_fn_dispatcher,
                    const char *whitelisted_test);

/**
 * @brief Handles runtime errors during grug test execution.
 *
 * Called by the bindings whenever a runtime error occurs,  
 * allowing `tests.c` to verify that an expected runtime error took place.
 *
 * @param reason Human-readable description of the error.
 * @param type Type of runtime error.
 * @param on_fn_name Name of the function where the error occurred.
 * @param on_fn_path Path to the grug source file containing the function.
 */
void grug_tests_runtime_error_handler(const char *reason,
                                      enum grug_runtime_error_type type,
                                      const char *on_fn_name,
                                      const char *on_fn_path);
