// TODO: Move this to the readme?
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

#define _XOPEN_SOURCE 700 // This is required to get struct FTW from ftw.h

#include <stdbool.h> // Provides bool
#include <stddef.h> // Provides size_t
#include <stdint.h> // Provides uint32_t

#ifndef GRUG_TYPE_NUMBER
    #define GRUG_TYPE_NUMBER double
#endif
#ifndef GRUG_TYPE_BOOL
    #define GRUG_TYPE_BOOL bool
#endif
#ifndef GRUG_TYPE_STRING
    #define GRUG_TYPE_STRING const char*
#endif
#ifndef GRUG_TYPE_ID
    #define GRUG_TYPE_ID uint64_t
#endif

#ifndef GRUG_TYPE_ON_FN_ID
    #define GRUG_TYPE_ON_FN_ID uint64_t
#endif

union grug_value {
#ifndef GRUG_NO_NUMBER
    GRUG_TYPE_NUMBER _number;
#endif
#ifndef GRUG_NO_BOOL
    GRUG_TYPE_BOOL _bool;
#endif
#ifndef GRUG_NO_STRING
    GRUG_TYPE_STRING _string;
#endif
#ifndef GRUG_NO_ID
    GRUG_TYPE_ID _id;
#endif
};

#define CALL(game_fn_name, ...) p_game_fn_##game_fn_name((const union grug_value[]){ __VA_ARGS__ })

#define CALL_ARGLESS(game_fn_name) p_game_fn_##game_fn_name()

static inline union grug_value grug_number(GRUG_TYPE_NUMBER v) { union grug_value r; r._number = v; return r; }
static inline union grug_value grug_bool(GRUG_TYPE_BOOL v) { union grug_value r; r._bool = v; return r; }
static inline union grug_value grug_string(GRUG_TYPE_STRING v) { union grug_value r; r._string = v; return r; }
static inline union grug_value grug_id(GRUG_TYPE_ID v) { union grug_value r; r._id = v; return r; }

enum grug_runtime_error_type {
	GRUG_ON_FN_STACK_OVERFLOW,
	GRUG_ON_FN_TIME_LIMIT_EXCEEDED,
	GRUG_ON_FN_GAME_FN_ERROR,
};

/**
 * @typedef compile_grug_file_t
 * @brief Function pointer type for compiling a grug file.
 *
 * This function is provided by `bindings_tester.c`.
 *
 * @param grug_file_path Path to the grug source file to compile.
 * @param mod_name Name of the mod which the grug source file is in.
 * @return `NULL` on success, or an error message string on failure.
 */
typedef const char *(*compile_grug_file_t)(const char *grug_file_path, const char *mod_name);

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
 *
 * It should call the specified function `on_fn_name` in the grug file at `grug_file_path`,
 * passing along the provided array of `values`.
 *
 * @param on_fn_name Name of the grug function to invoke.
 * @param grug_file_path Path to the grug source file containing the function.
 * @param args Array of `grug_value` arguments to pass to the function.
 */
typedef void (*on_fn_dispatcher_t)(const char *on_fn_name, const char *grug_file_path, const union grug_value args[]);

/**
 * @typedef dump_file_to_json_t
 * @brief Function pointer type for dumping a `.grug` file's AST to JSON.
 *
 * All tests verify round-trip fidelity: reading a JSON representation of a
 * grug AST and generating a textual `.grug` source file from it.
 *
 * This function is provided by `bindings_tester.c`.
 *
 * It should parse the grug file at `input_grug_path`, produce a JSON
 * representation of its AST, and write it to `output_json_path`.
 *
 * @param input_grug_path Path to the input `.grug` source file to be dumped.
 * @param output_json_path Path to write the produced JSON file.
 * @return `true` if an error occurred.
 */
typedef bool (*dump_file_to_json_t)(const char *input_grug_path, const char *output_json_path);

