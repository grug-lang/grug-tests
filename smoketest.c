#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool starts_with(const char *haystack, const char *needle) {
	return strncmp(haystack, needle, strlen(needle)) == 0;
}

// TODO: Implement
static const char *compile_grug_file(const char *grug_file_path) {
    printf("grug_file_path: %s\n", grug_file_path); // TODO: REMOVE!

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

// TODO: Implement
static void on_fn_dispatcher(const char *on_fn_name, const char *grug_file_path, struct grug_value values[], size_t value_count) {
    (void)on_fn_name;
    (void)grug_file_path;
    (void)values;
    (void)value_count;
}

// TODO: Implement
static bool dump_file_to_json(const char *input_grug_path, const char *output_json_path) {
    (void)input_grug_path;
    (void)output_json_path;
    return false;
}

// TODO: Implement
static bool generate_file_from_json(const char *input_json_path, const char *output_grug_path) {
    (void)input_json_path;
    (void)output_grug_path;
    return false;
}

// TODO: Implement
static void game_fn_error(const char *message) {
    (void)message;
}

int main(void) {
    grug_tests_run(compile_grug_file, init_globals_fn_dispatcher, on_fn_dispatcher, dump_file_to_json, generate_file_from_json, game_fn_error, NULL);
}
