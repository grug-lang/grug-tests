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

// Forward declarations to opaque ids
struct grug_state;
struct grug_file_id;
struct grug_entity_id;

#define CALL(state, game_fn_name, ...) p_game_fn_##game_fn_name((state), (const union grug_value[]){ __VA_ARGS__ })

#define CALL_ARGLESS(state, game_fn_name) p_game_fn_##game_fn_name((state), NULL)

typedef union grug_value (*game_fn)(struct grug_state* state, const union grug_value[]);

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
 * @param state Current active grug state.
 * @param grug_file_path Path to the grug source file to compile.
 * @param error_out Out parameter for a compile error message. Outputs `NULL` on success.
 * @return An opaque identifier to the compiled file that can be used to create entities.
 */
typedef struct grug_file_id* (*compile_grug_file_t)(struct grug_state* state, const char* file_path, const char** error_out);

/**
 * @typedef init_globals_t
 * @brief Function pointer type for initializing a grug_entity and keeping it
 * ready for subsequent calls to call_export_fn_t.
 *
 * It should create an entity from the file id, initialize its globals, and
 * store the entity in a global. Any previous entities should be deinitialized.
 *
 * @param state Current active grug state.
 * @param file_id The file_id to create an entity and run function for.
 */
typedef void (*init_globals_t)(struct grug_state* state, struct grug_file_id* file_id);

/**
 * @typedef call_export_fn_t
 * @brief Function pointer type for invoking a grug function handler.
 *
 * It should call `fn_name` on the entity created by `init_globals_t`
 * passing `args` and `args_count`.
 *
 * @param state Current active grug state.
 * @param file_id The file_id of the current entity. Must be the same as the
 * last call to `init_globals_t`.
 * @param fn_name Name of the grug function to invoke.
 * @param args Array of `grug_value` arguments to pass to the function.
 * @param args_count number of arguments being passed to the function.
 */
typedef void (*call_export_fn_t)(struct grug_state* state, struct grug_file_id* file_id, const char* fn_name, const union grug_value* args, size_t args_count);

/**
 * @typedef dump_file_to_json_t
 * @brief Function pointer type for dumping a `.grug` file's AST to JSON.
 *
 * All tests verify round-trip fidelity: reading a JSON representation of a
 * grug AST and generating a textual `.grug` source file from it.
 *
 * It should parse the grug file at `input_grug_path`, produce a JSON
 * representation of its AST, and write it to `output_buffer`. The output
 * should be null terminated
 *
 * @param state Current active grug state.
 * @param input_grug_path Path to the input `.grug` source file to be dumped.
 * @param output_buffer Buffer to write the output into.
 * @param output_buffer_len Size of the output buffer currently allocated.
 * @return the number of bytes written to the output, if the number of bytes
 * is equal to size of the buffer, the buffer is discarded and the function is
 * called again with a bigger buffer. If the returned size is (size_t)(-1),
 * it indicates an error in the function
 */
typedef size_t (*dump_file_to_json_t)(struct grug_state* state, const char *input_grug_path, char *output_buffer, size_t output_buffer_len);

/**
 * @typedef generate_file_from_json_t
 * @brief Function pointer type for generating a `.grug` file from an AST JSON.
 *
 * All tests verify round-trip fidelity: reading a JSON representation of a
 * grug AST and generating a textual `.grug` source file from it.
 *
 * It should read the AST from `input_json`, generate the `.grug` text for it,
 * and write it to `output_grug_path`.
 *
 * @param state Current active grug state.
 * @param input_buffer Buffer containing the AST JSON.
 * @param output_buffer Buffer to write the output into.
 * @param output_buffer_len Size of the output buffer currently allocated.
 * @return the number of bytes written to the output, if the number of bytes
 * is equal to size of the buffer, the buffer is discarded and the function is
 * called again with a bigger buffer. If the returned size is (size_t)(-1),
 * it indicates an error in the function
 */
typedef size_t (*generate_file_from_json_t)(struct grug_state* state, const char *input_buffer, char *output_buffer, size_t output_buffer_len);

/**
 * @typedef game_fn_error_t
 * @brief Function pointer type for throwing a game function error.
 *
 * @param state Current active grug state.
 * @param message The error message.
 */
typedef void (*game_fn_error_t)(struct grug_state* state, const char *message);

/**
 * @typedef create_grug_state_t
 * @brief Create an instance of `grug_state` that can be passed to all other functions.
 *
 * @param mod_api_path Path to the mod_api.json this state will be initialized with.
 * @param mods_dir Path to the mods directory this state should use.
 * @return Null if an error occurred. If no state is needed, return a dummy value like 42.
 */
typedef struct grug_state* (*create_grug_state_t) (
	const char* mod_api_path,
	const char* mods_dir
);

/**
 * @typedef destroy_grug_state_t
 * @brief Destroy a `grug_state` that was created from a previous call to `create_grug_state`.
 *
 * @param grug_state The state to destroy.
 */
typedef void (*destroy_grug_state_t)(struct grug_state* state);

/// A vtable of pointers passed from the grug implementation to grug-tests.
/// Contains all the functions needed to run the entire test suite.
struct grug_state_vtable {
	create_grug_state_t create_grug_state;
	destroy_grug_state_t destroy_grug_state;
	compile_grug_file_t compile_grug_file;
	init_globals_t init_globals;
	call_export_fn_t call_export_fn;
	dump_file_to_json_t dump_file_to_json;
	generate_file_from_json_t generate_file_from_json;
	game_fn_error_t game_fn_error;
};

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
 * @param mod_api_path Path to the mod_api.json in the root of grug-tests.
 * @param state_vtable vtable of the implementation's function pointers.
 * @param whitelisted_test A specific test name to run. Pass `NULL` if all tests should be run.
 */
void grug_tests_run(const char *tests_dir_path,
					const char *mod_api_path,
					struct grug_state_vtable state_vtable,
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
typedef union grug_value (*game_fn)           (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_nothing              (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_magic                (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_initialize           (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_initialize_bool      (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_identity             (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_max                  (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_say                  (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_sin                  (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_cos                  (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_mega                 (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_get_false            (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_set_is_happy         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_mega_f32             (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_mega_i32             (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_draw                 (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_blocked_alrm         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_spawn                (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_spawn_d              (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_has_resource         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_has_entity           (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_has_string           (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_get_opponent         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_get_os               (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_set_d                (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_set_opponent         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_motherload           (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_motherload_subless   (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_offset_32_bit_f32    (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_offset_32_bit_i32    (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_offset_32_bit_string (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_talk                 (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_get_position         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_set_position         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_cause_game_fn_error  (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_call_on_b_fn         (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_store                (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_print_csv            (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_retrieve             (struct grug_state* grug_state, const union grug_value args[]);
union grug_value game_fn_box_number           (struct grug_state* grug_state, const union grug_value args[]);
