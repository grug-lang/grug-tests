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

enum grug_type {
    grug_type_i32,
    grug_type_f32,
    grug_type_id,
};

struct grug_value {
    enum grug_type type;
    union {
        int32_t i32;
        float f32;
        uint64_t id;
    };
};

enum grug_runtime_error_type {
	GRUG_ON_FN_DIVISION_BY_ZERO,
	GRUG_ON_FN_STACK_OVERFLOW,
	GRUG_ON_FN_TIME_LIMIT_EXCEEDED,
	GRUG_ON_FN_OVERFLOW,
	GRUG_ON_FN_GAME_FN_ERROR,
};

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
 *
 * It should call the specified function `on_fn_name` in the grug file at `grug_file_path`,
 * passing along the provided array of `values`.
 *
 * @param on_fn_name Name of the grug function to invoke.
 * @param grug_file_path Path to the grug source file containing the function.
 * @param values Array of `grug_value` arguments to pass to the function.
 * @param value_count Number of values in the `values` array parameter.
 */
typedef void (*on_fn_dispatcher_t)(const char *on_fn_name, const char *grug_file_path, struct grug_value values[], size_t value_count);

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
 * @param compile_grug_file Function to compile a grug file.
 * @param init_globals_fn_dispatcher Function to initialize globals for a grug file.
 * @param on_fn_dispatcher Function to invoke grug functions during testing.
 * @param dump_file_to_json Function to dump a grug file's AST to JSON.
 * @param generate_file_from_json Function to generate a grug file from an AST JSON.
 * @param game_fn_error Function to throw a game function error.
 * @param whitelisted_test A specific test name to run. Pass `NULL` if all tests should be run.
 */
void grug_tests_run(compile_grug_file_t compile_grug_file,
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
int32_t game_fn_magic(void);
void game_fn_initialize(int32_t x);
void game_fn_initialize_bool(bool b);
int32_t game_fn_identity(int32_t x);
int32_t game_fn_max(int32_t x, int32_t y);
void game_fn_say(const char *message);
float game_fn_sin(float x);
float game_fn_cos(float x);
void game_fn_mega(float f1, int32_t i1, bool b1, float f2, float f3, float f4, bool b2, int32_t i2, float f5, float f6, float f7, float f8, uint64_t id, const char *str);
int game_fn_get_evil_false(void);
void game_fn_set_is_happy(bool is_happy);
void game_fn_mega_f32(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9);
void game_fn_mega_i32(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7);
void game_fn_draw(const char *sprite_path);
void game_fn_blocked_alrm(void);
void game_fn_nothing(void);
void game_fn_spawn(const char *name);
bool game_fn_has_resource(const char *path);
bool game_fn_has_entity(const char *name);
bool game_fn_has_string(const char *str);
uint64_t game_fn_get_opponent(void);
void game_fn_set_d(uint64_t target);
void game_fn_set_opponent(uint64_t target);
void game_fn_motherload(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, uint64_t id, float f9);
void game_fn_motherload_subless(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, uint64_t id, float f10);
void game_fn_offset_32_bit_f32(const char *s1, const char *s2, const char *s3, const char *s4, const char *s5, const char *s6, const char *s7, const char *s8, const char *s9, const char *s10, const char *s11, const char *s12, const char *s13, const char *s14, const char *s15, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, int32_t g);
void game_fn_offset_32_bit_i32(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19, float f20, float f21, float f22, float f23, float f24, float f25, float f26, float f27, float f28, float f29, float f30, int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t g);
void game_fn_offset_32_bit_string(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19, float f20, float f21, float f22, float f23, float f24, float f25, float f26, float f27, float f28, float f29, float f30, const char *s1, const char *s2, const char *s3, const char *s4, const char *s5, int32_t g);
void game_fn_talk(const char *message1, const char *message2, const char *message3, const char *message4);
uint64_t game_fn_get_position(uint64_t id);
void game_fn_set_position(uint64_t pos);
void game_fn_cause_game_fn_error(void);
void game_fn_call_on_b_fn(void);
void game_fn_store(uint64_t id);
uint64_t game_fn_retrieve(void);
uint64_t game_fn_box_i32(int32_t n);
