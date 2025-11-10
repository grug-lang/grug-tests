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

    // TODO: Refactor using a macro
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
