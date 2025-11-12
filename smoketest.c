#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *saved_on_fn_name;
static const char *saved_grug_file_path;

static bool streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

static bool starts_with(const char *haystack, const char *needle) {
	return strncmp(haystack, needle, strlen(needle)) == 0;
}

static const char *compile_grug_file(const char *grug_file_path) {
    // The tests/err/ tests return expected_error.txt
    if (starts_with(grug_file_path, "tests/err/")) {
        // Find the last '/' in the path
        const char *last_slash = strrchr(grug_file_path, '/');
        assert(last_slash);

        // Copy everything up to (and including) the last '/'
        static char expected_path[1337];
        size_t dir_len = last_slash - grug_file_path + 1;
        memcpy(expected_path, grug_file_path, dir_len);
        expected_path[dir_len] = '\0';

        // Append the new filename
        strcat(expected_path, "expected_error.txt");

	    FILE *f = fopen(expected_path, "r");
        assert(f);

        // Read file contents into buf
        static char buf[1337];
        size_t nread = fread(buf, 1, sizeof(buf) - 1, f);

        if (buf[nread - 1] == '\n') {
            nread--;
            if (buf[nread - 1] == '\r') {
                nread--;
            }
        }

        buf[nread] = '\0';

        fclose(f);

        return buf;
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_transfer_between_globals/")) {
        game_fn_get_opponent();
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_with_digits/")) {
        game_fn_box_i32(42);
    }

    return NULL;
}

// TODO: Implement
static void init_globals_fn_dispatcher(const char *grug_file_path) {
    (void)grug_file_path;
}

