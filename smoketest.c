#include "tests.h"

// TODO: Update tests.sh to run this, where CC must be used to pick between clang and gcc:
// clang -o smoketest smoketest.c tests.c -Wall -Wextra -pedantic -Werror -Wfatal-errors -fsanitize=address,undefined -g -lm && ./smoketest

// TODO: Implement
const char *compile_grug_file(const char *grug_file_path) {
    (void)grug_file_path;
    return NULL;
}

// TODO: Implement
void init_globals_fn_dispatcher(const char *grug_file_path) {
    (void)grug_file_path;
}

// TODO: Implement
void on_fn_dispatcher(const char *on_fn_name, const char *grug_file_path, struct grug_value values[], size_t value_count) {
    (void)on_fn_name;
    (void)grug_file_path;
    (void)values;
    (void)value_count;
}

// TODO: Implement
bool dump_file_to_json(const char *input_grug_path, const char *output_json_path) {
    (void)input_grug_path;
    (void)output_json_path;
    return false;
}

// TODO: Implement
bool generate_file_from_json(const char *input_json_path, const char *output_grug_path) {
    (void)input_json_path;
    (void)output_grug_path;
    return false;
}

// TODO: Implement
void game_fn_error(const char *message) {
    (void)message;
}

int main(void) {
    grug_tests_run(compile_grug_file, init_globals_fn_dispatcher, on_fn_dispatcher, dump_file_to_json, generate_file_from_json, game_fn_error, NULL);
}
