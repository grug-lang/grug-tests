#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

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
static int32_t (*p_game_fn_magic)(void);
static void (*p_game_fn_initialize)(int32_t);
static void (*p_game_fn_initialize_bool)(bool);
static int32_t (*p_game_fn_identity)(int32_t);
static int32_t (*p_game_fn_max)(int32_t, int32_t);
static void (*p_game_fn_say)(const char *);
static float (*p_game_fn_sin)(float);
static float (*p_game_fn_cos)(float);
static void (*p_game_fn_mega)(float, int32_t, bool, float, float, float, bool, int32_t,
                              float, float, float, float, uint64_t, const char *);
static int (*p_game_fn_get_evil_false)(void);
static void (*p_game_fn_set_is_happy)(bool);
static void (*p_game_fn_mega_f32)(float, float, float, float, float, float, float, float, float);
static void (*p_game_fn_mega_i32)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
static void (*p_game_fn_draw)(const char *);
static void (*p_game_fn_blocked_alrm)(void);
static void (*p_game_fn_spawn)(const char *);
static bool (*p_game_fn_has_resource)(const char *);
static bool (*p_game_fn_has_entity)(const char *);
static bool (*p_game_fn_has_string)(const char *);
static uint64_t (*p_game_fn_get_opponent)(void);
static void (*p_game_fn_set_d)(uint64_t);
static void (*p_game_fn_set_opponent)(uint64_t);
static void (*p_game_fn_motherload)(
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
    float, float, float, float, float, float, float, float,
    uint64_t, float
);
static void (*p_game_fn_motherload_subless)(
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
    float, float, float, float, float, float, float, float, float,
    uint64_t, float
);

static void (*p_game_fn_offset_32_bit_f32)(
    const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*,
    float, float, float, float, float, float, float, float,
    int32_t
);

static void (*p_game_fn_offset_32_bit_i32)(
    float, float, float, float, float, float, float, float, float, float,
    float, float, float, float, float, float, float, float, float, float,
    float, float, float, float, float, float, float, float, float, float,
    int32_t, int32_t, int32_t, int32_t, int32_t, int32_t
);

static void (*p_game_fn_offset_32_bit_string)(
    float, float, float, float, float, float, float, float, float, float,
    float, float, float, float, float, float, float, float, float, float,
    float, float, float, float, float, float, float, float, float, float,
    const char*, const char*, const char*, const char*, const char*, int32_t
);

static void (*p_game_fn_talk)(
    const char*, const char*, const char*, const char*
);

static uint64_t (*p_game_fn_get_position)(uint64_t);
static void (*p_game_fn_set_position)(uint64_t);
static void (*p_game_fn_cause_game_fn_error)(void);
static void (*p_game_fn_call_on_b_fn)(void);
static void (*p_game_fn_store)(uint64_t);
static uint64_t (*p_game_fn_retrieve)(void);
static uint64_t (*p_game_fn_box_i32)(int32_t);

static const char *saved_on_fn_name;
static const char *saved_grug_file_path;

static bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static bool starts_with(const char *haystack, const char *needle) {
    return strncmp(haystack, needle, strlen(needle)) == 0;
}