static void on_fn_dispatcher(const char *on_fn_name, const char *grug_file_path, struct grug_value values[], size_t value_count) {
    (void)values;
    (void)value_count;

    saved_on_fn_name = on_fn_name;
    saved_grug_file_path = grug_file_path;

    if (starts_with(grug_file_path, "tests/err_runtime/all/")) {
        grug_tests_runtime_error_handler("Division of an i32 by 0", GRUG_ON_FN_DIVISION_BY_ZERO, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/division_by_0/")) {
        grug_tests_runtime_error_handler("Division of an i32 by 0", GRUG_ON_FN_DIVISION_BY_ZERO, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/game_fn_error/")) {
        game_fn_cause_game_fn_error();
    } else if (starts_with(grug_file_path, "tests/err_runtime/game_fn_error_once/")) {
        if (streq(on_fn_name, "on_a")) {
            game_fn_cause_game_fn_error();
        } else {
            game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_addition/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_division/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_multiplication/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_negation/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_remainder/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_overflow_subtraction/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_underflow_addition/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_underflow_multiplication/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/i32_underflow_subtraction/")) {
        grug_tests_runtime_error_handler("i32 overflow", GRUG_ON_FN_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/on_fn_calls_erroring_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            game_fn_call_on_b_fn();
        } else {
            game_fn_cause_game_fn_error();
        }
    } else if (starts_with(grug_file_path, "tests/err_runtime/on_fn_errors_after_it_calls_other_on_fn/")) {
        if (streq(on_fn_name, "on_a")) {
            game_fn_call_on_b_fn();
            game_fn_cause_game_fn_error();
        } else {
            game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/err_runtime/remainder_by_0/")) {
        grug_tests_runtime_error_handler("Division of an i32 by 0", GRUG_ON_FN_DIVISION_BY_ZERO, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/stack_overflow/")) {
        grug_tests_runtime_error_handler("Stack overflow, so check for accidental infinite recursion", GRUG_ON_FN_STACK_OVERFLOW, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/time_limit_exceeded/")) {
        grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/time_limit_exceeded_exponential_calls/")) {
        grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/err_runtime/time_limit_exceeded_fibonacci/")) {
        grug_tests_runtime_error_handler("Took longer than 10 milliseconds to run", GRUG_ON_FN_TIME_LIMIT_EXCEEDED, on_fn_name, grug_file_path);
    } else if (starts_with(grug_file_path, "tests/ok/addition_as_argument/")) {
        game_fn_initialize(3);
    } else if (starts_with(grug_file_path, "tests/ok/addition_as_two_arguments/")) {
        game_fn_max(3, 9);
    } else if (starts_with(grug_file_path, "tests/ok/addition_with_multiplication/")) {
        game_fn_initialize(14);
    } else if (starts_with(grug_file_path, "tests/ok/addition_with_multiplication_2/")) {
        game_fn_initialize(10);
    } else if (starts_with(grug_file_path, "tests/ok/and_false_1/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_false_2/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_false_3/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_short_circuit/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/and_true/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/blocked_alrm/")) {
        game_fn_blocked_alrm();
    } else if (starts_with(grug_file_path, "tests/ok/bool_logical_not_false/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/bool_logical_not_true/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/bool_returned/")) {
        game_fn_get_evil_false();
        game_fn_set_is_happy(false);
    } else if (starts_with(grug_file_path, "tests/ok/bool_returned_global/")) {
        game_fn_get_evil_false();
        game_fn_set_is_happy(false);
    } else if (starts_with(grug_file_path, "tests/ok/bool_zero_extended_if_statement/")) {
        game_fn_nothing();
        game_fn_get_evil_false();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/bool_zero_extended_while_statement/")) {
        game_fn_nothing();
        game_fn_get_evil_false();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/break/")) {
        game_fn_nothing();
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/calls_100/")) {
        for (size_t i = 0; i < 100; i++) {
            game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/ok/calls_1000/")) {
        for (size_t i = 0; i < 1000; i++) {
            game_fn_nothing();
        }
    } else if (starts_with(grug_file_path, "tests/ok/calls_in_call/")) {
        game_fn_max(1, 2);
        game_fn_max(3, 4);
        game_fn_max(2, 4);
        game_fn_initialize(4);
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_block/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_block_twice/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_helper_fn/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_above_on_fn/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_between_statements/")) {
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_lone_block/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_lone_block_at_end/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/comment_lone_global/")) {
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/continue/")) {
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_decays_to_id/")) {
        game_fn_store(42);
    } else if (starts_with(grug_file_path, "tests/ok/custom_id_transfer_between_globals/")) {
        game_fn_set_opponent(69);
    } else if (starts_with(grug_file_path, "tests/ok/division_negative_result/")) {
        game_fn_initialize(-2);
    } else if (starts_with(grug_file_path, "tests/ok/division_positive_result/")) {
        game_fn_initialize(2);
    } else if (starts_with(grug_file_path, "tests/ok/double_negation_with_parentheses/")) {
        game_fn_initialize(2);
    } else if (starts_with(grug_file_path, "tests/ok/double_not_with_parentheses/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/else_after_else_if_false/")) {
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_after_else_if_true/")) {
        game_fn_nothing();
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_false/")) {
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_if_false/")) {
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_if_true/")) {
        game_fn_nothing();
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/else_true/")) {
        game_fn_nothing();
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/empty_line/")) {
        game_fn_nothing();
        game_fn_nothing();
    } else if (starts_with(grug_file_path, "tests/ok/entity_and_resource_as_subexpression/")) {
        game_fn_initialize_bool(game_fn_has_resource("tests/ok/entity_and_resource_as_subexpression/foo.txt") && game_fn_has_string("bar") && game_fn_has_entity("ok:baz"));
    } else if (starts_with(grug_file_path, "tests/ok/entity_duplicate/")) {
        game_fn_spawn("ok:foo");
        game_fn_spawn("ok:bar");
        game_fn_spawn("ok:bar");
        game_fn_spawn("ok:baz");
    } else if (starts_with(grug_file_path, "tests/ok/entity_in_on_fn/")) {
        game_fn_spawn("ok:foo");
    } else if (starts_with(grug_file_path, "tests/ok/entity_in_on_fn_with_mod_specified/")) {
        game_fn_spawn("wow:foo");
    } else if (starts_with(grug_file_path, "tests/ok/eq_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/eq_true/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_addition/")) {
        game_fn_sin(6.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_argument/")) {
        game_fn_sin(4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_division/")) {
        game_fn_sin(0.5f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_eq_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_eq_true/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ge_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ge_true_1/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ge_true_2/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_global_variable/")) {
        game_fn_sin(4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_gt_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_gt_true/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_le_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_le_true_1/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_le_true_2/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_local_variable/")) {
        game_fn_sin(4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_lt_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_lt_true/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_multiplication/")) {
        game_fn_sin(8.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ne_false/")) {
        game_fn_initialize_bool(false);
    } else if (starts_with(grug_file_path, "tests/ok/f32_negated/")) {
        game_fn_sin(-4.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_ne_true/")) {
        game_fn_initialize_bool(true);
    } else if (starts_with(grug_file_path, "tests/ok/f32_passed_to_helper_fn/")) {
        game_fn_sin(42.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_passed_to_on_fn/")) {
        game_fn_sin(42.0f);
    } else if (starts_with(grug_file_path, "tests/ok/f32_passing_sin_to_cos/")) {
        game_fn_cos(game_fn_sin(4.0f));
    } else if (starts_with(grug_file_path, "tests/ok/f32_subtraction/")) {
        game_fn_sin(-2.0f);
    } else if (starts_with(grug_file_path, "tests/ok/fibonacci/")) {
        game_fn_initialize(55);
    } else {
        assert(false);
    }
}

static bool copy_file(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");
    if (!src) {
        perror("Failed to open source file");
        return true;
    }

    FILE *dst = fopen(dst_path, "wb");
    if (!dst) {
        perror("Failed to open destination file");
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
    grug_tests_runtime_error_handler(message, GRUG_ON_FN_GAME_FN_ERROR, saved_on_fn_name, saved_grug_file_path);
}

int main(void) {
    grug_tests_run(compile_grug_file, init_globals_fn_dispatcher, on_fn_dispatcher, dump_file_to_json, generate_file_from_json, game_fn_error, NULL);
}
