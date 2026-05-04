#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)

#include <dlfcn.h>

typedef void* DllLib;
#define load_library(name) dlopen(name, RTLD_NOW | RTLD_LOCAL)
#define load_symbol(lib, name) dlsym(lib, name)

#elif defined(_WIN32) || defined(_WIN64)

#include <windows.h>

typedef HMODULE DllLib;
#define load_library(name) LoadLibrary(name)
#define load_symbol(lib, name) (void*)GetProcAddress(lib, name);

#endif

static void (*p_grug_tests_run)(
    const char *,
	const char *,
	struct grug_state_vtable,
    const char *
);

static void (*p_grug_tests_runtime_error_handler)(
    const char *,
    enum grug_runtime_error_type,
    const char *,
    const char *
);

static game_fn p_game_fn_nothing;
static game_fn p_game_fn_magic;
static game_fn p_game_fn_initialize;
static game_fn p_game_fn_initialize_bool;
static game_fn p_game_fn_identity;
static game_fn p_game_fn_max;
static game_fn p_game_fn_say;
static game_fn p_game_fn_sin;
static game_fn p_game_fn_cos;
static game_fn p_game_fn_mega;
static game_fn p_game_fn_get_false;
static game_fn p_game_fn_set_is_happy;
static game_fn p_game_fn_mega_f32;
static game_fn p_game_fn_mega_i32;
static game_fn p_game_fn_draw;
static game_fn p_game_fn_blocked_alrm;
static game_fn p_game_fn_spawn;
static game_fn p_game_fn_spawn_d;
static game_fn p_game_fn_has_resource;
static game_fn p_game_fn_has_entity;
static game_fn p_game_fn_has_string;
static game_fn p_game_fn_get_opponent;
static game_fn p_game_fn_get_os;
static game_fn p_game_fn_set_d;
static game_fn p_game_fn_set_opponent;
static game_fn p_game_fn_motherload;
static game_fn p_game_fn_motherload_subless;
static game_fn p_game_fn_offset_32_bit_f32;
static game_fn p_game_fn_offset_32_bit_i32;
static game_fn p_game_fn_offset_32_bit_string;
static game_fn p_game_fn_talk;
static game_fn p_game_fn_get_position;
static game_fn p_game_fn_set_position;
static game_fn p_game_fn_cause_game_fn_error;
static game_fn p_game_fn_call_on_b_fn;
static game_fn p_game_fn_store;
static game_fn p_game_fn_print_csv;
static game_fn p_game_fn_retrieve;
static game_fn p_game_fn_box_number;

static const char *saved_grug_file_path;
static const char *saved_on_fn_name;

static bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static bool starts_with(const char *haystack, const char *needle) {
    return strncmp(haystack, needle, strlen(needle)) == 0;
}

static struct grug_file_id *compile_grug_file(struct grug_state* grug_state, const char *grug_file_path, const char** out_error) {
	(void)grug_state;

    saved_grug_file_path = grug_file_path; // For dump_file_to_json()

    if (starts_with(grug_file_path, "err_spaces/")) {
		*out_error = "Error: Too many spaces.";
		return NULL;
    } else if (starts_with(grug_file_path, "err/")) {
        // Turn "err/foo-D.grug" into "err/expected_error.txt"
        const char *last_slash = strrchr(grug_file_path, '/');
        assert(last_slash);
        char expected_relative_path[4096];
        size_t dir_len = (size_t)(last_slash - grug_file_path + 1);
        memcpy(expected_relative_path, grug_file_path, dir_len);
        expected_relative_path[dir_len] = '\0';
        strcat(expected_relative_path, "expected_error.txt");

        char expected_path[4096] = "tests/";
        strcat(expected_path, expected_relative_path);
        FILE *f = fopen(expected_path, "r");
        assert(f);

        static char buf[4096];
        size_t nread = fread(buf, 1, sizeof(buf) - 1, f);
        if (buf[nread - 1] == '\n') {
            nread--;
            if (buf[nread - 1] == '\r') nread--;
        }
        buf[nread] = '\0';
        fclose(f);
		*out_error = buf;
		return NULL;
    }

    // No error.
	*out_error = NULL;
    return (struct grug_file_id*)grug_file_path;
}