static const char *compile_grug_file(const char *grug_file_path, const char *mod_name) {
    (void)mod_name;

    if (starts_with(grug_file_path, "tests/err/")) {
        const char *last_slash = strrchr(grug_file_path, '/');
        assert(last_slash);
        static char expected_path[1337];
        size_t dir_len = last_slash - grug_file_path + 1;
        memcpy(expected_path, grug_file_path, dir_len);
        expected_path[dir_len] = '\0';
        strcat(expected_path, "expected_error.txt");

        FILE *f = fopen(expected_path, "r");
        assert(f);
        static char buf[1337];
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

static void init_globals_fn_dispatcher(const char *grug_file_path) {
    if (starts_with(grug_file_path, "tests/ok/custom_id_transfer_between_globals/")) {
        p_game_fn_get_opponent();
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_with_digits/")) {
        p_game_fn_box_i32(42);
    } else if (starts_with(grug_file_path, "tests/ok/global_call_using_me/")) {
        p_game_fn_get_position(42);
    } else if (starts_with(grug_file_path, "tests/ok/global_id/")) {
        p_game_fn_get_opponent();
    } else if (starts_with(grug_file_path, "tests/ok/id_global_with_id_to_new_id/")) {
        p_game_fn_retrieve();
    } else if (starts_with(grug_file_path, "tests/ok/id_global_with_opponent_to_new_id/")) {
        p_game_fn_get_opponent();
    }
}

static void on_fn_dispatcher(const char *on_fn_name, const char *grug_file_path, struct grug_value values[], size_t value_count) {
    (void)value_count;
    saved_on_fn_name = on_fn_name;
    saved_grug_file_path = grug_file_path;

    if (starts_with(grug_file_path, "tests/err_runtime/all/")) {
        p_grug_tests_runtime_error_handler("Division of an i32 by 0", GRUG_ON_FN_DIVISION_BY_ZERO, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/division_by_0/")) {
        p_grug_tests_runtime_error_handler("Division of an i32 by 0", GRUG_ON_FN_DIVISION_BY_ZERO, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/game_fn_error/")) {
        p_game_fn_cause_game_fn_error();
    } else if (starts_with(grug_file_path, "tests/err_runtime/game_fn_error_once/")) {
        if (streq(on_fn_name, "on_a")) {
            p_game_fn_cause_game_fn_error();
        } else {
            p_game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_addition/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_division/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_multiplication/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_negation/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_remainder/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_subtraction/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_underflow_addition/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_underflow_multiplication/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_underflow_subtraction/")) {
        p_grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/on_fn_calls_erroring_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            p_game_fn_call_on_b_fn();
        } else {
            p_game_fn_cause_game_fn_error();
        }
    } else if (starts_with(grug_file_path, "tests/err_runtime/on_fn_errors_after_it_calls_other_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            p_game_fn_call_on_b_fn();
            p_game_fn_cause_game_fn_error();
        } else {
            p_game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/err_runtime/remainder_by_0/")) {
        p_grug_tests_runtime_error_handler("Division of an i32 by 0", GRUG_ON_FN_DIVISION_BY_ZERO, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/stack_overflow/")) {
        p_grug_tests_runtime_error_handler("Stack overflow, so check for accidental infinite recursion", GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/time_limit_exceeded/")) {
        p_grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/time_limit_exceeded_exponential_calls/")) {
        p_grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/time_limit_exceeded_fibonacci/")) {
        p_grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/ok/addition_as_argument/")) {
        p_game_fn_initialize(3);
    } else if (starts_with(grug_file_path, "tests/ok/addition_as_two_arguments/")) {
        p_game_fn_max(3, 9);
    } else if (starts_with(grug_file_path, "tests/ok/addition_with_multiplication/")) {
        p_game_fn_initialize(14);
    } else if (starts_with(grug_file_path, "tests/ok/addition_with_multiplication_2/")) {
        p_game_fn_initialize(10);
    } else if (starts_with(grug_file_path, "tests/ok/and_false_1/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_false_2/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_false_3/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_short_circuit/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/blocked_alrm/")) {
        p_game_fn_blocked_alrm();
    } else if (starts_with(grug_file_path, "tests/ok/bool_logical_not_false/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/bool_logical_not_true/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/bool_returned/")) {
        p_game_fn_get_evil_false();
        p_game_fn_set_is_happy(false);
    } else if (starts_with(grug_file_path, "tests/ok/bool_returned_global/")) {
        p_game_fn_get_evil_false();
        p_game_fn_set_is_happy(false);
    } else if (starts_with(grug_file_path, "tests/ok/bool_zero_extended_if_statement/")) {
        p_game_fn_nothing();
        p_game_fn_get_evil_false();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/bool_zero_extended_while_statement/")) {
        p_game_fn_nothing();
        p_game_fn_get_evil_false();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/break/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/calls_100/")) {
        for (size_t i = 0; i < 100; i++) {
            p_game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/ok/calls_1000/")) {
        for (size_t i = 0; i < 1000; i++) {
            p_game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/ok/calls_in_call/")) {
        p_game_fn_max(1, 2);
        p_game_fn_max(3, 4);
        p_game_fn_max(2, 4);
        p_game_fn_initialize(4);
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_block/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_block_twice/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_helper_fn/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_on_fn/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_between_statements/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_lone_block/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_lone_block_at_end/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_lone_global/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/continue/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_decays_to_id/")) {
        p_game_fn_store(42);
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_transfer_between_globals/")) {
        p_game_fn_set_opponent(69);
    } else if (starts_with(grug_file_path, "tests/ok/division_negative_result/")) {
        p_game_fn_initialize(-2);
    } else if (starts_with(grug_file_path, "tests/ok/division_positive_result/")) {
        p_game_fn_initialize(2);
    } else if (starts_with(grug_file_path, "tests/ok/double_negation_with_parentheses/")) {
        p_game_fn_initialize(2);
    } else if (starts_with(grug_file_path, "tests/ok/double_not_with_parentheses/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/else_after_else_if_false/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_after_else_if_true/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_false/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_if_false/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_if_true/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_true/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/empty_line/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/entity_and_resource_as_subexpression/")) {
        p_game_fn_initialize_bool(p_game_fn_has_resource("tests/ok/entity_and_resource_as_subexpression/foo.txt") && p_game_fn_has_string("bar") && p_game_fn_has_entity("ok:baz"));
    } else if (starts_with(grug_file_path, "tests/ok/entity_duplicate/")) {
        p_game_fn_spawn("ok:foo");
        p_game_fn_spawn("ok:bar");
        p_game_fn_spawn("ok:bar");
        p_game_fn_spawn("ok:baz");
    } else if (starts_with(grug_file_path, "tests/ok/entity_in_on_fn/")) {
        p_game_fn_spawn("ok:foo");
    } else if (starts_with(grug_file_path, "tests/ok/entity_in_on_fn_with_mod_specified/")) {
        p_game_fn_spawn("wow:foo");
    } else if (starts_with(grug_file_path, "tests/ok/eq_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/eq_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_addition/")) {
        p_game_fn_sin(6.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_argument/")) {
        p_game_fn_sin(4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_division/")) {
        p_game_fn_sin(0.5f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_eq_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_eq_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ge_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ge_true_1/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ge_true_2/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_global_variable/")) {
        p_game_fn_sin(4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_gt_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_gt_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_le_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_le_true_1/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_le_true_2/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_local_variable/")) {
        p_game_fn_sin(4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_lt_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_lt_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_multiplication/")) {
        p_game_fn_sin(8.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ne_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_negated/")) {
        p_game_fn_sin(-4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ne_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_passed_to_helper_fn/")) {
        p_game_fn_sin(42.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_passed_to_on_fn/")) {
        p_game_fn_sin(42.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_passing_sin_to_cos/")) {
        p_game_fn_cos(p_game_fn_sin(4.0f));
    } else if (starts_with(grug_file_path, "tests/ok/f32_subtraction/")) {
        p_game_fn_sin(-2.0f);
    } else if (starts_with(grug_file_path, "tests/ok/fibonacci/")) {
        p_game_fn_initialize(55);
    } else if (starts_with(grug_file_path, "tests/ok/ge_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/ge_true_1/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/ge_true_2/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/global_call_using_me/")) {
        p_game_fn_set_position(1337);
    } else if (starts_with(grug_file_path, "tests/ok/global_can_use_earlier_global/")) {
        p_game_fn_initialize(5);
    } else if (starts_with(grug_file_path, "tests/ok/global_containing_negation/")) {
        p_game_fn_initialize(-2);
    } else if (starts_with(grug_file_path, "tests/ok/global_id/")) {
        p_game_fn_set_position(69);
    } else if (starts_with(grug_file_path, "tests/ok/globals/")) {
        p_game_fn_initialize(420);
        p_game_fn_initialize(1337);
    } else if (starts_with(grug_file_path, "tests/ok/gt_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/gt_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_called_in_if/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_called_indirectly/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_overwriting_param/")) {
        p_game_fn_initialize(20);
        p_game_fn_sin(30.0f);
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_returning_void_has_no_return/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_returning_void_returns_void/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_same_param_name_as_on_fn/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/helper_fn_same_param_name_as_other_helper_fn/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/i32_max/")) {
        p_game_fn_initialize(2147483647);
    } else if (starts_with(grug_file_path, "tests/ok/i32_min/")) {
        p_game_fn_initialize(-2147483648);
    } else if (starts_with(grug_file_path, "tests/ok/i32_negated/")) {
        p_game_fn_initialize(-42);
    } else if (starts_with(grug_file_path, "tests/ok/i32_negative_is_smaller_than_positive/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/id_binary_expr_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/id_binary_expr_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/id_eq_1/")) {
        p_game_fn_retrieve();
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/id_eq_2/")) {
        p_game_fn_retrieve();
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/id_global_with_id_to_new_id/")) {
        p_game_fn_store(123);
    } else if (starts_with(grug_file_path, "tests/ok/id_global_with_opponent_to_new_id/")) {
        p_game_fn_store(69);
    } else if (starts_with(grug_file_path, "tests/ok/id_helper_fn_param/")) {
        p_game_fn_store(p_game_fn_retrieve());
    } else if (starts_with(grug_file_path, "tests/ok/id_local_variable_get_and_set/")) {
        p_game_fn_set_opponent(p_game_fn_get_opponent());
    } else if (starts_with(grug_file_path, "tests/ok/id_ne_1/")) {
        p_game_fn_retrieve();
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/id_ne_2/")) {
        p_game_fn_retrieve();
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/id_on_fn_param/")) {
        p_game_fn_store(values[0].id);
    } else if (starts_with(grug_file_path, "tests/ok/id_returned_from_helper/")) {
        p_game_fn_store(42);
    } else if (starts_with(grug_file_path, "tests/ok/id_with_d_to_new_id_and_id_to_old_id/")) {
        p_game_fn_store(p_game_fn_retrieve());
    } else if (starts_with(grug_file_path, "tests/ok/id_with_d_to_old_id/")) {
        p_game_fn_store(42);
    } else if (starts_with(grug_file_path, "tests/ok/id_with_id_to_new_id/")) {
        p_game_fn_store(p_game_fn_retrieve());
    } else if (starts_with(grug_file_path, "tests/ok/if_false/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/if_true/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/le_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/le_true_1/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/le_true_2/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/local_id_can_be_reassigned/")) {
        p_game_fn_get_opponent();
        p_game_fn_get_opponent();
    } else if (starts_with(grug_file_path, "tests/ok/lt_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/lt_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/max_args/")) {
        p_game_fn_mega(1.0f, 21, true, 2.0f, 3.0f, 4.0f, false, 1337, 5.0f, 6.0f, 7.0f, 8.0f, 42, "foo");
    } else if (starts_with(grug_file_path, "tests/ok/me/")) {
        p_game_fn_set_d(42);
    } else if (starts_with(grug_file_path, "tests/ok/me_assigned_to_local_variable/")) {
        p_game_fn_set_d(42);
    } else if (starts_with(grug_file_path, "tests/ok/me_passed_to_helper_fn/")) {
        p_game_fn_set_d(42);
    } else if (starts_with(grug_file_path, "tests/ok/multiplication_as_two_arguments/")) {
        p_game_fn_max(6, 20);
    } else if (starts_with(grug_file_path, "tests/ok/ne_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/ne_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/negate_parenthesized_expr/")) {
        p_game_fn_initialize(-5);
    } else if (starts_with(grug_file_path, "tests/ok/negative_literal/")) {
        p_game_fn_initialize(-42);
    } else if (starts_with(grug_file_path, "tests/ok/nested_break/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/nested_continue/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/no_empty_line_between_statements/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_game_fn_nothing/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_game_fn_nothing_twice/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_game_fn_plt_order/")) {
        p_game_fn_nothing();
        p_game_fn_magic();
        p_game_fn_initialize(42);
        p_game_fn_identity(69);
        p_game_fn_max(1337, 8192);
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_helper_fns/")) {
        p_game_fn_nothing();
        p_game_fn_initialize(42);
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_no_game_fn/")) {
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_no_game_fn_but_with_addition/")) {
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_calling_no_game_fn_but_with_global/")) {
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_overwriting_param/")) {
        p_game_fn_initialize(20);
        p_game_fn_sin(30.0f);
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_passing_argument_to_helper_fn/")) {
        p_game_fn_initialize(42);
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_passing_magic_to_initialize/")) {
        p_game_fn_initialize(p_game_fn_magic());
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_three/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_three_unused_first/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_three_unused_second/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/on_fn_three_unused_third/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/or_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/or_short_circuit/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/or_true_1/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/or_true_2/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/or_true_3/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/pass_string_argument_to_game_fn/")) {
        p_game_fn_say("foo");
    } else if (starts_with(grug_file_path, "tests/ok/pass_string_argument_to_helper_fn/")) {
        p_game_fn_say("foo");
    } else if (starts_with(grug_file_path, "tests/ok/remainder_negative_negative/")) {
        p_game_fn_initialize(-1);
    } else if (starts_with(grug_file_path, "tests/ok/remainder_negative_positive/")) {
        p_game_fn_initialize(-1);
    } else if (starts_with(grug_file_path, "tests/ok/remainder_positive_negative/")) {
        p_game_fn_initialize(1);
    } else if (starts_with(grug_file_path, "tests/ok/remainder_positive_positive/")) {
        p_game_fn_initialize(1);
    } else if (starts_with(grug_file_path, "tests/ok/resource_and_entity/")) {
        p_game_fn_draw("tests/ok/resource_and_entity/foo.txt");
        p_game_fn_spawn("ok:foo");
    } else if (starts_with(grug_file_path, "tests/ok/resource_can_contain_dot_1/")) {
        p_game_fn_draw("tests/ok/resource_can_contain_dot_1/.foo");
    } else if (starts_with(grug_file_path, "tests/ok/resource_can_contain_dot_2/")) {
        p_game_fn_draw("tests/ok/resource_can_contain_dot_2/foo.");
    } else if (starts_with(grug_file_path, "tests/ok/resource_can_contain_dot_3/")) {
        p_game_fn_draw("tests/ok/resource_can_contain_dot_3/foo.bar");
    } else if (starts_with(grug_file_path, "tests/ok/resource_can_contain_dot_dot_1/")) {
        p_game_fn_draw("tests/ok/resource_can_contain_dot_dot_1/..foo");
    } else if (starts_with(grug_file_path, "tests/ok/resource_can_contain_dot_dot_2/")) {
        p_game_fn_draw("tests/ok/resource_can_contain_dot_dot_2/foo..");
    } else if (starts_with(grug_file_path, "tests/ok/resource_can_contain_dot_dot_3/")) {
        p_game_fn_draw("tests/ok/resource_can_contain_dot_dot_3/foo..bar");
    } else if (starts_with(grug_file_path, "tests/ok/resource_duplicate/")) {
        p_game_fn_draw("tests/ok/resource_duplicate/foo.txt");
        p_game_fn_draw("tests/ok/resource_duplicate/bar.txt");
        p_game_fn_draw("tests/ok/resource_duplicate/bar.txt");
        p_game_fn_draw("tests/ok/resource_duplicate/baz.txt");
    } else if (starts_with(grug_file_path, "tests/ok/return/")) {
        p_game_fn_initialize(42);
    } else if (starts_with(grug_file_path, "tests/ok/return_from_on_fn/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/return_from_on_fn_minimal/")) {
    } else if (starts_with(grug_file_path, "tests/ok/return_with_no_value/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/same_variable_name_in_different_functions/")) {
        p_game_fn_initialize(42);
        p_game_fn_initialize(69);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_game_fn/")) {
        p_game_fn_motherload(1, 2, 3, 4, 5, 6, 7, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 42, 9.0f);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_game_fn_subless/")) {
        p_game_fn_motherload_subless(1, 2, 3, 4, 5, 6, 7, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 42, 10.0f);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_helper_fn/")) {
        p_game_fn_motherload(1, 2, 3, 4, 5, 6, 7, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 42, 9.0f);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_helper_fn_32_bit_f32/")) {
        p_game_fn_offset_32_bit_f32("1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 1);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_helper_fn_32_bit_i32/")) {
        p_game_fn_offset_32_bit_i32(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 1, 2, 3, 4, 5, 6);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_helper_fn_32_bit_string/")) {
        p_game_fn_offset_32_bit_string(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, "1", "2", "3", "4", "5", 1);
    } else if (starts_with(grug_file_path, "tests/ok/spill_args_to_helper_fn_subless/")) {
        p_game_fn_motherload_subless(1, 2, 3, 4, 5, 6, 7, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 42, 10.0f);
    } else if (starts_with(grug_file_path, "tests/ok/stack_16_byte_alignment/")) {
        p_game_fn_nothing();
        p_game_fn_initialize(42);
    } else if (starts_with(grug_file_path, "tests/ok/stack_16_byte_alignment_midway/")) {
        p_game_fn_initialize(p_game_fn_magic() + 42);
    } else if (starts_with(grug_file_path, "tests/ok/string_can_be_passed_to_helper_fn/")) {
        p_game_fn_say("foo");
    } else if (starts_with(grug_file_path, "tests/ok/string_duplicate/")) {
        p_game_fn_talk("foo", "bar", "bar", "baz");
    } else if (starts_with(grug_file_path, "tests/ok/string_eq_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/string_eq_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/string_eq_true_empty/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/string_ne_false/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/string_ne_false_empty/")) {
        p_game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/string_ne_true/")) {
        p_game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/sub_rsp_32_bits_local_variables_i32/")) {
        for (int32_t n = 1; n <= 30; n++) {
            p_game_fn_initialize(30);
        }
    } else if (starts_with(grug_file_path, "tests/ok/sub_rsp_32_bits_local_variables_id/")) {
        for (size_t i = 0; i < 15; i++) {
            p_game_fn_set_d(42);
        }
    } else if (starts_with(grug_file_path, "tests/ok/subtraction_negative_result/")) {
        p_game_fn_initialize(-3);
    } else if (starts_with(grug_file_path, "tests/ok/subtraction_positive_result/")) {
        p_game_fn_initialize(3);
    } else if (starts_with(grug_file_path, "tests/ok/variable/")) {
        p_game_fn_initialize(42);
    } else if (starts_with(grug_file_path, "tests/ok/variable_does_not_shadow_in_different_if_statement/")) {
        p_game_fn_initialize(42);
        p_game_fn_initialize(69);
    } else if (starts_with(grug_file_path, "tests/ok/variable_reassignment/")) {
        p_game_fn_initialize(69);
    } else if (starts_with(grug_file_path, "tests/ok/variable_reassignment_does_not_dealloc_outer_variable/")) {
        p_game_fn_initialize(69);
    } else if (starts_with(grug_file_path, "tests/ok/variable_string_global/")) {
        p_game_fn_say("foo");
    } else if (starts_with(grug_file_path, "tests/ok/variable_string_local/")) {
        p_game_fn_say("foo");
    } else if (starts_with(grug_file_path, "tests/ok/void_function_early_return/")) {
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/while_false/")) {
        p_game_fn_nothing();
        p_game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/write_to_global_variable/")) {
        p_game_fn_max(43, 69);
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

static void *load_sym(void *h, const char *name) {
    void *p = dlsym(h, name);
    assert(p && "Failed to load required symbol from tests.so");
    return p;
}

static void load_tests_so(void) {
    void *h = dlopen("./tests.so", RTLD_NOW | RTLD_LOCAL);
    assert(h && "Could not load tests.so");

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
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
    p_game_fn_get_evil_false = load_sym(h, "game_fn_get_evil_false");
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
    p_game_fn_box_i32 = load_sym(h, "game_fn_box_i32");
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