/**
 * @typedef generate_file_from_json_t
 * @brief Function pointer type for generating a `.grug` file from an AST JSON.
 *
 * All tests verify round-trip fidelity: reading a JSON representation of a
 * grug AST and generating a textual `.grug` source file from it.
 *
 * This function is provided by `bindings_tester.c`.
 *
 * It should read the AST at `input_json_path`, generate the `.grug` text for it,
 * and write it to `output_grug_path`.
 *
 * @param input_json_path Path to the input JSON file containing the grug AST.
 * @param output_grug_path Path to write the generated `.grug` source file.
 * @return `true` if an error occurred.
 */
typedef bool (*generate_file_from_json_t)(const char *input_json_path, const char *output_grug_path);

/**
 * @typedef game_fn_error_t
 * @brief Function pointer type for throwing a game function error.
 *
 * This function is provided by `bindings_tester.c`.
 *
 * @param message The error message.
 */
typedef void (*game_fn_error_t)(const char *message);

/**
 * @brief Runs all grug tests.
 *
 * Called by the bindings to execute all available grug tests.
 * This function iterates over all `.grug` files in the following directories:
 * - `tests/err/`
 * - `tests/err_runtime/`
 * - `tests/ok/`
 *
 * @param tests_dir_path Path to grug-tests its tests/ directory.
 * @param compile_grug_file Function to compile a grug file.
 * @param init_globals_fn_dispatcher Function to initialize globals for a grug file.
 * @param on_fn_dispatcher Function to invoke grug functions during testing.
 * @param dump_file_to_json Function to dump a grug file's AST to JSON.
 * @param generate_file_from_json Function to generate a grug file from an AST JSON.
 * @param game_fn_error Function to throw a game function error.
 * @param whitelisted_test A specific test name to run. Pass `NULL` if all tests should be run.
 */
void grug_tests_run(const char *tests_dir_path,
                    compile_grug_file_t compile_grug_file,
                    init_globals_fn_dispatcher_t init_globals_fn_dispatcher,
                    on_fn_dispatcher_t on_fn_dispatcher,
                    dump_file_to_json_t dump_file_to_json,
                    generate_file_from_json_t generate_file_from_json,
                    game_fn_error_t game_fn_error,
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

/**
 * @brief Game functions that the bindings must call.
 */
void game_fn_nothing(void);
union grug_value game_fn_magic(void);
void game_fn_initialize(const union grug_value args[]);
void game_fn_initialize_bool(const union grug_value args[]);
union grug_value game_fn_identity(const union grug_value args[]);
union grug_value game_fn_max(const union grug_value args[]);
void game_fn_say(const union grug_value args[]);
union grug_value game_fn_sin(const union grug_value args[]);
union grug_value game_fn_cos(const union grug_value args[]);
void game_fn_mega(const union grug_value args[]);
union grug_value game_fn_get_evil_false(void);
void game_fn_set_is_happy(const union grug_value args[]);
void game_fn_mega_f32(const union grug_value args[]);
void game_fn_mega_i32(const union grug_value args[]);
void game_fn_draw(const union grug_value args[]);
void game_fn_blocked_alrm(void);
void game_fn_spawn(const union grug_value args[]);
union grug_value game_fn_has_resource(const union grug_value args[]);
union grug_value game_fn_has_entity(const union grug_value args[]);
union grug_value game_fn_has_string(const union grug_value args[]);
union grug_value game_fn_get_opponent(void);
void game_fn_set_d(const union grug_value args[]);
void game_fn_set_opponent(const union grug_value args[]);
void game_fn_motherload(const union grug_value args[]);
void game_fn_motherload_subless(const union grug_value args[]);
void game_fn_offset_32_bit_f32(const union grug_value args[]);
void game_fn_offset_32_bit_i32(const union grug_value args[]);
void game_fn_offset_32_bit_string(const union grug_value args[]);
void game_fn_talk(const union grug_value args[]);
union grug_value game_fn_get_position(const union grug_value args[]);
void game_fn_set_position(const union grug_value args[]);
void game_fn_cause_game_fn_error(void);
void game_fn_call_on_b_fn(void);
void game_fn_store(const union grug_value args[]);
union grug_value game_fn_retrieve(void);
union grug_value game_fn_box_i32(const union grug_value args[]);