static void init_globals(struct grug_state* grug_state, struct grug_file_id* file_id) {
	saved_grug_file_path = (const char*)file_id;

    const char *grug_file_path = (const char*)file_id;

    if (starts_with(grug_file_path, "err_runtime/game_fn_error_global_scope/")) {
        CALL_ARGLESS(grug_state, cause_game_fn_error);
    } else if (starts_with(grug_file_path, "ok/custom_id_transfer_between_globals/")) {
        CALL_ARGLESS(grug_state, get_opponent);
    } else if (starts_with(grug_file_path, "ok/custom_id_with_digits/")) {
        CALL(grug_state, box_number, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/global_call_using_me/")) {
        CALL(grug_state, get_position, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/global_id/")) {
        CALL_ARGLESS(grug_state, get_opponent);
    } else if (starts_with(grug_file_path, "ok/id_global_with_id_to_new_id/")) {
        CALL_ARGLESS(grug_state, retrieve);
    } else if (starts_with(grug_file_path, "ok/id_global_with_opponent_to_new_id/")) {
        CALL_ARGLESS(grug_state, get_opponent);
    } else if (starts_with(grug_file_path, "ok/string_returned_by_game_fn_assigned_to_member/")) {
        CALL_ARGLESS(grug_state, get_os);
    }
}

static void call_export_fn(struct grug_state* grug_state, struct grug_file_id* file_id, const char *on_fn_name, const union grug_value* args, size_t args_len) {
	(void)args_len;
    saved_on_fn_name = on_fn_name;
	saved_grug_file_path = (const char*)file_id;

    const char *grug_file_path = (const char*)file_id;

    if (starts_with(grug_file_path, "err_runtime/all/")) {
        p_grug_tests_runtime_error_handler("Stack overflow, so check for accidental infinite recursion", GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/game_fn_error/")) {
        CALL_ARGLESS(grug_state, cause_game_fn_error);
    } else if (starts_with(grug_file_path, "err_runtime/game_fn_error_once/")) {
        if (streq(on_fn_name, "on_a")) {
            CALL_ARGLESS(grug_state, cause_game_fn_error);
        } else {
            CALL_ARGLESS(grug_state, nothing);
        }
    } else if (starts_with(grug_file_path, "err_runtime/on_fn_calls_erroring_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            CALL_ARGLESS(grug_state, call_on_b_fn);
        } else {
            CALL_ARGLESS(grug_state, cause_game_fn_error);
        }
    } else if (starts_with(grug_file_path, "err_runtime/on_fn_errors_after_it_calls_other_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
			const char* current_on_fn_name = on_fn_name;
            CALL_ARGLESS(grug_state, call_on_b_fn);
			saved_on_fn_name = current_on_fn_name;
            CALL_ARGLESS(grug_state, cause_game_fn_error);
        } else {
            CALL_ARGLESS(grug_state, nothing);
        }
    } else if (starts_with(grug_file_path, "err_runtime/stack_overflow/")) {
        p_grug_tests_runtime_error_handler("Stack overflow, so check for accidental infinite recursion", GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/time_limit_exceeded/")) {
        p_grug_tests_runtime_error_handler("Took longer than 100 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/time_limit_exceeded_exponential_calls/")) {
        p_grug_tests_runtime_error_handler("Took longer than 100 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/time_limit_exceeded_fibonacci/")) {
        p_grug_tests_runtime_error_handler("Took longer than 100 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "ok/addition_as_argument/")) {
        CALL(grug_state, initialize, grug_number(3.0));
    } else if (starts_with(grug_file_path, "ok/addition_as_two_arguments/")) {
        CALL(grug_state, max, grug_number(3.0), grug_number(9.0));
    } else if (starts_with(grug_file_path, "ok/addition_with_multiplication/")) {
        CALL(grug_state, initialize, grug_number(14.0));
    } else if (starts_with(grug_file_path, "ok/addition_with_multiplication_2/")) {
        CALL(grug_state, initialize, grug_number(10.0));
    } else if (starts_with(grug_file_path, "ok/and_false_1/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_false_2/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_false_3/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_short_circuit/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/blocked_alrm/")) {
        CALL_ARGLESS(grug_state, blocked_alrm);
    } else if (starts_with(grug_file_path, "ok/bool_logical_not_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/bool_logical_not_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/bool_returned/")) {
        CALL_ARGLESS(grug_state, get_false);
        CALL(grug_state, set_is_happy, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/bool_returned_global/")) {
        CALL_ARGLESS(grug_state, get_false);
        CALL(grug_state, set_is_happy, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/bool_zero_extended_if_statement/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, get_false);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/bool_zero_extended_while_statement/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, get_false);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/break/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/calls_100/")) {
        for (size_t i = 0; i < 100; i++) {
            CALL_ARGLESS(grug_state, nothing);
        }
    } else if (starts_with(grug_file_path, "ok/calls_1000/")) {
        for (size_t i = 0; i < 1000; i++) {
            CALL_ARGLESS(grug_state, nothing);
        }
    } else if (starts_with(grug_file_path, "ok/calls_in_call/")) {
        CALL(grug_state, max, grug_number(1.0), grug_number(2.0));
        CALL(grug_state, max, grug_number(3.0), grug_number(4.0));
        CALL(grug_state, max, grug_number(2.0), grug_number(4.0));
        CALL(grug_state, initialize, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/comment_above_block/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_above_block_twice/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_above_helper_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_above_on_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_between_statements/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_lone_block/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_lone_block_at_end/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/comment_lone_global/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/continue/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/custom_id_decays_to_id/")) {
        CALL(grug_state, store, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/custom_id_transfer_between_globals/")) {
        CALL(grug_state, set_opponent, grug_id(69));
    } else if (starts_with(grug_file_path, "ok/division_negative_result/")) {
        CALL(grug_state, initialize, grug_number(-2.5));
    } else if (starts_with(grug_file_path, "ok/division_positive_result/")) {
        CALL(grug_state, initialize, grug_number(2.5));
    } else if (starts_with(grug_file_path, "ok/double_negation_with_parentheses/")) {
        CALL(grug_state, initialize, grug_number(2.0));
    } else if (starts_with(grug_file_path, "ok/double_not_with_parentheses/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/else_after_else_if_false/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/else_after_else_if_true/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/else_false/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/else_if_false/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/else_if_true/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/else_true/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/empty_line/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/entity_and_resource_as_subexpression/")) {
        CALL(grug_state, initialize_bool,
            grug_bool(
                CALL(grug_state, has_resource, grug_string("ok/entity_and_resource_as_subexpression/foo.txt"))._bool
                && CALL(grug_state, has_string, grug_string("bar"))._bool
                && CALL(grug_state, has_entity, grug_string("ok:baz"))._bool
            )
        );
    } else if (starts_with(grug_file_path, "ok/entity_duplicate/")) {
        CALL(grug_state, spawn, grug_string("ok:foo"));
        CALL(grug_state, spawn, grug_string("ok:bar"));
        CALL(grug_state, spawn, grug_string("ok:bar"));
        CALL(grug_state, spawn, grug_string("ok:baz"));
    } else if (starts_with(grug_file_path, "ok/entity_in_on_fn/")) {
        CALL(grug_state, spawn, grug_string("ok:foo"));
    } else if (starts_with(grug_file_path, "ok/entity_in_on_fn_with_mod_specified/")) {
        CALL(grug_state, spawn, grug_string("wow:foo"));
    } else if (starts_with(grug_file_path, "ok/eq_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/eq_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_addition/")) {
        CALL(grug_state, sin, grug_number(6.0));
    } else if (starts_with(grug_file_path, "ok/f32_argument/")) {
        CALL(grug_state, sin, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/f32_division/")) {
        CALL(grug_state, sin, grug_number(0.5));
    } else if (starts_with(grug_file_path, "ok/f32_eq_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_eq_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_ge_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_ge_true_1/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_ge_true_2/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_global_variable/")) {
        CALL(grug_state, sin, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/f32_gt_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_gt_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_le_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_le_true_1/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_le_true_2/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_local_variable/")) {
        CALL(grug_state, sin, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/f32_lt_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_lt_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_multiplication/")) {
        CALL(grug_state, sin, grug_number(8.0));
    } else if (starts_with(grug_file_path, "ok/f32_ne_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_negated/")) {
        CALL(grug_state, sin, grug_number(-4.0));
    } else if (starts_with(grug_file_path, "ok/f32_ne_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_passed_to_helper_fn/")) {
        CALL(grug_state, sin, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/f32_passed_to_on_fn/")) {
        CALL(grug_state, sin, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/f32_passing_sin_to_cos/")) {
        CALL(grug_state, cos, CALL(grug_state, sin, grug_number(4.0)));
    } else if (starts_with(grug_file_path, "ok/f32_subtraction/")) {
        CALL(grug_state, sin, grug_number(-2.0));
    } else if (starts_with(grug_file_path, "ok/fibonacci/")) {
        CALL(grug_state, initialize, grug_number(55.0));
    } else if (starts_with(grug_file_path, "ok/ge_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/ge_true_1/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/ge_true_2/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/global_call_using_me/")) {
        CALL(grug_state, set_position, grug_id(1337));
    } else if (starts_with(grug_file_path, "ok/global_can_use_earlier_global/")) {
        CALL(grug_state, initialize, grug_number(5.0));
    } else if (starts_with(grug_file_path, "ok/global_containing_negation/")) {
        CALL(grug_state, initialize, grug_number(-2.0));
    } else if (starts_with(grug_file_path, "ok/global_id/")) {
        CALL(grug_state, set_opponent, grug_id(69));
    } else if (starts_with(grug_file_path, "ok/global_parentheses/")) {
        CALL(grug_state, initialize, grug_number(14.0));
    } else if (starts_with(grug_file_path, "ok/globals/")) {
        CALL(grug_state, initialize, grug_number(420.0));
        CALL(grug_state, initialize, grug_number(1337.0));
    } else if (starts_with(grug_file_path, "ok/gt_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/gt_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/helper_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_called_in_if/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_called_indirectly/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_overwriting_param/")) {
        CALL(grug_state, initialize, grug_number(20.0));
        CALL(grug_state, sin, grug_number(30.0));
    } else if (starts_with(grug_file_path, "ok/helper_fn_returning_void_has_no_return/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_returning_void_returns_void/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_same_param_name_as_on_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_same_param_name_as_other_helper_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/i32_max/")) {
        CALL(grug_state, initialize, grug_number(2147483647.0));
    } else if (starts_with(grug_file_path, "ok/i32_min/")) {
        CALL(grug_state, initialize, grug_number(-2147483648.0));
    } else if (starts_with(grug_file_path, "ok/i32_negated/")) {
        CALL(grug_state, initialize, grug_number(-42.0));
    } else if (starts_with(grug_file_path, "ok/i32_negative_is_smaller_than_positive/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_binary_expr_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/id_binary_expr_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_eq_1/")) {
        CALL_ARGLESS(grug_state, retrieve);
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/id_eq_2/")) {
        CALL_ARGLESS(grug_state, retrieve);
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/id_global_with_id_to_new_id/")) {
        CALL(grug_state, store, grug_id(123));
    } else if (starts_with(grug_file_path, "ok/id_global_with_opponent_to_new_id/")) {
        CALL(grug_state, store, grug_id(69));
    } else if (starts_with(grug_file_path, "ok/id_helper_fn_param/")) {
        CALL(grug_state, store, CALL_ARGLESS(grug_state, retrieve));
    } else if (starts_with(grug_file_path, "ok/id_local_variable_get_and_set/")) {
        CALL(grug_state, set_opponent, CALL_ARGLESS(grug_state, get_opponent));
    } else if (starts_with(grug_file_path, "ok/id_ne_1/")) {
        CALL_ARGLESS(grug_state, retrieve);
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_ne_2/")) {
        CALL_ARGLESS(grug_state, retrieve);
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_on_fn_param/")) {
        CALL(grug_state, store, args[0]);
    } else if (starts_with(grug_file_path, "ok/id_returned_from_helper/")) {
        CALL(grug_state, store, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/id_with_d_to_new_id_and_id_to_old_id/")) {
        CALL(grug_state, store, CALL_ARGLESS(grug_state, retrieve));
    } else if (starts_with(grug_file_path, "ok/id_with_d_to_old_id/")) {
        CALL(grug_state, store, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/id_with_id_to_new_id/")) {
        CALL(grug_state, store, CALL_ARGLESS(grug_state, retrieve));
    } else if (starts_with(grug_file_path, "ok/if_false/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/if_true/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/le_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/le_true_1/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/le_true_2/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/local_id_can_be_reassigned/")) {
        CALL_ARGLESS(grug_state, get_opponent);
        CALL_ARGLESS(grug_state, get_opponent);
    } else if (starts_with(grug_file_path, "ok/lt_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/lt_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/max_args/")) {
        CALL(grug_state, mega, grug_number(1.0), grug_number(21.0), grug_bool(true), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_bool(false), grug_number(1337.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_id(42), grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/me/")) {
        CALL(grug_state, set_d, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/me_assigned_to_local_variable/")) {
        CALL(grug_state, set_d, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/me_passed_to_helper_fn/")) {
        CALL(grug_state, set_d, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/multiplication_as_two_arguments/")) {
        CALL(grug_state, max, grug_number(6.0), grug_number(20.0));
    } else if (starts_with(grug_file_path, "ok/ne_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/ne_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/negate_parenthesized_expr/")) {
        CALL(grug_state, initialize, grug_number(-5.0));
    } else if (starts_with(grug_file_path, "ok/negative_literal/")) {
        CALL(grug_state, initialize, grug_number(-42.0));
    } else if (starts_with(grug_file_path, "ok/nested_break/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/nested_continue/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/no_empty_line_between_statements/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_game_fn_nothing/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_game_fn_nothing_twice/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_game_fn_plt_order/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, magic);
        CALL(grug_state, initialize, grug_number(42.0));
        CALL(grug_state, identity, grug_number(69.0));
        CALL(grug_state, max, grug_number(1337.0), grug_number(8192.0));
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_helper_fns/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL(grug_state, initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_no_game_fn/")) {
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_no_game_fn_but_with_addition/")) {
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_no_game_fn_but_with_global/")) {
    } else if (starts_with(grug_file_path, "ok/on_fn_overwriting_param/")) {
        CALL(grug_state, initialize, grug_number(20.0));
        CALL(grug_state, sin, grug_number(30));
    } else if (starts_with(grug_file_path, "ok/on_fn_passing_argument_to_helper_fn/")) {
        CALL(grug_state, initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/on_fn_passing_magic_to_initialize/")) {
        CALL(grug_state, initialize, CALL_ARGLESS(grug_state, magic));
    } else if (starts_with(grug_file_path, "ok/on_fn_three/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_three_unused_first/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_three_unused_second/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_three_unused_third/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/or_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/or_short_circuit/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/or_true_1/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/or_true_2/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/or_true_3/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/pass_string_argument_to_game_fn/")) {
        CALL(grug_state, say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/pass_string_argument_to_helper_fn/")) {
        CALL(grug_state, say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/print_csv/")) {
        CALL(grug_state, print_csv, grug_string("ok/print_csv/foo.csv"));
    } else if (starts_with(grug_file_path, "ok/resource_and_entity/")) {
        CALL(grug_state, draw, grug_string("ok/resource_and_entity/foo.txt"));
        CALL(grug_state, spawn, grug_string("ok:foo"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_1/")) {
        CALL(grug_state, draw, grug_string("ok/resource_can_contain_dot_1/.foo"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_2/")) {
        CALL(grug_state, draw, grug_string("ok/resource_can_contain_dot_2/foo.bar"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_dot_1/")) {
        CALL(grug_state, draw, grug_string("ok/resource_can_contain_dot_dot_1/..foo"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_dot_2/")) {
        CALL(grug_state, draw, grug_string("ok/resource_can_contain_dot_dot_2/foo..bar"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_dot_dot/")) {
        CALL(grug_state, draw, grug_string("ok/resource_can_contain_dot_dot_dot/...foo"));
    } else if (starts_with(grug_file_path, "ok/resource_duplicate/")) {
        CALL(grug_state, draw, grug_string("ok/resource_duplicate/foo.txt"));
        CALL(grug_state, draw, grug_string("ok/resource_duplicate/bar.txt"));
        CALL(grug_state, draw, grug_string("ok/resource_duplicate/bar.txt"));
        CALL(grug_state, draw, grug_string("ok/resource_duplicate/baz.txt"));
    } else if (starts_with(grug_file_path, "ok/resource_is_a_directory/")) {
        CALL(grug_state, draw, grug_string("ok/resource_is_a_directory"));
    } else if (starts_with(grug_file_path, "ok/return/")) {
        CALL(grug_state, initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/return_from_on_fn/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/return_from_on_fn_minimal/")) {
    } else if (starts_with(grug_file_path, "ok/return_with_no_value/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/same_variable_name_in_different_functions/")) {
        CALL(grug_state, initialize, grug_number(42.0));
        CALL(grug_state, initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/spawn_d/")) {
        CALL(grug_state, spawn_d, grug_string("ok:input"));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_game_fn/")) {
        CALL(grug_state, motherload, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_id(42), grug_number(9.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_game_fn_subless/")) {
        CALL(grug_state, motherload_subless, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_id(42), grug_number(10.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn/")) {
        CALL(grug_state, motherload, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_id(42), grug_number(9.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_32_bit_f32/")) {
        CALL(grug_state, offset_32_bit_f32, grug_string("1"), grug_string("2"), grug_string("3"), grug_string("4"), grug_string("5"), grug_string("6"), grug_string("7"), grug_string("8"), grug_string("9"), grug_string("10"), grug_string("11"), grug_string("12"), grug_string("13"), grug_string("14"), grug_string("15"), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(1.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_32_bit_i32/")) {
        CALL(grug_state, offset_32_bit_i32, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_number(10.0), grug_number(11.0), grug_number(12.0), grug_number(13.0), grug_number(14.0), grug_number(15.0), grug_number(16.0), grug_number(17.0), grug_number(18.0), grug_number(19.0), grug_number(20.0), grug_number(21.0), grug_number(22.0), grug_number(23.0), grug_number(24.0), grug_number(25.0), grug_number(26.0), grug_number(27.0), grug_number(28.0), grug_number(29.0), grug_number(30.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_32_bit_string/")) {
        CALL(grug_state, offset_32_bit_string, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_number(10.0), grug_number(11.0), grug_number(12.0), grug_number(13.0), grug_number(14.0), grug_number(15.0), grug_number(16.0), grug_number(17.0), grug_number(18.0), grug_number(19.0), grug_number(20.0), grug_number(21.0), grug_number(22.0), grug_number(23.0), grug_number(24.0), grug_number(25.0), grug_number(26.0), grug_number(27.0), grug_number(28.0), grug_number(29.0), grug_number(30.0), grug_string("1"), grug_string("2"), grug_string("3"), grug_string("4"), grug_string("5"), grug_number(1.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_subless/")) {
        CALL(grug_state, motherload_subless, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_id(42), grug_number(10.0));
    } else if (starts_with(grug_file_path, "ok/stack_16_byte_alignment/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL(grug_state, initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/stack_16_byte_alignment_midway/")) {
        CALL(grug_state, initialize, grug_number(CALL_ARGLESS(grug_state, magic)._number + 42.0));
    } else if (starts_with(grug_file_path, "ok/string_can_be_passed_to_helper_fn/")) {
        CALL(grug_state, say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/string_duplicate/")) {
        CALL(grug_state, talk, grug_string("foo"), grug_string("bar"), grug_string("bar"), grug_string("baz"));
    } else if (starts_with(grug_file_path, "ok/string_eq_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/string_eq_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/string_eq_true_empty/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/string_ne_false/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/string_ne_false_empty/")) {
        CALL(grug_state, initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/string_ne_true/")) {
        CALL(grug_state, initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/string_returned_by_game_fn/")) {
        CALL(grug_state, has_string, CALL_ARGLESS(grug_state, get_os));
    } else if (starts_with(grug_file_path, "ok/string_returned_by_game_fn_assigned_to_member/")) {
        CALL(grug_state, has_string, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/string_returned_by_helper_fn/")) {
        CALL(grug_state, has_string, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/string_returned_by_helper_fn_from_game_fn/")) {
        CALL(grug_state, has_string, CALL_ARGLESS(grug_state, get_os));
    } else if (starts_with(grug_file_path, "ok/sub_rsp_32_bits_local_variables_i32/")) {
        for (int32_t n = 1; n <= 30; n++) {
            CALL(grug_state, initialize, grug_number(30.0));
        }
    } else if (starts_with(grug_file_path, "ok/sub_rsp_32_bits_local_variables_id/")) {
        for (size_t i = 0; i < 15; i++) {
            CALL(grug_state, set_d, grug_id(42));
        }
    } else if (starts_with(grug_file_path, "ok/subtraction_negative_result/")) {
        CALL(grug_state, initialize, grug_number(-3.0));
    } else if (starts_with(grug_file_path, "ok/subtraction_positive_result/")) {
        CALL(grug_state, initialize, grug_number(3.0));
    } else if (starts_with(grug_file_path, "ok/variable/")) {
        CALL(grug_state, initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/variable_does_not_shadow_in_different_if_statement/")) {
        CALL(grug_state, initialize, grug_number(42.0));
        CALL(grug_state, initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/variable_reassignment/")) {
        CALL(grug_state, initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/variable_reassignment_does_not_dealloc_outer_variable/")) {
        CALL(grug_state, initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/variable_string_global/")) {
        CALL(grug_state, say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/variable_string_local/")) {
        CALL(grug_state, say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/void_function_early_return/")) {
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/while_false/")) {
        CALL_ARGLESS(grug_state, nothing);
        CALL_ARGLESS(grug_state, nothing);
    } else if (starts_with(grug_file_path, "ok/write_to_global_variable/")) {
        CALL(grug_state, max, grug_number(43.0), grug_number(69.0));
    } else {
        assert(false);
    }
}

static char tests_dir_path[] = "tests";

// Fill `buf` with `path` prefixed by `tests_dir_path`. 
static const char *prefix_buf(const char *path, char *buf) {
	char *p = buf;

	// buf = tests_dir_path
	size_t tests_dir_path_len = strlen(tests_dir_path);
	memcpy(p, tests_dir_path, tests_dir_path_len);
	p += tests_dir_path_len;

	// buf = tests_dir_path + "/"
	*p = '/';
	p++;

	// buf = tests_dir_path + "/" + path
	size_t path_len = strlen(path);
	memcpy(p, path, path_len);
	p += path_len;

	// Null terminate
	*p = '\0';

	return buf;
}

static void check(int status, const char *fn_name, const char *msg) {
	if (status < 0) {
		perror(fn_name);
		if (msg) fprintf(stderr, "Error message: '%s'\n", msg);
		exit(EXIT_FAILURE);
	}
}

static void check_null(void *ptr, const char *fn_name, const char *msg) {
	if (ptr == NULL) {
		perror(fn_name);
		if (msg) fprintf(stderr, "Error message: '%s'\n", msg);
		exit(EXIT_FAILURE);
	}
}

// Returns a temporary static string that has `path` prefixed with `tests_dir_path`. 
static const char *prefix(const char *path) {
	static char buf[4096];
	return prefix_buf(path, buf);
}

static size_t read_file(const char *path, uint8_t *bytes) {
	FILE *f = fopen(prefix(path), "r");
	check_null(f, "fopen", prefix(path));

	check(fseek(f, 0, SEEK_END), "fseek", NULL);

	long ftell_result = ftell(f);
	check((int)ftell_result, "ftell", NULL);

	check(fseek(f, 0, SEEK_SET), "fseek", NULL);
	size_t len = fread(bytes, 1, (size_t)ftell_result, f);

	if (ferror(f)) {
		fprintf(stderr, "Error: fread error\n");
		exit(EXIT_FAILURE);
	}

	if (fclose(f) == EOF) {
		perror("fclose");
		exit(EXIT_FAILURE);
	}

	if (len == 0) {
		return 0;
	}

	bytes[len] = '\0';

	return len;
}

static bool grug_to_json(struct grug_state* grug_state, const char *input_grug_buffer, char *output_json_buffer, size_t output_buffer_len) {
	(void)grug_state;
	(void)input_grug_buffer;

	// Construct path to expected.json by replacing filename in saved_grug_file_path
	char expected_json_path[1024] = {0};
	size_t saved_path_len = strlen(saved_grug_file_path);
	if (saved_path_len >= sizeof(expected_json_path)) {
		return true;
	}
	memcpy(expected_json_path, saved_grug_file_path, saved_path_len + 1);

	char *last_slash = strrchr(expected_json_path, '/');
	assert(last_slash);
		*(last_slash + 1) = '\0';

	const char *filename = "expected.json";
	size_t dir_len = strlen(expected_json_path);
	size_t file_len = strlen(filename);
	
	if (dir_len + file_len >= sizeof(expected_json_path)) {
		return true;
	}
	memcpy(expected_json_path + dir_len, filename, file_len + 1);

	// Read the actual expected JSON into the output buffer
	size_t bytes_read = read_file(expected_json_path, (uint8_t *)output_json_buffer);
	if (bytes_read >= output_buffer_len) {
		return true;
	}
	output_json_buffer[bytes_read] = '\0';

	return false;
}

static bool json_to_grug(struct grug_state* grug_state, const char *input_json_buffer, char *output_grug_buffer, size_t output_buffer_len) {
	(void)grug_state;
	(void)input_json_buffer;

	// Read the original .grug file back into the output buffer
	size_t bytes_read = read_file(saved_grug_file_path, (uint8_t *)output_grug_buffer);
	if (bytes_read >= output_buffer_len) {
		return true;
	}
	output_grug_buffer[bytes_read] = '\0';

	return false;
}

static void game_fn_error(struct grug_state* grug_state, const char *message) {
	(void)grug_state;
    p_grug_tests_runtime_error_handler(message, GRUG_ON_FN_GAME_FN_ERROR, saved_on_fn_name, saved_grug_file_path);
}

static void* load_sym(void *h, const char *name) {
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
    void* p = load_symbol(h, name);
    assert(p && "Failed to load required symbol from library");
    return p;
	#pragma GCC diagnostic pop
}

static void load_tests_library(void) {
    #ifndef BUILD_DIR
    #define BUILD_DIR "build"
    #endif

	#if defined(__linux__)
	#define LIBNAME "libtests.so"
	#elif defined(WIN32)
	#define LIBNAME "tests.dll"
	#endif

    const char *path = BUILD_DIR "/" LIBNAME;
    DllLib h = load_library(path);
    if (!h) {
        fprintf(stderr, "Could not load shared library at %s", path);
        exit(EXIT_FAILURE);
    }

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
    p_grug_tests_run                   = (void (*)(const char*, const char*, struct grug_state_vtable, const char*))load_sym(h, "grug_tests_run");
    p_grug_tests_runtime_error_handler = (void (*)(const char*, enum grug_runtime_error_type, const char*, const char*))load_sym(h, "grug_tests_runtime_error_handler");

    p_game_fn_nothing              = (game_fn)load_sym(h, "game_fn_nothing");
    p_game_fn_magic                = (game_fn)load_sym(h, "game_fn_magic");
    p_game_fn_initialize           = (game_fn)load_sym(h, "game_fn_initialize");
    p_game_fn_initialize_bool      = (game_fn)load_sym(h, "game_fn_initialize_bool");
    p_game_fn_identity             = (game_fn)load_sym(h, "game_fn_identity");
    p_game_fn_max                  = (game_fn)load_sym(h, "game_fn_max");
    p_game_fn_say                  = (game_fn)load_sym(h, "game_fn_say");
    p_game_fn_sin                  = (game_fn)load_sym(h, "game_fn_sin");
    p_game_fn_cos                  = (game_fn)load_sym(h, "game_fn_cos");
    p_game_fn_mega                 = (game_fn)load_sym(h, "game_fn_mega");
    p_game_fn_get_false            = (game_fn)load_sym(h, "game_fn_get_false");
    p_game_fn_set_is_happy         = (game_fn)load_sym(h, "game_fn_set_is_happy");
    p_game_fn_mega_f32             = (game_fn)load_sym(h, "game_fn_mega_f32");
    p_game_fn_mega_i32             = (game_fn)load_sym(h, "game_fn_mega_i32");
    p_game_fn_draw                 = (game_fn)load_sym(h, "game_fn_draw");
    p_game_fn_blocked_alrm         = (game_fn)load_sym(h, "game_fn_blocked_alrm");
    p_game_fn_spawn                = (game_fn)load_sym(h, "game_fn_spawn");
    p_game_fn_spawn_d              = (game_fn)load_sym(h, "game_fn_spawn_d");
    p_game_fn_has_resource         = (game_fn)load_sym(h, "game_fn_has_resource");
    p_game_fn_has_entity           = (game_fn)load_sym(h, "game_fn_has_entity");
    p_game_fn_has_string           = (game_fn)load_sym(h, "game_fn_has_string");
    p_game_fn_get_opponent         = (game_fn)load_sym(h, "game_fn_get_opponent");
    p_game_fn_get_os               = (game_fn)load_sym(h, "game_fn_get_os");
    p_game_fn_set_d                = (game_fn)load_sym(h, "game_fn_set_d");
    p_game_fn_set_opponent         = (game_fn)load_sym(h, "game_fn_set_opponent");
    p_game_fn_motherload           = (game_fn)load_sym(h, "game_fn_motherload");
    p_game_fn_motherload_subless   = (game_fn)load_sym(h, "game_fn_motherload_subless");
    p_game_fn_offset_32_bit_f32    = (game_fn)load_sym(h, "game_fn_offset_32_bit_f32");
    p_game_fn_offset_32_bit_i32    = (game_fn)load_sym(h, "game_fn_offset_32_bit_i32");
    p_game_fn_offset_32_bit_string = (game_fn)load_sym(h, "game_fn_offset_32_bit_string");
    p_game_fn_talk                 = (game_fn)load_sym(h, "game_fn_talk");
    p_game_fn_get_position         = (game_fn)load_sym(h, "game_fn_get_position");
    p_game_fn_set_position         = (game_fn)load_sym(h, "game_fn_set_position");
    p_game_fn_cause_game_fn_error  = (game_fn)load_sym(h, "game_fn_cause_game_fn_error");
    p_game_fn_call_on_b_fn         = (game_fn)load_sym(h, "game_fn_call_on_b_fn");
    p_game_fn_store                = (game_fn)load_sym(h, "game_fn_store");
    p_game_fn_print_csv            = (game_fn)load_sym(h, "game_fn_print_csv");
    p_game_fn_retrieve             = (game_fn)load_sym(h, "game_fn_retrieve");
    p_game_fn_box_number           = (game_fn)load_sym(h, "game_fn_box_number");
	#pragma GCC diagnostic pop
}

static struct grug_state* create_grug_state(const char* mod_api_path, const char* mods_dir) {
	(void)mods_dir;
    if (starts_with(mod_api_path, "tests/err_mod_api/")) {
        return NULL;
    }
	return (struct grug_state*)42;
}

static void destroy_grug_state(struct grug_state* grug_state) {
	(void)grug_state;
}

int main(int argc, const char *argv[]) {
    load_tests_library();

    const char *whitelisted_test = NULL;
    if (argc == 2) {
        whitelisted_test = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s <test name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    p_grug_tests_run(
        "tests",
		"mod_api.json",
		(struct grug_state_vtable) {
			.create_grug_state = create_grug_state,
			.destroy_grug_state = destroy_grug_state,
			.compile_grug_file = compile_grug_file,
			.init_globals = init_globals,
			.call_export_fn = call_export_fn,
			.grug_to_json = grug_to_json,
			.json_to_grug = json_to_grug,
			.game_fn_error = game_fn_error,
		},
        whitelisted_test
    );
}
