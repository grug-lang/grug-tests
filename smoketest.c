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

#elif defined(WIN32) 

#include <windows.h>

typedef HMODULE DllLib;
#define load_library(name) LoadLibrary(name)
#define load_symbol(lib, name) (void*)GetProcAddress(lib, name);

#endif

static void (*p_grug_tests_run)(
    const char *,
    compile_grug_file_t,
    init_globals_fn_dispatcher_t,
    on_fn_dispatcher_t,
    dump_file_to_json_t,
    generate_file_from_json_t,
    game_fn_error_t,
    const char *
);

static void (*p_grug_tests_runtime_error_handler)(
    const char *,
    enum grug_runtime_error_type,
    const char *,
    const char *
);

static void (*p_game_fn_nothing)(void);
static union grug_value (*p_game_fn_magic)(void);
static void (*p_game_fn_initialize)(const union grug_value args[]);
static void (*p_game_fn_initialize_bool)(const union grug_value args[]);
static union grug_value (*p_game_fn_identity)(const union grug_value args[]);
static union grug_value (*p_game_fn_max)(const union grug_value args[]);
static void (*p_game_fn_say)(const union grug_value args[]);
static union grug_value (*p_game_fn_sin)(const union grug_value args[]);
static union grug_value (*p_game_fn_cos)(const union grug_value args[]);
static void (*p_game_fn_mega)(const union grug_value args[]);
static union grug_value (*p_game_fn_get_false)(void);
static void (*p_game_fn_set_is_happy)(const union grug_value args[]);
static void (*p_game_fn_mega_f32)(const union grug_value args[]);
static void (*p_game_fn_mega_i32)(const union grug_value args[]);
static void (*p_game_fn_draw)(const union grug_value args[]);
static void (*p_game_fn_blocked_alrm)(void);
static void (*p_game_fn_spawn)(const union grug_value args[]);
static union grug_value (*p_game_fn_has_resource)(const union grug_value args[]);
static union grug_value (*p_game_fn_has_entity)(const union grug_value args[]);
static union grug_value (*p_game_fn_has_string)(const union grug_value args[]);
static union grug_value (*p_game_fn_get_opponent)(void);
static void (*p_game_fn_set_d)(const union grug_value args[]);
static void (*p_game_fn_set_opponent)(const union grug_value args[]);
static void (*p_game_fn_motherload)(const union grug_value args[]);
static void (*p_game_fn_motherload_subless)(const union grug_value args[]);
static void (*p_game_fn_offset_32_bit_f32)(const union grug_value args[]);
static void (*p_game_fn_offset_32_bit_i32)(const union grug_value args[]);
static void (*p_game_fn_offset_32_bit_string)(const union grug_value args[]);
static void (*p_game_fn_talk)(const union grug_value args[]);
static union grug_value (*p_game_fn_get_position)(const union grug_value args[]);
static void (*p_game_fn_set_position)(const union grug_value args[]);
static void (*p_game_fn_cause_game_fn_error)(void);
static void (*p_game_fn_call_on_b_fn)(void);
static void (*p_game_fn_store)(const union grug_value args[]);
static union grug_value (*p_game_fn_retrieve)(void);
static union grug_value (*p_game_fn_box_number)(const union grug_value args[]);

static const char *saved_grug_file_path;
static const char *saved_on_fn_name;

static bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static bool starts_with(const char *haystack, const char *needle) {
    return strncmp(haystack, needle, strlen(needle)) == 0;
}

static const char *compile_grug_file(const char *grug_file_path) {
    saved_grug_file_path = grug_file_path;

    if (starts_with(grug_file_path, "err/")) {
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
        return buf;
    }

    // No error happened, so no error message to return.
    return NULL;
}

static void init_globals_fn_dispatcher(void) {
    const char *grug_file_path = saved_grug_file_path;

    if (starts_with(grug_file_path, "ok/custom_id_transfer_between_globals/")) {
        CALL_ARGLESS(get_opponent);
    } else if (starts_with(grug_file_path, "ok/custom_id_with_digits/")) {
        CALL(box_number, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/global_call_using_me/")) {
        CALL(get_position, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/global_id/")) {
        CALL_ARGLESS(get_opponent);
    } else if (starts_with(grug_file_path, "ok/id_global_with_id_to_new_id/")) {
        CALL_ARGLESS(retrieve);
    } else if (starts_with(grug_file_path, "ok/id_global_with_opponent_to_new_id/")) {
        CALL_ARGLESS(get_opponent);
    }
}

static void on_fn_dispatcher(const char *on_fn_name, const union grug_value args[]) {
    saved_on_fn_name = on_fn_name;

    const char *grug_file_path = saved_grug_file_path;

    if (starts_with(grug_file_path, "err_runtime/all/")) {
        p_grug_tests_runtime_error_handler("Stack overflow, so check for accidental infinite recursion", GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/game_fn_error/")) {
        CALL_ARGLESS(cause_game_fn_error);
    } else if (starts_with(grug_file_path, "err_runtime/game_fn_error_once/")) {
        if (streq(on_fn_name, "on_a")) {
            CALL_ARGLESS(cause_game_fn_error);
        } else {
            CALL_ARGLESS(nothing);
        }
    } else if (starts_with(grug_file_path, "err_runtime/on_fn_calls_erroring_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            CALL_ARGLESS(call_on_b_fn);
        } else {
            CALL_ARGLESS(cause_game_fn_error);
        }
    } else if (starts_with(grug_file_path, "err_runtime/on_fn_errors_after_it_calls_other_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            CALL_ARGLESS(call_on_b_fn);
            CALL_ARGLESS(cause_game_fn_error);
        } else {
            CALL_ARGLESS(nothing);
        }
    } else if (starts_with(grug_file_path, "err_runtime/stack_overflow/")) {
        p_grug_tests_runtime_error_handler("Stack overflow, so check for accidental infinite recursion", GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/time_limit_exceeded/")) {
        p_grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/time_limit_exceeded_exponential_calls/")) {
        p_grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "err_runtime/time_limit_exceeded_fibonacci/")) {
        p_grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "ok/addition_as_argument/")) {
        CALL(initialize, grug_number(3.0));
    } else if (starts_with(grug_file_path, "ok/addition_as_two_arguments/")) {
        CALL(max, grug_number(3.0), grug_number(9.0));
    } else if (starts_with(grug_file_path, "ok/addition_with_multiplication/")) {
        CALL(initialize, grug_number(14.0));
    } else if (starts_with(grug_file_path, "ok/addition_with_multiplication_2/")) {
        CALL(initialize, grug_number(10.0));
    } else if (starts_with(grug_file_path, "ok/and_false_1/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_false_2/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_false_3/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_short_circuit/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/and_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/blocked_alrm/")) {
        CALL_ARGLESS(blocked_alrm);
    } else if (starts_with(grug_file_path, "ok/bool_logical_not_false/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/bool_logical_not_true/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/bool_returned/")) {
        CALL_ARGLESS(get_false);
        CALL(set_is_happy, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/bool_returned_global/")) {
        CALL_ARGLESS(get_false);
        CALL(set_is_happy, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/bool_zero_extended_if_statement/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(get_false);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/bool_zero_extended_while_statement/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(get_false);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/break/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/calls_100/")) {
        for (size_t i = 0; i < 100; i++) {
            CALL_ARGLESS(nothing);
        }
    } else if (starts_with(grug_file_path, "ok/calls_1000/")) {
        for (size_t i = 0; i < 1000; i++) {
            CALL_ARGLESS(nothing);
        }
    } else if (starts_with(grug_file_path, "ok/calls_in_call/")) {
        CALL(max, grug_number(1.0), grug_number(2.0));
        CALL(max, grug_number(3.0), grug_number(4.0));
        CALL(max, grug_number(2.0), grug_number(4.0));
        CALL(initialize, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/comment_above_block/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_above_block_twice/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_above_helper_fn/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_above_on_fn/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_between_statements/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_lone_block/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_lone_block_at_end/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/comment_lone_global/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/continue/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/custom_id_decays_to_id/")) {
        CALL(store, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/custom_id_transfer_between_globals/")) {
        CALL(set_opponent, grug_id(69));
    } else if (starts_with(grug_file_path, "ok/division_negative_result/")) {
        CALL(initialize, grug_number(-2.5));
    } else if (starts_with(grug_file_path, "ok/division_positive_result/")) {
        CALL(initialize, grug_number(2.5));
    } else if (starts_with(grug_file_path, "ok/double_negation_with_parentheses/")) {
        CALL(initialize, grug_number(2.0));
    } else if (starts_with(grug_file_path, "ok/double_not_with_parentheses/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/else_after_else_if_false/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/else_after_else_if_true/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/else_false/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/else_if_false/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/else_if_true/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/else_true/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/empty_line/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/entity_and_resource_as_subexpression/")) {
        CALL(initialize_bool,
            grug_bool(
                CALL(has_resource, grug_string("ok/entity_and_resource_as_subexpression/foo.txt"))._bool
                && CALL(has_string, grug_string("bar"))._bool
                && CALL(has_entity, grug_string("ok:baz"))._bool
            )
        );
    } else if (starts_with(grug_file_path, "ok/entity_duplicate/")) {
        CALL(spawn, grug_string("ok:foo"));
        CALL(spawn, grug_string("ok:bar"));
        CALL(spawn, grug_string("ok:bar"));
        CALL(spawn, grug_string("ok:baz"));
    } else if (starts_with(grug_file_path, "ok/entity_in_on_fn/")) {
        CALL(spawn, grug_string("ok:foo"));
    } else if (starts_with(grug_file_path, "ok/entity_in_on_fn_with_mod_specified/")) {
        CALL(spawn, grug_string("wow:foo"));
    } else if (starts_with(grug_file_path, "ok/eq_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/eq_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_addition/")) {
        CALL(sin, grug_number(6.0));
    } else if (starts_with(grug_file_path, "ok/f32_argument/")) {
        CALL(sin, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/f32_division/")) {
        CALL(sin, grug_number(0.5));
    } else if (starts_with(grug_file_path, "ok/f32_eq_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_eq_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_ge_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_ge_true_1/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_ge_true_2/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_global_variable/")) {
        CALL(sin, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/f32_gt_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_gt_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_le_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_le_true_1/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_le_true_2/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_local_variable/")) {
        CALL(sin, grug_number(4.0));
    } else if (starts_with(grug_file_path, "ok/f32_lt_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_lt_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_multiplication/")) {
        CALL(sin, grug_number(8.0));
    } else if (starts_with(grug_file_path, "ok/f32_ne_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/f32_negated/")) {
        CALL(sin, grug_number(-4.0));
    } else if (starts_with(grug_file_path, "ok/f32_ne_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/f32_passed_to_helper_fn/")) {
        CALL(sin, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/f32_passed_to_on_fn/")) {
        CALL(sin, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/f32_passing_sin_to_cos/")) {
        CALL(cos, CALL(sin, grug_number(4.0)));
    } else if (starts_with(grug_file_path, "ok/f32_subtraction/")) {
        CALL(sin, grug_number(-2.0));
    } else if (starts_with(grug_file_path, "ok/fibonacci/")) {
        CALL(initialize, grug_number(55.0));
    } else if (starts_with(grug_file_path, "ok/ge_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/ge_true_1/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/ge_true_2/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/global_call_using_me/")) {
        CALL(set_position, grug_id(1337));
    } else if (starts_with(grug_file_path, "ok/global_can_use_earlier_global/")) {
        CALL(initialize, grug_number(5.0));
    } else if (starts_with(grug_file_path, "ok/global_containing_negation/")) {
        CALL(initialize, grug_number(-2.0));
    } else if (starts_with(grug_file_path, "ok/global_id/")) {
        CALL(set_opponent, grug_id(69));
    } else if (starts_with(grug_file_path, "ok/globals/")) {
        CALL(initialize, grug_number(420.0));
        CALL(initialize, grug_number(1337.0));
    } else if (starts_with(grug_file_path, "ok/gt_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/gt_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/helper_fn/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_called_in_if/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_called_indirectly/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_overwriting_param/")) {
        CALL(initialize, grug_number(20.0));
        CALL(sin, grug_number(30.0));
    } else if (starts_with(grug_file_path, "ok/helper_fn_returning_void_has_no_return/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_returning_void_returns_void/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_same_param_name_as_on_fn/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/helper_fn_same_param_name_as_other_helper_fn/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/i32_max/")) {
        CALL(initialize, grug_number(2147483647.0));
    } else if (starts_with(grug_file_path, "ok/i32_min/")) {
        CALL(initialize, grug_number(-2147483648.0));
    } else if (starts_with(grug_file_path, "ok/i32_negated/")) {
        CALL(initialize, grug_number(-42.0));
    } else if (starts_with(grug_file_path, "ok/i32_negative_is_smaller_than_positive/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_binary_expr_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/id_binary_expr_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_eq_1/")) {
        CALL_ARGLESS(retrieve);
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/id_eq_2/")) {
        CALL_ARGLESS(retrieve);
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/id_global_with_id_to_new_id/")) {
        CALL(store, grug_id(123));
    } else if (starts_with(grug_file_path, "ok/id_global_with_opponent_to_new_id/")) {
        CALL(store, grug_id(69));
    } else if (starts_with(grug_file_path, "ok/id_helper_fn_param/")) {
        CALL(store, CALL_ARGLESS(retrieve));
    } else if (starts_with(grug_file_path, "ok/id_local_variable_get_and_set/")) {
        CALL(set_opponent, CALL_ARGLESS(get_opponent));
    } else if (starts_with(grug_file_path, "ok/id_ne_1/")) {
        CALL_ARGLESS(retrieve);
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_ne_2/")) {
        CALL_ARGLESS(retrieve);
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/id_on_fn_param/")) {
        CALL(store, args[0]);
    } else if (starts_with(grug_file_path, "ok/id_returned_from_helper/")) {
        CALL(store, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/id_with_d_to_new_id_and_id_to_old_id/")) {
        CALL(store, CALL_ARGLESS(retrieve));
    } else if (starts_with(grug_file_path, "ok/id_with_d_to_old_id/")) {
        CALL(store, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/id_with_id_to_new_id/")) {
        CALL(store, CALL_ARGLESS(retrieve));
    } else if (starts_with(grug_file_path, "ok/if_false/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/if_true/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/le_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/le_true_1/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/le_true_2/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/local_id_can_be_reassigned/")) {
        CALL_ARGLESS(get_opponent);
        CALL_ARGLESS(get_opponent);
    } else if (starts_with(grug_file_path, "ok/lt_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/lt_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/max_args/")) {
        CALL(mega, grug_number(1.0), grug_number(21.0), grug_bool(true), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_bool(false), grug_number(1337.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_id(42), grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/me/")) {
        CALL(set_d, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/me_assigned_to_local_variable/")) {
        CALL(set_d, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/me_passed_to_helper_fn/")) {
        CALL(set_d, grug_id(42));
    } else if (starts_with(grug_file_path, "ok/multiplication_as_two_arguments/")) {
        CALL(max, grug_number(6.0), grug_number(20.0));
    } else if (starts_with(grug_file_path, "ok/ne_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/ne_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/negate_parenthesized_expr/")) {
        CALL(initialize, grug_number(-5.0));
    } else if (starts_with(grug_file_path, "ok/negative_literal/")) {
        CALL(initialize, grug_number(-42.0));
    } else if (starts_with(grug_file_path, "ok/nested_break/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/nested_continue/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/no_empty_line_between_statements/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_game_fn_nothing/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_game_fn_nothing_twice/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_game_fn_plt_order/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(magic);
        CALL(initialize, grug_number(42.0));
        CALL(identity, grug_number(69.0));
        CALL(max, grug_number(1337.0), grug_number(8192.0));
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_helper_fns/")) {
        CALL_ARGLESS(nothing);
        CALL(initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_no_game_fn/")) {
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_no_game_fn_but_with_addition/")) {
    } else if (starts_with(grug_file_path, "ok/on_fn_calling_no_game_fn_but_with_global/")) {
    } else if (starts_with(grug_file_path, "ok/on_fn_overwriting_param/")) {
        CALL(initialize, grug_number(20.0));
        CALL(sin, grug_number(30));
    } else if (starts_with(grug_file_path, "ok/on_fn_passing_argument_to_helper_fn/")) {
        CALL(initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/on_fn_passing_magic_to_initialize/")) {
        CALL(initialize, CALL_ARGLESS(magic));
    } else if (starts_with(grug_file_path, "ok/on_fn_three/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_three_unused_first/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_three_unused_second/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/on_fn_three_unused_third/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/or_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/or_short_circuit/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/or_true_1/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/or_true_2/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/or_true_3/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/pass_string_argument_to_game_fn/")) {
        CALL(say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/pass_string_argument_to_helper_fn/")) {
        CALL(say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/resource_and_entity/")) {
        CALL(draw, grug_string("ok/resource_and_entity/foo.txt"));
        CALL(spawn, grug_string("ok:foo"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_1/")) {
        CALL(draw, grug_string("ok/resource_can_contain_dot_1/.foo"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_2/")) {
        CALL(draw, grug_string("ok/resource_can_contain_dot_2/foo."));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_3/")) {
        CALL(draw, grug_string("ok/resource_can_contain_dot_3/foo.bar"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_dot_1/")) {
        CALL(draw, grug_string("ok/resource_can_contain_dot_dot_1/..foo"));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_dot_2/")) {
        CALL(draw, grug_string("ok/resource_can_contain_dot_dot_2/foo.."));
    } else if (starts_with(grug_file_path, "ok/resource_can_contain_dot_dot_3/")) {
        CALL(draw, grug_string("ok/resource_can_contain_dot_dot_3/foo..bar"));
    } else if (starts_with(grug_file_path, "ok/resource_duplicate/")) {
        CALL(draw, grug_string("ok/resource_duplicate/foo.txt"));
        CALL(draw, grug_string("ok/resource_duplicate/bar.txt"));
        CALL(draw, grug_string("ok/resource_duplicate/bar.txt"));
        CALL(draw, grug_string("ok/resource_duplicate/baz.txt"));
    } else if (starts_with(grug_file_path, "ok/return/")) {
        CALL(initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/return_from_on_fn/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/return_from_on_fn_minimal/")) {
    } else if (starts_with(grug_file_path, "ok/return_with_no_value/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/same_variable_name_in_different_functions/")) {
        CALL(initialize, grug_number(42.0));
        CALL(initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_game_fn/")) {
        CALL(motherload, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_id(42), grug_number(9.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_game_fn_subless/")) {
        CALL(motherload_subless, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_id(42), grug_number(10.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn/")) {
        CALL(motherload, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_id(42), grug_number(9.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_32_bit_f32/")) {
        CALL(offset_32_bit_f32, grug_string("1"), grug_string("2"), grug_string("3"), grug_string("4"), grug_string("5"), grug_string("6"), grug_string("7"), grug_string("8"), grug_string("9"), grug_string("10"), grug_string("11"), grug_string("12"), grug_string("13"), grug_string("14"), grug_string("15"), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(1.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_32_bit_i32/")) {
        CALL(offset_32_bit_i32, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_number(10.0), grug_number(11.0), grug_number(12.0), grug_number(13.0), grug_number(14.0), grug_number(15.0), grug_number(16.0), grug_number(17.0), grug_number(18.0), grug_number(19.0), grug_number(20.0), grug_number(21.0), grug_number(22.0), grug_number(23.0), grug_number(24.0), grug_number(25.0), grug_number(26.0), grug_number(27.0), grug_number(28.0), grug_number(29.0), grug_number(30.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_32_bit_string/")) {
        CALL(offset_32_bit_string, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_number(10.0), grug_number(11.0), grug_number(12.0), grug_number(13.0), grug_number(14.0), grug_number(15.0), grug_number(16.0), grug_number(17.0), grug_number(18.0), grug_number(19.0), grug_number(20.0), grug_number(21.0), grug_number(22.0), grug_number(23.0), grug_number(24.0), grug_number(25.0), grug_number(26.0), grug_number(27.0), grug_number(28.0), grug_number(29.0), grug_number(30.0), grug_string("1"), grug_string("2"), grug_string("3"), grug_string("4"), grug_string("5"), grug_number(1.0));
    } else if (starts_with(grug_file_path, "ok/spill_args_to_helper_fn_subless/")) {
        CALL(motherload_subless, grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(1.0), grug_number(2.0), grug_number(3.0), grug_number(4.0), grug_number(5.0), grug_number(6.0), grug_number(7.0), grug_number(8.0), grug_number(9.0), grug_id(42), grug_number(10.0));
    } else if (starts_with(grug_file_path, "ok/stack_16_byte_alignment/")) {
        CALL_ARGLESS(nothing);
        CALL(initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/stack_16_byte_alignment_midway/")) {
        CALL(initialize, grug_number(CALL_ARGLESS(magic)._number + 42.0));
    } else if (starts_with(grug_file_path, "ok/string_can_be_passed_to_helper_fn/")) {
        CALL(say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/string_duplicate/")) {
        CALL(talk, grug_string("foo"), grug_string("bar"), grug_string("bar"), grug_string("baz"));
    } else if (starts_with(grug_file_path, "ok/string_eq_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/string_eq_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/string_eq_true_empty/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/string_ne_false/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/string_ne_false_empty/")) {
        CALL(initialize_bool, grug_bool(false));
    } else if (starts_with(grug_file_path, "ok/string_ne_true/")) {
        CALL(initialize_bool, grug_bool(true));
    } else if (starts_with(grug_file_path, "ok/sub_rsp_32_bits_local_variables_i32/")) {
        for (int32_t n = 1; n <= 30; n++) {
            CALL(initialize, grug_number(30.0));
        }
    } else if (starts_with(grug_file_path, "ok/sub_rsp_32_bits_local_variables_id/")) {
        for (size_t i = 0; i < 15; i++) {
            CALL(set_d, grug_id(42));
        }
    } else if (starts_with(grug_file_path, "ok/subtraction_negative_result/")) {
        CALL(initialize, grug_number(-3.0));
    } else if (starts_with(grug_file_path, "ok/subtraction_positive_result/")) {
        CALL(initialize, grug_number(3.0));
    } else if (starts_with(grug_file_path, "ok/variable/")) {
        CALL(initialize, grug_number(42.0));
    } else if (starts_with(grug_file_path, "ok/variable_does_not_shadow_in_different_if_statement/")) {
        CALL(initialize, grug_number(42.0));
        CALL(initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/variable_reassignment/")) {
        CALL(initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/variable_reassignment_does_not_dealloc_outer_variable/")) {
        CALL(initialize, grug_number(69.0));
    } else if (starts_with(grug_file_path, "ok/variable_string_global/")) {
        CALL(say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/variable_string_local/")) {
        CALL(say, grug_string("foo"));
    } else if (starts_with(grug_file_path, "ok/void_function_early_return/")) {
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/while_false/")) {
        CALL_ARGLESS(nothing);
        CALL_ARGLESS(nothing);
    } else if (starts_with(grug_file_path, "ok/write_to_global_variable/")) {
        CALL(max, grug_number(43.0), grug_number(69.0));
    } else {
        assert(false);
    }
}

static bool copy_file(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");
    if (!src) {
        perror("Failed to open source file");
        fprintf(stderr, "path: %s\n", src_path);
        return true;
    }

    FILE *dst = fopen(dst_path, "wb");
    if (!dst) {
        perror("Failed to open destination file");
        fprintf(stderr, "path: %s\n", dst_path);
        fclose(src);
        return true;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            perror("Write error");
            fclose(src);
            fclose(dst);
            return true;
        }
    }

    if (ferror(src)) {
        perror("Read error");
        fclose(src);
        fclose(dst);
        return true;
    }

    fclose(src);
    fclose(dst);
    return false;
}

static bool dump_file_to_json(const char *input_grug_path, const char *output_json_path) {
    return copy_file(input_grug_path, output_json_path);
}

static bool generate_file_from_json(const char *input_json_path, const char *output_grug_path) {
    return copy_file(input_json_path, output_grug_path);
}

static void game_fn_error(const char *message) {
    p_grug_tests_runtime_error_handler(message, GRUG_ON_FN_GAME_FN_ERROR, saved_on_fn_name, saved_grug_file_path);
}

static void* load_sym(void *h, const char *name) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #pragma GCC diagnostic ignored "-Wint-conversion"
    void* p = load_symbol(h, name);
    assert(p && "Failed to load required symbol from tests.so");
    return p;
    #pragma GCC diagnostic pop
}

static void load_tests_so(void) {

	#if defined(__linux__)
	#define LIBNAME "libtests.so"
	#elif defined(WIN32)
	#define LIBNAME "tests.dll"
	#endif

    DllLib h = load_library("./build/" LIBNAME);
    assert(h && "Could not load shared library");

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    p_grug_tests_run = load_sym(h, "grug_tests_run");
    p_grug_tests_runtime_error_handler = load_sym(h, "grug_tests_runtime_error_handler");

    p_game_fn_nothing = load_sym(h, "game_fn_nothing");
    p_game_fn_magic = load_sym(h, "game_fn_magic");
    p_game_fn_initialize = load_sym(h, "game_fn_initialize");
    p_game_fn_initialize_bool = load_sym(h, "game_fn_initialize_bool");
    p_game_fn_identity = load_sym(h, "game_fn_identity");
    p_game_fn_max = load_sym(h, "game_fn_max");
    p_game_fn_say = load_sym(h, "game_fn_say");
    p_game_fn_sin = load_sym(h, "game_fn_sin");
    p_game_fn_cos = load_sym(h, "game_fn_cos");
    p_game_fn_mega = load_sym(h, "game_fn_mega");
    p_game_fn_get_false = load_sym(h, "game_fn_get_false");
    p_game_fn_set_is_happy = load_sym(h, "game_fn_set_is_happy");
    p_game_fn_mega_f32 = load_sym(h, "game_fn_mega_f32");
    p_game_fn_mega_i32 = load_sym(h, "game_fn_mega_i32");
    p_game_fn_draw = load_sym(h, "game_fn_draw");
    p_game_fn_blocked_alrm = load_sym(h, "game_fn_blocked_alrm");
    p_game_fn_spawn = load_sym(h, "game_fn_spawn");
    p_game_fn_has_resource = load_sym(h, "game_fn_has_resource");
    p_game_fn_has_entity = load_sym(h, "game_fn_has_entity");
    p_game_fn_has_string = load_sym(h, "game_fn_has_string");
    p_game_fn_get_opponent = load_sym(h, "game_fn_get_opponent");
    p_game_fn_set_d = load_sym(h, "game_fn_set_d");
    p_game_fn_set_opponent = load_sym(h, "game_fn_set_opponent");
    p_game_fn_motherload = load_sym(h, "game_fn_motherload");
    p_game_fn_motherload_subless = load_sym(h, "game_fn_motherload_subless");
    p_game_fn_offset_32_bit_f32 = load_sym(h, "game_fn_offset_32_bit_f32");
    p_game_fn_offset_32_bit_i32 = load_sym(h, "game_fn_offset_32_bit_i32");
    p_game_fn_offset_32_bit_string = load_sym(h, "game_fn_offset_32_bit_string");
    p_game_fn_talk = load_sym(h, "game_fn_talk");
    p_game_fn_get_position = load_sym(h, "game_fn_get_position");
    p_game_fn_set_position = load_sym(h, "game_fn_set_position");
    p_game_fn_cause_game_fn_error = load_sym(h, "game_fn_cause_game_fn_error");
    p_game_fn_call_on_b_fn = load_sym(h, "game_fn_call_on_b_fn");
    p_game_fn_store = load_sym(h, "game_fn_store");
    p_game_fn_retrieve = load_sym(h, "game_fn_retrieve");
    p_game_fn_box_number = load_sym(h, "game_fn_box_number");
    #pragma GCC diagnostic pop
}

int main(int argc, const char *argv[]) {
    load_tests_so();

    const char *whitelisted_test = NULL;
    if (argc == 2) {
        whitelisted_test = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s <test name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    p_grug_tests_run(
        "tests",
        compile_grug_file,
        init_globals_fn_dispatcher,
        on_fn_dispatcher,
        dump_file_to_json,
        generate_file_from_json,
        game_fn_error,
        whitelisted_test
    );
}
