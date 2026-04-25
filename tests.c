#include "tests.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define assert_call_count(game_fn_name, expected_count) do { \
	size_t count = game_fn_ ## game_fn_name ## _call_count; \
	if (count != expected_count) { \
		fprintf(stderr, "%s:%d: Assertion call count %zu (%s) == %d failed.\n", __FILE__, __LINE__, count, #game_fn_name, expected_count); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define assert_error_handler_call_count(expected_count) do { \
	if (error_handler_call_count != expected_count) { \
		fprintf(stderr, "%s:%d: Assertion error handler call count %zu == %d failed.\n", __FILE__, __LINE__, error_handler_call_count, expected_count); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define assert_number(d, expected_number) do { \
	if (d != expected_number) { \
		fprintf(stderr, "%s:%d: Assertion %f (%s) == %f failed.\n", __FILE__, __LINE__, d, #d, expected_number); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define assert_true(b) do { \
	if (!b) { \
		fprintf(stderr, "%s:%d: Assertion %s == true failed.\n", __FILE__, __LINE__, #b); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define assert_false(b) do { \
	if (b) { \
		fprintf(stderr, "%s:%d: Assertion %s == false failed.\n", __FILE__, __LINE__, #b); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define assert_string(str, expected_str) do { \
	if (!streq(str, expected_str)) { \
		fprintf(stderr, "%s:%d: Assertion '%s' (%s) == '%s' failed.\n", __FILE__, __LINE__, str, #str, expected_str); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#define assert_id(id, expected_id) do { \
	if (id != expected_id) { \
		fprintf(stderr, "%s:%d: Assertion ID %"PRIu64" (%s) == %d failed.\n", __FILE__, __LINE__, id, #id, expected_id); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

static const char *get_type_name[] = {
	[GRUG_ON_FN_STACK_OVERFLOW] = "GRUG_ON_FN_STACK_OVERFLOW",
	[GRUG_ON_FN_TIME_LIMIT_EXCEEDED] = "GRUG_ON_FN_TIME_LIMIT_EXCEEDED",
	[GRUG_ON_FN_GAME_FN_ERROR] = "GRUG_ON_FN_GAME_FN_ERROR",
};

#define assert_runtime_error_type(expected_type) do { \
	if (runtime_error_type != expected_type) { \
		fprintf(stderr, "%s:%d: Assertion runtime error type %s == %s failed.\n", __FILE__, __LINE__, get_type_name[runtime_error_type], get_type_name[expected_type]); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

#if defined(_WIN32)
#include <windows.h>
#define SLASH "\\"
#elif defined(__linux__)
#include <sys/stat.h>
#define SLASH "/"
#endif

// Most implementations shouldn't pass -DASSERT_ALIGNMENT.
// It caught a ton of stack misalignment bugs
// in the original version of grug.c, as it emitted raw machine code.
// Note that ASSERT_ALIGNMENT breaks with -DCMAKE_BUILD_TYPE=Release.
#ifdef ASSERT_ALIGNMENT

// From https://stackoverflow.com/a/2114249/13279557
#ifdef __x86_64__
#define ASSERT_16_BYTE_STACK_ALIGNED() do {\
	int64_t rsp;\
	\
	__asm__ volatile("mov %%rsp, %0\n\t" : "=r" (rsp));\
	\
	if ((rsp & 0xf) != 0) {\
		static char msg[] = "The stack was not 16-byte aligned!\n";\
		_Pragma("GCC diagnostic push")\
		_Pragma("GCC diagnostic ignored \"-Wunused-result\"")\
		write(STDERR_FILENO, msg, sizeof(msg) - 1);\
		_Pragma("GCC diagnostic pop")\
		abort();\
	}\
} while (0)
#elif __aarch64__
#define ASSERT_16_BYTE_STACK_ALIGNED() do {\
	int64_t rsp;\
	\
	__asm__ volatile("mov %0, sp\n\t" : "=r" (rsp));\
	\
	if ((rsp & 0xf) != 0) {\
		static char msg[] = "The stack was not 16-byte aligned!\n";\
		_Pragma("GCC diagnostic push")\
		_Pragma("GCC diagnostic ignored \"-Wunused-result\"")\
		write(STDERR_FILENO, msg, sizeof(msg) - 1);\
		_Pragma("GCC diagnostic pop")\
		abort();\
	}\
} while (0)
#else
#error Unrecognized architecture
#endif

#else // ASSERT_ALIGNMENT

#define ASSERT_16_BYTE_STACK_ALIGNED()

#endif // ASSERT_ALIGNMENT

static const char *tests_dir_path;
static const char *mod_api_path;
static const char *whitelisted_test;

static struct grug_file_id* current_file_id;

static create_grug_state_t       create_grug_state;
static destroy_grug_state_t      destroy_grug_state;
static compile_grug_file_t       compile_grug_file;
static init_globals_t            init_globals;
static call_export_fn_t          call_export_fn;
static dump_file_to_json_t       dump_file_to_json;
static generate_file_from_json_t generate_file_from_json;
static game_fn_error_t           game_fn_error;

struct error_test_data {
	const char *test_name_str;
	const char *grug_path;
	const char *expected_error_path;
	const char *results_path;
	const char *grug_output_path;
};
static struct error_test_data error_test_datas[420420];
static size_t err_test_datas_size;

struct ok_test_data {
	void (*run)(void* grug_state, void* file_id);
	struct grug_file_id* file_id;
	const char *test_name_str;
	const char *grug_path;
	const char *results_path;
	const char *dump_path;
	const char *applied_path;
};
static struct ok_test_data ok_test_datas[420420];
static size_t ok_test_datas_size;

struct runtime_error_test_data {
	void (*run)(void* grug_state, void* file_id);
	struct grug_file_id* file_id;
	const char *test_name_str;
	const char *grug_path;
	const char *expected_error_path;
	const char *results_path;
	const char *dump_path;
	const char *applied_path;
};
static struct runtime_error_test_data runtime_error_test_datas[420420];
static size_t err_runtime_test_datas_size;

static size_t game_fn_nothing_call_count;
static size_t game_fn_magic_call_count;
static size_t game_fn_initialize_call_count;
static size_t game_fn_initialize_bool_call_count;
static size_t game_fn_identity_call_count;
static size_t game_fn_max_call_count;
static size_t game_fn_say_call_count;
static size_t game_fn_sin_call_count;
static size_t game_fn_cos_call_count;
static size_t game_fn_mega_call_count;
static size_t game_fn_get_false_call_count;
static size_t game_fn_set_is_happy_call_count;
static size_t game_fn_mega_f32_call_count;
static size_t game_fn_mega_i32_call_count;
static size_t game_fn_draw_call_count;
static size_t game_fn_blocked_alrm_call_count;
static size_t game_fn_spawn_call_count;
static size_t game_fn_spawn_d_call_count;
static size_t game_fn_has_resource_call_count;
static size_t game_fn_has_entity_call_count;
static size_t game_fn_has_string_call_count;
static size_t game_fn_get_opponent_call_count;
static size_t game_fn_get_os_call_count;
static size_t game_fn_set_d_call_count;
static size_t game_fn_set_opponent_call_count;
static size_t game_fn_motherload_call_count;
static size_t game_fn_motherload_subless_call_count;
static size_t game_fn_offset_32_bit_f32_call_count;
static size_t game_fn_offset_32_bit_i32_call_count;
static size_t game_fn_offset_32_bit_string_call_count;
static size_t game_fn_talk_call_count;
static size_t game_fn_get_position_call_count;
static size_t game_fn_set_position_call_count;
static size_t game_fn_cause_game_fn_error_call_count;
static size_t game_fn_call_on_b_fn_call_count;
static size_t game_fn_store_call_count;
static size_t game_fn_print_csv_call_count;
static size_t game_fn_retrieve_call_count;
static size_t game_fn_box_number_call_count;

static bool had_runtime_error = false;
static size_t error_handler_call_count = 0;
static char runtime_error_reason[256];
static enum grug_runtime_error_type runtime_error_type = 0;
static char runtime_error_on_fn_name[256];
static char runtime_error_on_fn_path[256];

static bool streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

static void call_export_fn_argless(void* grug_state, void* file_id, const char *on_fn_name) {
	call_export_fn(grug_state, file_id, on_fn_name, NULL, 0);
}

union grug_value game_fn_nothing(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_nothing_call_count++;
	return (union grug_value) {0};
}
union grug_value game_fn_magic(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_magic_call_count++;

	return grug_number(42.0);
}
static double game_fn_initialize_x;
union grug_value game_fn_initialize(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_initialize_call_count++;

	game_fn_initialize_x = args[0]._number;
	return (union grug_value) {0};
}
static bool game_fn_initialize_bool_b;
union grug_value game_fn_initialize_bool(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_initialize_bool_call_count++;

	game_fn_initialize_bool_b = args[0]._bool;
	return (union grug_value) {0};
}
static double game_fn_identity_x;
union grug_value game_fn_identity(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_identity_call_count++;

	game_fn_identity_x = args[0]._number;

	return args[0];
}
static double game_fn_max_x;
static double game_fn_max_y;
union grug_value game_fn_max(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_max_call_count++;

	game_fn_max_x = args[0]._number;
	game_fn_max_y = args[1]._number;

	return args[0]._number > args[1]._number ? args[0] : args[1];
}
static char game_fn_say_message[256];
union grug_value game_fn_say(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_say_call_count++;

	strcpy(game_fn_say_message, args[0]._string);
	return (union grug_value) {0};
}
static double game_fn_sin_x;
union grug_value game_fn_sin(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_sin_call_count++;

	game_fn_sin_x = args[0]._number;

	return grug_number(sin(args[0]._number));
}
static double game_fn_cos_x;
union grug_value game_fn_cos(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_cos_call_count++;

	game_fn_cos_x = args[0]._number;

	return grug_number(cos(args[0]._number));
}
static double game_fn_mega_f1;
static double game_fn_mega_i1;
static bool game_fn_mega_b1;
static double game_fn_mega_f2;
static double game_fn_mega_f3;
static double game_fn_mega_f4;
static bool game_fn_mega_b2;
static double game_fn_mega_i2;
static double game_fn_mega_f5;
static double game_fn_mega_f6;
static double game_fn_mega_f7;
static double game_fn_mega_f8;
static uint64_t game_fn_mega_id;
static char game_fn_mega_str[256];
union grug_value game_fn_mega(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_mega_call_count++;

	game_fn_mega_f1 = args[0]._number;
	game_fn_mega_i1 = args[1]._number;
	game_fn_mega_b1 = args[2]._bool;
	game_fn_mega_f2 = args[3]._number;
	game_fn_mega_f3 = args[4]._number;
	game_fn_mega_f4 = args[5]._number;
	game_fn_mega_b2 = args[6]._bool;
	game_fn_mega_i2 = args[7]._number;
	game_fn_mega_f5 = args[8]._number;
	game_fn_mega_f6 = args[9]._number;
	game_fn_mega_f7 = args[10]._number;
	game_fn_mega_f8 = args[11]._number;
	game_fn_mega_id = args[12]._id;
	strcpy(game_fn_mega_str, args[13]._string);
	return (union grug_value) {0};
}
union grug_value game_fn_get_false(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_false_call_count++;

	return grug_bool(false);
}
static bool game_fn_set_is_happy_is_happy;
union grug_value game_fn_set_is_happy(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_is_happy_call_count++;

	game_fn_set_is_happy_is_happy = args[0]._bool;
	return (union grug_value) {0};
}
static double game_fn_mega_f32_f1;
static double game_fn_mega_f32_f2;
static double game_fn_mega_f32_f3;
static double game_fn_mega_f32_f4;
static double game_fn_mega_f32_f5;
static double game_fn_mega_f32_f6;
static double game_fn_mega_f32_f7;
static double game_fn_mega_f32_f8;
static double game_fn_mega_f32_f9;
union grug_value game_fn_mega_f32(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_mega_f32_call_count++;

	game_fn_mega_f32_f1 = args[0]._number;
	game_fn_mega_f32_f2 = args[1]._number;
	game_fn_mega_f32_f3 = args[2]._number;
	game_fn_mega_f32_f4 = args[3]._number;
	game_fn_mega_f32_f5 = args[4]._number;
	game_fn_mega_f32_f6 = args[5]._number;
	game_fn_mega_f32_f7 = args[6]._number;
	game_fn_mega_f32_f8 = args[7]._number;
	game_fn_mega_f32_f9 = args[8]._number;
	return (union grug_value) {0};
}
static double game_fn_mega_i32_i1;
static double game_fn_mega_i32_i2;
static double game_fn_mega_i32_i3;
static double game_fn_mega_i32_i4;
static double game_fn_mega_i32_i5;
static double game_fn_mega_i32_i6;
static double game_fn_mega_i32_i7;
union grug_value game_fn_mega_i32(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_mega_i32_call_count++;

	game_fn_mega_i32_i1 = args[0]._number;
	game_fn_mega_i32_i2 = args[1]._number;
	game_fn_mega_i32_i3 = args[2]._number;
	game_fn_mega_i32_i4 = args[3]._number;
	game_fn_mega_i32_i5 = args[4]._number;
	game_fn_mega_i32_i6 = args[5]._number;
	game_fn_mega_i32_i7 = args[6]._number;
	return (union grug_value) {0};
}
static char game_fn_draw_sprite_path[256];
union grug_value game_fn_draw(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_draw_call_count++;
	strcpy(game_fn_draw_sprite_path, args[0]._string);
	return (union grug_value) {0};
}
union grug_value game_fn_blocked_alrm(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_blocked_alrm_call_count++;
	return (union grug_value) {0};
}
static char game_fn_spawn_name[256];
union grug_value game_fn_spawn(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_spawn_call_count++;

	strcpy(game_fn_spawn_name, args[0]._string);
	return (union grug_value) {0};
}
static char game_fn_spawn_d_name[256];
union grug_value game_fn_spawn_d(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_spawn_d_call_count++;

	strcpy(game_fn_spawn_d_name, args[0]._string);
	return (union grug_value) {0};
}
static char game_fn_has_resource_path[256];
union grug_value game_fn_has_resource(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_has_resource_call_count++;

	strcpy(game_fn_has_resource_path, args[0]._string);

	return grug_bool(true);
}
static char game_fn_has_entity_name[256];
union grug_value game_fn_has_entity(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_has_entity_call_count++;

	strcpy(game_fn_has_entity_name, args[0]._string);

	return grug_bool(true);
}
static char game_fn_has_string_str[256];
union grug_value game_fn_has_string(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_has_string_call_count++;

	strcpy(game_fn_has_string_str, args[0]._string);

	return grug_bool(true);
}
union grug_value game_fn_get_opponent(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_opponent_call_count++;

	return grug_id(69);
}
union grug_value game_fn_get_os(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_os_call_count++;

	return grug_string("foo");
}
static uint64_t game_fn_set_d_target;
union grug_value game_fn_set_d(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_d_call_count++;

	game_fn_set_d_target = args[0]._id;
	return (union grug_value) {0};
}
static uint64_t game_fn_set_opponent_target;
union grug_value game_fn_set_opponent(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_opponent_call_count++;

	game_fn_set_opponent_target = args[0]._id;
	return (union grug_value) {0};
}
static double game_fn_motherload_i1;
static double game_fn_motherload_i2;
static double game_fn_motherload_i3;
static double game_fn_motherload_i4;
static double game_fn_motherload_i5;
static double game_fn_motherload_i6;
static double game_fn_motherload_i7;
static double game_fn_motherload_f1;
static double game_fn_motherload_f2;
static double game_fn_motherload_f3;
static double game_fn_motherload_f4;
static double game_fn_motherload_f5;
static double game_fn_motherload_f6;
static double game_fn_motherload_f7;
static double game_fn_motherload_f8;
static uint64_t game_fn_motherload_id;
static double game_fn_motherload_f9;
union grug_value game_fn_motherload(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_motherload_call_count++;

	game_fn_motherload_i1 = args[0]._number;
	game_fn_motherload_i2 = args[1]._number;
	game_fn_motherload_i3 = args[2]._number;
	game_fn_motherload_i4 = args[3]._number;
	game_fn_motherload_i5 = args[4]._number;
	game_fn_motherload_i6 = args[5]._number;
	game_fn_motherload_i7 = args[6]._number;
	game_fn_motherload_f1 = args[7]._number;
	game_fn_motherload_f2 = args[8]._number;
	game_fn_motherload_f3 = args[9]._number;
	game_fn_motherload_f4 = args[10]._number;
	game_fn_motherload_f5 = args[11]._number;
	game_fn_motherload_f6 = args[12]._number;
	game_fn_motherload_f7 = args[13]._number;
	game_fn_motherload_f8 = args[14]._number;
	game_fn_motherload_id = args[15]._id;
	game_fn_motherload_f9 = args[16]._number;
	return (union grug_value) {0};
}
static double game_fn_motherload_subless_i1;
static double game_fn_motherload_subless_i2;
static double game_fn_motherload_subless_i3;
static double game_fn_motherload_subless_i4;
static double game_fn_motherload_subless_i5;
static double game_fn_motherload_subless_i6;
static double game_fn_motherload_subless_i7;
static double game_fn_motherload_subless_f1;
static double game_fn_motherload_subless_f2;
static double game_fn_motherload_subless_f3;
static double game_fn_motherload_subless_f4;
static double game_fn_motherload_subless_f5;
static double game_fn_motherload_subless_f6;
static double game_fn_motherload_subless_f7;
static double game_fn_motherload_subless_f8;
static double game_fn_motherload_subless_f9;
static uint64_t game_fn_motherload_subless_id;
static double game_fn_motherload_subless_f10;
union grug_value game_fn_motherload_subless(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_motherload_subless_call_count++;

	game_fn_motherload_subless_i1 = args[0]._number;
	game_fn_motherload_subless_i2 = args[1]._number;
	game_fn_motherload_subless_i3 = args[2]._number;
	game_fn_motherload_subless_i4 = args[3]._number;
	game_fn_motherload_subless_i5 = args[4]._number;
	game_fn_motherload_subless_i6 = args[5]._number;
	game_fn_motherload_subless_i7 = args[6]._number;
	game_fn_motherload_subless_f1 = args[7]._number;
	game_fn_motherload_subless_f2 = args[8]._number;
	game_fn_motherload_subless_f3 = args[9]._number;
	game_fn_motherload_subless_f4 = args[10]._number;
	game_fn_motherload_subless_f5 = args[11]._number;
	game_fn_motherload_subless_f6 = args[12]._number;
	game_fn_motherload_subless_f7 = args[13]._number;
	game_fn_motherload_subless_f8 = args[14]._number;
	game_fn_motherload_subless_f9 = args[15]._number;
	game_fn_motherload_subless_id = args[16]._id;
	game_fn_motherload_subless_f10 = args[17]._number;
	return (union grug_value) {0};
}
static char game_fn_offset_32_bit_f32_s1[256];
static char game_fn_offset_32_bit_f32_s2[256];
static char game_fn_offset_32_bit_f32_s3[256];
static char game_fn_offset_32_bit_f32_s4[256];
static char game_fn_offset_32_bit_f32_s5[256];
static char game_fn_offset_32_bit_f32_s6[256];
static char game_fn_offset_32_bit_f32_s7[256];
static char game_fn_offset_32_bit_f32_s8[256];
static char game_fn_offset_32_bit_f32_s9[256];
static char game_fn_offset_32_bit_f32_s10[256];
static char game_fn_offset_32_bit_f32_s11[256];
static char game_fn_offset_32_bit_f32_s12[256];
static char game_fn_offset_32_bit_f32_s13[256];
static char game_fn_offset_32_bit_f32_s14[256];
static char game_fn_offset_32_bit_f32_s15[256];
static double game_fn_offset_32_bit_f32_f1;
static double game_fn_offset_32_bit_f32_f2;
static double game_fn_offset_32_bit_f32_f3;
static double game_fn_offset_32_bit_f32_f4;
static double game_fn_offset_32_bit_f32_f5;
static double game_fn_offset_32_bit_f32_f6;
static double game_fn_offset_32_bit_f32_f7;
static double game_fn_offset_32_bit_f32_f8;
static double game_fn_offset_32_bit_f32_g;
union grug_value game_fn_offset_32_bit_f32(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_offset_32_bit_f32_call_count++;

	strcpy(game_fn_offset_32_bit_f32_s1, args[0]._string);
	strcpy(game_fn_offset_32_bit_f32_s2, args[1]._string);
	strcpy(game_fn_offset_32_bit_f32_s3, args[2]._string);
	strcpy(game_fn_offset_32_bit_f32_s4, args[3]._string);
	strcpy(game_fn_offset_32_bit_f32_s5, args[4]._string);
	strcpy(game_fn_offset_32_bit_f32_s6, args[5]._string);
	strcpy(game_fn_offset_32_bit_f32_s7, args[6]._string);
	strcpy(game_fn_offset_32_bit_f32_s8, args[7]._string);
	strcpy(game_fn_offset_32_bit_f32_s9, args[8]._string);
	strcpy(game_fn_offset_32_bit_f32_s10, args[9]._string);
	strcpy(game_fn_offset_32_bit_f32_s11, args[10]._string);
	strcpy(game_fn_offset_32_bit_f32_s12, args[11]._string);
	strcpy(game_fn_offset_32_bit_f32_s13, args[12]._string);
	strcpy(game_fn_offset_32_bit_f32_s14, args[13]._string);
	strcpy(game_fn_offset_32_bit_f32_s15, args[14]._string);
	game_fn_offset_32_bit_f32_f1 = args[15]._number;
	game_fn_offset_32_bit_f32_f2 = args[16]._number;
	game_fn_offset_32_bit_f32_f3 = args[17]._number;
	game_fn_offset_32_bit_f32_f4 = args[18]._number;
	game_fn_offset_32_bit_f32_f5 = args[19]._number;
	game_fn_offset_32_bit_f32_f6 = args[20]._number;
	game_fn_offset_32_bit_f32_f7 = args[21]._number;
	game_fn_offset_32_bit_f32_f8 = args[22]._number;
	game_fn_offset_32_bit_f32_g = args[23]._number;
	return (union grug_value) {0};
}
static double game_fn_offset_32_bit_i32_f1;
static double game_fn_offset_32_bit_i32_f2;
static double game_fn_offset_32_bit_i32_f3;
static double game_fn_offset_32_bit_i32_f4;
static double game_fn_offset_32_bit_i32_f5;
static double game_fn_offset_32_bit_i32_f6;
static double game_fn_offset_32_bit_i32_f7;
static double game_fn_offset_32_bit_i32_f8;
static double game_fn_offset_32_bit_i32_f9;
static double game_fn_offset_32_bit_i32_f10;
static double game_fn_offset_32_bit_i32_f11;
static double game_fn_offset_32_bit_i32_f12;
static double game_fn_offset_32_bit_i32_f13;
static double game_fn_offset_32_bit_i32_f14;
static double game_fn_offset_32_bit_i32_f15;
static double game_fn_offset_32_bit_i32_f16;
static double game_fn_offset_32_bit_i32_f17;
static double game_fn_offset_32_bit_i32_f18;
static double game_fn_offset_32_bit_i32_f19;
static double game_fn_offset_32_bit_i32_f20;
static double game_fn_offset_32_bit_i32_f21;
static double game_fn_offset_32_bit_i32_f22;
static double game_fn_offset_32_bit_i32_f23;
static double game_fn_offset_32_bit_i32_f24;
static double game_fn_offset_32_bit_i32_f25;
static double game_fn_offset_32_bit_i32_f26;
static double game_fn_offset_32_bit_i32_f27;
static double game_fn_offset_32_bit_i32_f28;
static double game_fn_offset_32_bit_i32_f29;
static double game_fn_offset_32_bit_i32_f30;
static double game_fn_offset_32_bit_i32_i1;
static double game_fn_offset_32_bit_i32_i2;
static double game_fn_offset_32_bit_i32_i3;
static double game_fn_offset_32_bit_i32_i4;
static double game_fn_offset_32_bit_i32_i5;
static double game_fn_offset_32_bit_i32_g;
union grug_value game_fn_offset_32_bit_i32(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_offset_32_bit_i32_call_count++;

	game_fn_offset_32_bit_i32_f1 = args[0]._number;
	game_fn_offset_32_bit_i32_f2 = args[1]._number;
	game_fn_offset_32_bit_i32_f3 = args[2]._number;
	game_fn_offset_32_bit_i32_f4 = args[3]._number;
	game_fn_offset_32_bit_i32_f5 = args[4]._number;
	game_fn_offset_32_bit_i32_f6 = args[5]._number;
	game_fn_offset_32_bit_i32_f7 = args[6]._number;
	game_fn_offset_32_bit_i32_f8 = args[7]._number;
	game_fn_offset_32_bit_i32_f9 = args[8]._number;
	game_fn_offset_32_bit_i32_f10 = args[9]._number;
	game_fn_offset_32_bit_i32_f11 = args[10]._number;
	game_fn_offset_32_bit_i32_f12 = args[11]._number;
	game_fn_offset_32_bit_i32_f13 = args[12]._number;
	game_fn_offset_32_bit_i32_f14 = args[13]._number;
	game_fn_offset_32_bit_i32_f15 = args[14]._number;
	game_fn_offset_32_bit_i32_f16 = args[15]._number;
	game_fn_offset_32_bit_i32_f17 = args[16]._number;
	game_fn_offset_32_bit_i32_f18 = args[17]._number;
	game_fn_offset_32_bit_i32_f19 = args[18]._number;
	game_fn_offset_32_bit_i32_f20 = args[19]._number;
	game_fn_offset_32_bit_i32_f21 = args[20]._number;
	game_fn_offset_32_bit_i32_f22 = args[21]._number;
	game_fn_offset_32_bit_i32_f23 = args[22]._number;
	game_fn_offset_32_bit_i32_f24 = args[23]._number;
	game_fn_offset_32_bit_i32_f25 = args[24]._number;
	game_fn_offset_32_bit_i32_f26 = args[25]._number;
	game_fn_offset_32_bit_i32_f27 = args[26]._number;
	game_fn_offset_32_bit_i32_f28 = args[27]._number;
	game_fn_offset_32_bit_i32_f29 = args[28]._number;
	game_fn_offset_32_bit_i32_f30 = args[29]._number;
	game_fn_offset_32_bit_i32_i1 = args[30]._number;
	game_fn_offset_32_bit_i32_i2 = args[31]._number;
	game_fn_offset_32_bit_i32_i3 = args[32]._number;
	game_fn_offset_32_bit_i32_i4 = args[33]._number;
	game_fn_offset_32_bit_i32_i5 = args[34]._number;
	game_fn_offset_32_bit_i32_g = args[35]._number;
	return (union grug_value) {0};
}
static double game_fn_offset_32_bit_string_f1;
static double game_fn_offset_32_bit_string_f2;
static double game_fn_offset_32_bit_string_f3;
static double game_fn_offset_32_bit_string_f4;
static double game_fn_offset_32_bit_string_f5;
static double game_fn_offset_32_bit_string_f6;
static double game_fn_offset_32_bit_string_f7;
static double game_fn_offset_32_bit_string_f8;
static double game_fn_offset_32_bit_string_f9;
static double game_fn_offset_32_bit_string_f10;
static double game_fn_offset_32_bit_string_f11;
static double game_fn_offset_32_bit_string_f12;
static double game_fn_offset_32_bit_string_f13;
static double game_fn_offset_32_bit_string_f14;
static double game_fn_offset_32_bit_string_f15;
static double game_fn_offset_32_bit_string_f16;
static double game_fn_offset_32_bit_string_f17;
static double game_fn_offset_32_bit_string_f18;
static double game_fn_offset_32_bit_string_f19;
static double game_fn_offset_32_bit_string_f20;
static double game_fn_offset_32_bit_string_f21;
static double game_fn_offset_32_bit_string_f22;
static double game_fn_offset_32_bit_string_f23;
static double game_fn_offset_32_bit_string_f24;
static double game_fn_offset_32_bit_string_f25;
static double game_fn_offset_32_bit_string_f26;
static double game_fn_offset_32_bit_string_f27;
static double game_fn_offset_32_bit_string_f28;
static double game_fn_offset_32_bit_string_f29;
static double game_fn_offset_32_bit_string_f30;
static char game_fn_offset_32_bit_string_s1[256];
static char game_fn_offset_32_bit_string_s2[256];
static char game_fn_offset_32_bit_string_s3[256];
static char game_fn_offset_32_bit_string_s4[256];
static char game_fn_offset_32_bit_string_s5[256];
static double game_fn_offset_32_bit_string_g;
union grug_value game_fn_offset_32_bit_string(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_offset_32_bit_string_call_count++;

	game_fn_offset_32_bit_string_f1 = args[0]._number;
	game_fn_offset_32_bit_string_f2 = args[1]._number;
	game_fn_offset_32_bit_string_f3 = args[2]._number;
	game_fn_offset_32_bit_string_f4 = args[3]._number;
	game_fn_offset_32_bit_string_f5 = args[4]._number;
	game_fn_offset_32_bit_string_f6 = args[5]._number;
	game_fn_offset_32_bit_string_f7 = args[6]._number;
	game_fn_offset_32_bit_string_f8 = args[7]._number;
	game_fn_offset_32_bit_string_f9 = args[8]._number;
	game_fn_offset_32_bit_string_f10 = args[9]._number;
	game_fn_offset_32_bit_string_f11 = args[10]._number;
	game_fn_offset_32_bit_string_f12 = args[11]._number;
	game_fn_offset_32_bit_string_f13 = args[12]._number;
	game_fn_offset_32_bit_string_f14 = args[13]._number;
	game_fn_offset_32_bit_string_f15 = args[14]._number;
	game_fn_offset_32_bit_string_f16 = args[15]._number;
	game_fn_offset_32_bit_string_f17 = args[16]._number;
	game_fn_offset_32_bit_string_f18 = args[17]._number;
	game_fn_offset_32_bit_string_f19 = args[18]._number;
	game_fn_offset_32_bit_string_f20 = args[19]._number;
	game_fn_offset_32_bit_string_f21 = args[20]._number;
	game_fn_offset_32_bit_string_f22 = args[21]._number;
	game_fn_offset_32_bit_string_f23 = args[22]._number;
	game_fn_offset_32_bit_string_f24 = args[23]._number;
	game_fn_offset_32_bit_string_f25 = args[24]._number;
	game_fn_offset_32_bit_string_f26 = args[25]._number;
	game_fn_offset_32_bit_string_f27 = args[26]._number;
	game_fn_offset_32_bit_string_f28 = args[27]._number;
	game_fn_offset_32_bit_string_f29 = args[28]._number;
	game_fn_offset_32_bit_string_f30 = args[29]._number;
	strcpy(game_fn_offset_32_bit_string_s1, args[30]._string);
	strcpy(game_fn_offset_32_bit_string_s2, args[31]._string);
	strcpy(game_fn_offset_32_bit_string_s3, args[32]._string);
	strcpy(game_fn_offset_32_bit_string_s4, args[33]._string);
	strcpy(game_fn_offset_32_bit_string_s5, args[34]._string);
	game_fn_offset_32_bit_string_g = args[35]._number;
	return (union grug_value) {0};
}
static char game_fn_talk_message1[256];
static char game_fn_talk_message2[256];
static char game_fn_talk_message3[256];
static char game_fn_talk_message4[256];
union grug_value game_fn_talk(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_talk_call_count++;

	strcpy(game_fn_talk_message1, args[0]._string);
	strcpy(game_fn_talk_message2, args[1]._string);
	strcpy(game_fn_talk_message3, args[2]._string);
	strcpy(game_fn_talk_message4, args[3]._string);
	return (union grug_value) {0};
}
union grug_value game_fn_get_position(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_position_call_count++;

	(void)args;

	return grug_id(1337);
}
static uint64_t game_fn_set_position_pos;
union grug_value game_fn_set_position(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_position_call_count++;

	game_fn_set_position_pos = args[0]._id;
	return (union grug_value) {0};
}
union grug_value game_fn_cause_game_fn_error(struct grug_state* grug_state, const union grug_value args[]) {
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_cause_game_fn_error_call_count++;

	game_fn_error(grug_state, "cause_game_fn_error(): Example game function error");
	return grug_bool(true);
}
union grug_value game_fn_call_on_b_fn(struct grug_state* grug_state, const union grug_value args[]) {
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_call_on_b_fn_call_count++;

	call_export_fn_argless(grug_state, current_file_id, "on_b");
	return (union grug_value) {0};
}
static uint64_t game_fn_store_id;
union grug_value game_fn_store(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_store_call_count++;

	game_fn_store_id = args[0]._id;
	return (union grug_value) {0};
}
static char game_fn_print_csv_path[256];
union grug_value game_fn_print_csv(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_print_csv_call_count++;
	strcpy(game_fn_print_csv_path, args[0]._string);
	return (union grug_value) {0};
}
union grug_value game_fn_retrieve(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	(void)args;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_retrieve_call_count++;

	return grug_id(123);
}
union grug_value game_fn_box_number(struct grug_state* grug_state, const union grug_value args[]) {
	(void)grug_state;
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_box_number_call_count++;

	return grug_id((uint64_t)args[0]._number);
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

static bool is_whitelisted_test(const char *name) {
	return whitelisted_test == NULL || streq(name, whitelisted_test);
}

static void print_string_debug(const char* str) {
	fprintf(stderr, "\"");
	while (*str != '\0') {
		switch (*str) {
			case '\n': fputs("\\n", stderr); break;
			case '\r': fputs("\\r", stderr); break;
			case '\t': fputs("\\t", stderr); break;
			case '\v': fputs("\\v", stderr); break;
			case '\f': fputs("\\f", stderr); break;
			case '\b': fputs("\\b", stderr); break;
			case '\\': fputs("\\\\", stderr); break;
			case '"':  fputs("\\\"", stderr); break;

			default:
				if (isprint(*str)) {
					fputc(*str, stderr);
				} else {
					fprintf(stderr, "\\x%02X", *str);
				}
				break;
		}
		str++;
	}
	fprintf(stderr, "\"\n");
}

#define ADD_TEST_ERROR(test_name, entity_type) do {\
	if (is_whitelisted_test(#test_name)) {\
		error_test_datas[err_test_datas_size++] = (struct error_test_data){\
			.test_name_str = #test_name,\
			.grug_path = "err"SLASH#test_name SLASH"input-"entity_type".grug",\
			.expected_error_path = "err"SLASH#test_name SLASH"expected_error.txt",\
			.results_path = "err"SLASH#test_name SLASH"results",\
			.grug_output_path = "err"SLASH#test_name SLASH"results"SLASH"grug_output.txt"\
		};\
	}\
} while (0)

#define ADD_TEST_ERROR_FILE_NAME(test_name, file_name) do {\
	if (is_whitelisted_test(#test_name)) {\
		error_test_datas[err_test_datas_size++] = (struct error_test_data){\
			.test_name_str = #test_name,\
			.grug_path = "err"SLASH#test_name SLASH file_name,\
			.expected_error_path = "err"SLASH#test_name SLASH"expected_error.txt",\
			.results_path = "err"SLASH#test_name SLASH"results",\
			.grug_output_path = "err"SLASH#test_name SLASH"results"SLASH"grug_output.txt"\
		};\
	}\
} while (0)

#define ADD_TEST_OK(test_name, entity_type) do {\
	if (is_whitelisted_test(#test_name)) {\
		ok_test_datas[ok_test_datas_size++] = (struct ok_test_data){\
			.run = ok_##test_name,\
			.file_id = NULL,\
			.test_name_str = #test_name,\
			.grug_path = "ok"SLASH#test_name SLASH"input-"entity_type".grug",\
			.results_path = "ok"SLASH#test_name SLASH"results",\
			.dump_path = "ok"SLASH#test_name SLASH"results"SLASH"dump.json",\
			.applied_path = "ok"SLASH#test_name SLASH"results"SLASH"applied.grug"\
		};\
	}\
} while (0)

#define ADD_TEST_RUNTIME_ERROR(test_name, entity_type) do {\
	if (is_whitelisted_test(#test_name)) {\
		runtime_error_test_datas[err_runtime_test_datas_size++] = (struct runtime_error_test_data){\
			.run = runtime_error_##test_name,\
			.file_id = NULL,\
			.test_name_str = #test_name,\
			.grug_path = "err_runtime"SLASH#test_name SLASH"input-"entity_type".grug",\
			.expected_error_path = "err_runtime"SLASH#test_name SLASH"expected_error.txt",\
			.results_path = "err_runtime"SLASH#test_name SLASH"results",\
			.dump_path = "err_runtime"SLASH#test_name SLASH"results"SLASH"dump.json",\
			.applied_path = "err_runtime"SLASH#test_name SLASH"results"SLASH"applied.grug"\
		};\
	}\
} while (0)

// This is the Fisher-Yates shuffle:
// https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
// https://blog.codinghorror.com/the-danger-of-naivete/
#ifdef SHUFFLES
#define SHUFFLE(arr, size, T) do {\
	for (size_t i = size; i > 0; i--) {\
		size_t n = (size_t)rand() % i;\
		\
		T old = arr[i-1];\
		arr[i-1] = arr[n];\
		arr[n] = old;\
	}\
} while (0)
#endif

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

	if (bytes[len - 1] == '\n') {
		len--;
		if (bytes[len - 1] == '\r') {
			len--;
		}
	}

	bytes[len] = '\0';

	return len;
}

static const char *get_expected_error(const char *expected_error_path) {
	static char expected_error[420420];
	read_file(expected_error_path, (uint8_t *)expected_error);

	return expected_error;
}

static void make_dir_if_not_exists(const char* results_path) {
	#if defined(__linux__)
	if (mkdir(prefix(results_path), 0755) == -1 && errno != EEXIST) {
	#elif defined(WIN32)
	if (CreateDirectory(prefix(results_path), NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
	#endif
		perror("mkdir");
		fprintf(stderr, "prefix(results_path): \"%s\"\n", prefix(results_path));\
		exit(EXIT_FAILURE);
	}
}

static void run_err_spaces_test(struct grug_state *grug_state, const char *name) {
	if (!is_whitelisted_test(name)) {
		return;
	}

	printf("Running tests/err_spaces/%s-D.grug...\n", name);

    char grug_path[4096];
    int grug_len = snprintf(grug_path, sizeof(grug_path), "%s"SLASH"%s-D.grug", tests_dir_path, name);
    if (grug_len < 0 || (size_t)grug_len >= sizeof(grug_path)) {
		fprintf(stderr, "Error: Filling grug_path failed\n");
		exit(EXIT_FAILURE);
	}

    // This version does not have the tests/ prefix
    char relative_path[4096];
    int rel_len = snprintf(relative_path, sizeof(relative_path), "err_spaces"SLASH"%s-D.grug", name);
    if (rel_len < 0 || (size_t)rel_len >= sizeof(relative_path)) {
		fprintf(stderr, "Error: Filling relative_path failed\n");
		exit(EXIT_FAILURE);
	}

    const char *msg = NULL;
    compile_grug_file(grug_state, relative_path, &msg);

    if (msg == NULL) {
        fprintf(stderr, "\nError: Expected compilation failure for %s-D.grug, but it succeeded\n", name);
        exit(EXIT_FAILURE);
    }
}

static void run_err_spaces_tests(struct grug_state *grug_state) {
	run_err_spaces_test(grug_state, "add_expr_00");
	run_err_spaces_test(grug_state, "add_expr_01");
	run_err_spaces_test(grug_state, "add_expr_02");
	run_err_spaces_test(grug_state, "add_expr_03");
	run_err_spaces_test(grug_state, "and_expr_00");
	run_err_spaces_test(grug_state, "and_expr_01");
	run_err_spaces_test(grug_state, "atom_00");
	run_err_spaces_test(grug_state, "atom_01");
	run_err_spaces_test(grug_state, "atom_02");
	run_err_spaces_test(grug_state, "atom_03");
	run_err_spaces_test(grug_state, "block_00");
	run_err_spaces_test(grug_state, "block_01");
	run_err_spaces_test(grug_state, "block_02");
	run_err_spaces_test(grug_state, "block_03");
	run_err_spaces_test(grug_state, "block_04");
	run_err_spaces_test(grug_state, "block_05");
	run_err_spaces_test(grug_state, "break_00");
	run_err_spaces_test(grug_state, "break_01");
	run_err_spaces_test(grug_state, "call_expr_00");
	run_err_spaces_test(grug_state, "call_expr_01");
	run_err_spaces_test(grug_state, "call_expr_02");
	run_err_spaces_test(grug_state, "call_expr_03");
	run_err_spaces_test(grug_state, "call_expr_04");
	run_err_spaces_test(grug_state, "call_expr_05");
	run_err_spaces_test(grug_state, "call_stmt_00");
	run_err_spaces_test(grug_state, "call_stmt_01");
	run_err_spaces_test(grug_state, "call_stmt_02");
	run_err_spaces_test(grug_state, "call_stmt_03");
	run_err_spaces_test(grug_state, "call_stmt_04");
	run_err_spaces_test(grug_state, "call_stmt_05");
	run_err_spaces_test(grug_state, "call_stmt_06");
	run_err_spaces_test(grug_state, "call_stmt_07");
	run_err_spaces_test(grug_state, "compare_expr_00");
	run_err_spaces_test(grug_state, "compare_expr_01");
	run_err_spaces_test(grug_state, "compare_expr_02");
	run_err_spaces_test(grug_state, "compare_expr_03");
	run_err_spaces_test(grug_state, "compare_expr_04");
	run_err_spaces_test(grug_state, "compare_expr_05");
	run_err_spaces_test(grug_state, "compare_expr_06");
	run_err_spaces_test(grug_state, "compare_expr_07");
	run_err_spaces_test(grug_state, "continue_00");
	run_err_spaces_test(grug_state, "continue_01");
	run_err_spaces_test(grug_state, "entity_00");
	run_err_spaces_test(grug_state, "entity_01");
	run_err_spaces_test(grug_state, "entity_02");
	run_err_spaces_test(grug_state, "equality_expr_00");
	run_err_spaces_test(grug_state, "equality_expr_01");
	run_err_spaces_test(grug_state, "equality_expr_02");
	run_err_spaces_test(grug_state, "equality_expr_03");
	run_err_spaces_test(grug_state, "helper_fn_00");
	run_err_spaces_test(grug_state, "helper_fn_01");
	run_err_spaces_test(grug_state, "helper_fn_02");
	run_err_spaces_test(grug_state, "helper_fn_03");
	run_err_spaces_test(grug_state, "if_00");
	run_err_spaces_test(grug_state, "if_01");
	run_err_spaces_test(grug_state, "if_02");
	run_err_spaces_test(grug_state, "literal_00");
	run_err_spaces_test(grug_state, "literal_01");
	run_err_spaces_test(grug_state, "literal_02");
	run_err_spaces_test(grug_state, "literal_03");
	run_err_spaces_test(grug_state, "literal_04");
	run_err_spaces_test(grug_state, "literal_05");
	run_err_spaces_test(grug_state, "mul_expr_00");
	run_err_spaces_test(grug_state, "mul_expr_01");
	run_err_spaces_test(grug_state, "mul_expr_02");
	run_err_spaces_test(grug_state, "mul_expr_03");
	run_err_spaces_test(grug_state, "not_00");
	run_err_spaces_test(grug_state, "not_01");
	run_err_spaces_test(grug_state, "on_fn_00");
	run_err_spaces_test(grug_state, "on_fn_01");
	run_err_spaces_test(grug_state, "on_fn_02");
	run_err_spaces_test(grug_state, "on_fn_03");
	run_err_spaces_test(grug_state, "on_fn_04");
	run_err_spaces_test(grug_state, "or_expr_00");
	run_err_spaces_test(grug_state, "or_expr_01");
	run_err_spaces_test(grug_state, "params_00");
	run_err_spaces_test(grug_state, "params_01");
	run_err_spaces_test(grug_state, "params_02");
	run_err_spaces_test(grug_state, "params_03");
	run_err_spaces_test(grug_state, "params_04");
	run_err_spaces_test(grug_state, "params_05");
	run_err_spaces_test(grug_state, "params_06-F.grug");
	run_err_spaces_test(grug_state, "params_07-F.grug");
	run_err_spaces_test(grug_state, "params_08-F.grug");
	run_err_spaces_test(grug_state, "params_09-F.grug");
	run_err_spaces_test(grug_state, "params_10-G.grug");
	run_err_spaces_test(grug_state, "params_11-G.grug");
	run_err_spaces_test(grug_state, "reassign_00");
	run_err_spaces_test(grug_state, "reassign_01");
	run_err_spaces_test(grug_state, "reassign_02");
	run_err_spaces_test(grug_state, "reassign_03");
	run_err_spaces_test(grug_state, "resource_00");
	run_err_spaces_test(grug_state, "resource_01");
	run_err_spaces_test(grug_state, "resource_02");
	run_err_spaces_test(grug_state, "return_00");
	run_err_spaces_test(grug_state, "return_01");
	run_err_spaces_test(grug_state, "return_02");
	run_err_spaces_test(grug_state, "unary_expr_00");
	run_err_spaces_test(grug_state, "unary_expr_01");
	run_err_spaces_test(grug_state, "unary_expr_02");
	run_err_spaces_test(grug_state, "unary_expr_03");
	run_err_spaces_test(grug_state, "unary_expr_04");
	run_err_spaces_test(grug_state, "unary_expr_05");
	run_err_spaces_test(grug_state, "unary_expr_06");
	run_err_spaces_test(grug_state, "unary_expr_07");
	run_err_spaces_test(grug_state, "vardecl_00");
	run_err_spaces_test(grug_state, "vardecl_01");
	run_err_spaces_test(grug_state, "vardecl_02");
	run_err_spaces_test(grug_state, "vardecl_03");
	run_err_spaces_test(grug_state, "vardecl_04");
	run_err_spaces_test(grug_state, "vardecl_05");
	run_err_spaces_test(grug_state, "while_00");
	run_err_spaces_test(grug_state, "while_01");
	run_err_spaces_test(grug_state, "while_02");
}

static void run_err_mod_api_test(const char *name) {
	if (!is_whitelisted_test(name)) {
		return;
	}

    printf("Running tests/err_mod_api/%s...\n", name);

    char path[4096];
    int len = snprintf(path, sizeof(path), "%s"SLASH"err_mod_api"SLASH"%s", tests_dir_path, name);
    if (len < 0 || (size_t)len >= sizeof(path)) {
		fprintf(stderr, "Error: Filling err_mod_api path failed\n");
		exit(EXIT_FAILURE);
	}

	void* grug_state = create_grug_state(path, tests_dir_path);
	if (grug_state) {
		fprintf(stderr, "Error: Expected create_grug_state(\"%s\", \"%s\") to return NULL\n", path, tests_dir_path);
		exit(EXIT_FAILURE);
	}
}

static void run_err_mod_api_tests(void) {
	run_err_mod_api_test("entities_must_be_json_object.json");
	run_err_mod_api_test("entities_must_be_sorted.json");
	run_err_mod_api_test("entity_must_be_json_object.json");
	run_err_mod_api_test("game_fns_must_be_json_object.json");
	run_err_mod_api_test("game_fns_must_be_sorted.json");
	run_err_mod_api_test("on_fns_must_be_json_object.json");
	run_err_mod_api_test("on_fns_must_be_sorted.json");
	run_err_mod_api_test("root_must_be_object.json");
}

static void test_error(
	void* grug_state,
	const char *test_name,
	const char *grug_path,
	const char *expected_error_path,
	const char *results_path,
	const char *grug_output_path
) {
	printf("Running tests/err/%s...\n", test_name);

	make_dir_if_not_exists(results_path);

	const char* msg = NULL;
	compile_grug_file(grug_state, grug_path, &msg);

	const char *expected_error = get_expected_error(expected_error_path);

	if (!msg) {
		fprintf(stderr, "\nError: Compilation succeeded, but expected this error message:\n");
		print_string_debug(expected_error);
		exit(EXIT_FAILURE);
	}

	FILE *f = fopen(prefix(grug_output_path), "w");
	check_null(f, "fopen", grug_output_path);

	size_t msg_len = strlen(msg);

	size_t written = fwrite(msg, 1, msg_len, f);
	if (written != msg_len) {
		if (ferror(f)) {
			perror("fwrite");
		} else {
			fprintf(stderr, "fwrite: short write\n");
		}
		exit(EXIT_FAILURE);
	}

	if (fclose(f) == EOF) {
		perror("fclose");
		exit(EXIT_FAILURE);
	}

	if (!streq(msg, expected_error)) {
		fprintf(stderr, "\nError: The output differs from the expected output.\n");
		fprintf(stderr, "Output:\n");
		print_string_debug(msg);

		fprintf(stderr, "Expected:\n");
		print_string_debug(expected_error);

		exit(EXIT_FAILURE);
	}
}

static void diff_roundtrip(
	void* grug_state,
	const char *grug_path,
	const char *dump_path,
	const char *applied_path
) {
	static char buf[4096];
	if (dump_file_to_json(grug_state, prefix(grug_path), prefix_buf(dump_path, buf))) {
		fprintf(stderr, "Error: Failed to dump file AST\n");
		exit(EXIT_FAILURE);
	}

	if (generate_file_from_json(grug_state, prefix(dump_path), prefix_buf(applied_path, buf))) {
		fprintf(stderr, "Error: Failed to apply file AST\n");
		exit(EXIT_FAILURE);
	}

	static uint8_t grug_path_bytes[420420];
	size_t grug_path_bytes_len = read_file(grug_path, grug_path_bytes);
	grug_path_bytes[grug_path_bytes_len] = '\0';

	static uint8_t applied_path_bytes[420420];
	size_t applied_path_bytes_len = read_file(applied_path, applied_path_bytes);
	applied_path_bytes[applied_path_bytes_len] = '\0';

	if (!streq((const char *)grug_path_bytes, (const char *)applied_path_bytes)) {
		fprintf(stderr, "\nError: The roundtrip output differs from the expected output.\n");
		fprintf(stderr, "Output:\n");
		print_string_debug((const char *)applied_path_bytes);

		fprintf(stderr, "Expected:\n");
		print_string_debug((const char *)grug_path_bytes);

		exit(EXIT_FAILURE);
	}
}

static void reset(void) {
	had_runtime_error = false;
	error_handler_call_count = 0;
	runtime_error_type = 0;
	game_fn_nothing_call_count = 0;
	game_fn_magic_call_count = 0;
	game_fn_initialize_call_count = 0;
	game_fn_initialize_bool_call_count = 0;
	game_fn_identity_call_count = 0;
	game_fn_max_call_count = 0;
	game_fn_say_call_count = 0;
	game_fn_sin_call_count = 0;
	game_fn_cos_call_count = 0;
	game_fn_mega_call_count = 0;
	game_fn_get_false_call_count = 0;
	game_fn_set_is_happy_call_count = 0;
	game_fn_mega_f32_call_count = 0;
	game_fn_mega_i32_call_count = 0;
	game_fn_draw_call_count = 0;
	game_fn_blocked_alrm_call_count = 0;
	game_fn_spawn_call_count = 0;
	game_fn_spawn_d_call_count = 0;
	game_fn_has_resource_call_count = 0;
	game_fn_has_entity_call_count = 0;
	game_fn_has_string_call_count = 0;
	game_fn_get_opponent_call_count = 0;
	game_fn_get_os_call_count = 0;
	game_fn_set_d_call_count = 0;
	game_fn_set_opponent_call_count = 0;
	game_fn_motherload_call_count = 0;
	game_fn_motherload_subless_call_count = 0;
	game_fn_offset_32_bit_f32_call_count = 0;
	game_fn_offset_32_bit_i32_call_count = 0;
	game_fn_offset_32_bit_string_call_count = 0;
	game_fn_talk_call_count = 0;
	game_fn_get_position_call_count = 0;
	game_fn_set_position_call_count = 0;
	game_fn_cause_game_fn_error_call_count = 0;
	game_fn_call_on_b_fn_call_count = 0;
	game_fn_store_call_count = 0;
	game_fn_print_csv_call_count = 0;
	game_fn_retrieve_call_count = 0;
	game_fn_box_number_call_count = 0;
}

static void* prologue(void* grug_state, const char *grug_path, const char *results_path) {
	make_dir_if_not_exists(results_path);

	const char *msg = NULL;
	void *file_id = compile_grug_file(grug_state, grug_path, &msg);
	if (msg) {
		fprintf(stderr, "Error: The test wasn't supposed to print anything, but did:\n");
		fprintf(stderr, "----\n");
		print_string_debug(msg);
		fprintf(stderr, "----\n");

		exit(EXIT_FAILURE);
	}
	return file_id;
}

static void ok_addition_as_argument(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_false(had_runtime_error);

	assert_number(game_fn_initialize_x, 3.0);
}

static void ok_addition_as_two_arguments(void* grug_state, void* file_id) {
	assert_call_count(max, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(max, 1);

	assert_number(game_fn_max_x, 3.0);
	assert_number(game_fn_max_y, 9.0);
}

static void ok_addition_with_multiplication(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 14.0);
}

static void ok_addition_with_multiplication_2(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 10.0);
}

static void ok_and_false_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_and_false_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_and_false_3(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_and_short_circuit(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_and_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_blocked_alrm(void* grug_state, void* file_id) {
	assert_call_count(blocked_alrm, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(blocked_alrm, 1);
}

static void ok_bool_logical_not_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_bool_logical_not_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_bool_returned(void* grug_state, void* file_id) {
	assert_call_count(set_is_happy, 0);
	assert_call_count(get_false, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(set_is_happy, 1);
	assert_call_count(get_false, 1);

	assert_false(game_fn_set_is_happy_is_happy);
}

static void ok_bool_returned_global(void* grug_state, void* file_id) {
	assert_call_count(set_is_happy, 0);
	assert_call_count(get_false, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(set_is_happy, 1);
	assert_call_count(get_false, 1);

	assert_false(game_fn_set_is_happy_is_happy);
}

static void ok_bool_zero_extended_if_statement(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
	assert_call_count(get_false, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
	assert_call_count(get_false, 1);
}

static void ok_bool_zero_extended_while_statement(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_break(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 3);
}

static void ok_calls_100(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 100);
}

static void ok_calls_1000(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1000);
}

static void ok_calls_in_call(void* grug_state, void* file_id) {
	assert_call_count(max, 0);
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(max, 3);
	assert_call_count(initialize, 1);

	assert_number(game_fn_max_x, 2.0);
	assert_number(game_fn_max_y, 4.0);
	assert_number(game_fn_initialize_x, 4.0);
}

static void ok_comment_above_block(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_comment_above_block_twice(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_comment_above_globals(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_comment_above_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_comment_above_on_fn(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_comment_between_globals(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_comment_between_statements(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_comment_lone_block(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_comment_lone_block_at_end(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_comment_lone_global(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_continue(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_custom_id_decays_to_id(void* grug_state, void* file_id) {
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 42);
}

static void ok_custom_id_transfer_between_globals(void* grug_state, void* file_id) {
	assert_call_count(get_opponent, 1);
	assert_call_count(set_opponent, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_opponent, 1); 
	assert_call_count(set_opponent, 1);

	assert_id(game_fn_set_opponent_target, 69);
}

static void ok_custom_id_with_digits(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
	assert_call_count(box_number, 1);
}

static void ok_division_negative_result(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -2.5);
}

static void ok_division_positive_result(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 2.5);
}

static void ok_double_negation_with_parentheses(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 2.0);
}

static void ok_double_not_with_parentheses(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_else_after_else_if_false(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_else_after_else_if_true(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 3);
}

static void ok_else_false(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_else_if_false(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_else_if_true(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 3);
}

static void ok_else_true(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 3);
}

static void ok_empty_file(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_empty_line(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_entity_and_resource_as_subexpression(void* grug_state, void* file_id) {
	assert_call_count(has_resource, 0);
	assert_call_count(has_entity, 0);
	assert_call_count(has_string, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(has_resource, 1);
	assert_call_count(has_entity, 1);
	assert_call_count(has_string, 1);

	assert_string(game_fn_has_resource_path, "ok"SLASH"entity_and_resource_as_subexpression/foo.txt");
	assert_string(game_fn_has_entity_name, "ok:baz");
	assert_string(game_fn_has_string_str, "bar");
}

static void ok_entity_duplicate(void* grug_state, void* file_id) {
	assert_call_count(spawn, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(spawn, 4);

	assert_string(game_fn_spawn_name, "ok:baz");
}

static void ok_entity_in_on_fn(void* grug_state, void* file_id) {
	assert_call_count(spawn, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(spawn, 1);

	assert_string(game_fn_spawn_name, "ok:foo");
}

static void ok_entity_in_on_fn_with_mod_specified(void* grug_state, void* file_id) {
	assert_call_count(spawn, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(spawn, 1);

	assert_string(game_fn_spawn_name, "wow:foo");
}

static void ok_eq_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_eq_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_addition(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 6.0);
}

static void ok_f32_argument(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 4.0);
}

static void ok_f32_division(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 0.5);
}

static void ok_f32_eq_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_f32_eq_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_ge_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_f32_ge_true_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_ge_true_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_global_variable(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 4.0);
}

static void ok_f32_gt_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_f32_gt_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_le_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_f32_le_true_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_le_true_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_local_variable(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 4.0);
}

static void ok_f32_lt_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_f32_lt_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_multiplication(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 8.0);
}

static void ok_f32_ne_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_f32_negated(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, -4.0);
}

static void ok_f32_ne_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_f32_passed_to_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 42.0);
}

static void ok_f32_passed_to_on_fn(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn(grug_state, file_id, "on_a", (const union grug_value[]){{._number=42.0}}, 1);
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, 42.0);
}

static void ok_f32_passing_sin_to_cos(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
	assert_call_count(cos, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);
	assert_call_count(cos, 1);

	assert_number(game_fn_sin_x, 4.0);
	assert_number(game_fn_cos_x, sin(4.0));
}

static void ok_f32_subtraction(void* grug_state, void* file_id) {
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(sin, 1);

	assert_number(game_fn_sin_x, -2.0);
}

static void ok_fibonacci(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 55.0);
}

static void ok_ge_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_ge_true_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_ge_true_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_global_2_does_not_have_error_handling(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_global_call_using_me(void* grug_state, void* file_id) {
	assert_call_count(get_position, 1); // Called by init_globals()
	assert_call_count(set_position, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_position, 1);
	assert_call_count(set_position, 1);

	assert_id(game_fn_set_position_pos, 1337);
}

static void ok_global_can_use_earlier_global(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 5.0);
}

static void ok_global_containing_negation(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -2.0);
}

static void ok_global_id(void* grug_state, void* file_id) {
	assert_call_count(get_opponent, 1); // Called by init_globals()
	assert_call_count(set_opponent, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_opponent, 1);
	assert_call_count(set_opponent, 1);

	assert_id(game_fn_set_opponent_target, 69);
}

static void ok_global_parentheses(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 14.0);
}

static void ok_globals(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 2);

	assert_number(game_fn_initialize_x, 1337.0);
}

static void ok_globals_1000(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_globals_1000_string(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_globals_32(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_globals_64(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_gt_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_gt_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_helper_fn_called_in_if(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_helper_fn_called_indirectly(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_helper_fn_overwriting_param(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
	assert_call_count(sin, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);
	assert_call_count(sin, 1);

	assert_number(game_fn_initialize_x, 20.0);
	assert_number(game_fn_sin_x, 30.0);
}

static void ok_helper_fn_returning_void_has_no_return(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_helper_fn_returning_void_returns_void(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_helper_fn_same_param_name_as_on_fn(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn(grug_state, file_id, "on_a", (const union grug_value[]){{._number=42.0}}, 1);
	assert_call_count(nothing, 1);
}

static void ok_helper_fn_same_param_name_as_other_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn(grug_state, file_id, "on_a", (const union grug_value[]){{._number=42.0}}, 1);
	assert_call_count(nothing, 2);
}

static void ok_i32_max(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 2147483647.0);
}

static void ok_i32_min(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -2147483648.0);
}

static void ok_i32_negated(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -42.0);
}

static void ok_i32_negative_is_smaller_than_positive(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_id_binary_expr_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_id_binary_expr_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_id_eq_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
	assert_call_count(retrieve, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);
	assert_call_count(retrieve, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_id_eq_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
	assert_call_count(retrieve, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);
	assert_call_count(retrieve, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_id_global_with_id_to_new_id(void* grug_state, void* file_id) {
	assert_call_count(retrieve, 1); // Called by init_globals()
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(retrieve, 1); // Called by init_globals()
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 123);
}

static void ok_id_global_with_opponent_to_new_id(void* grug_state, void* file_id) {
	assert_call_count(get_opponent, 1); // Called by init_globals()
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_opponent, 1); // Called by init_globals()
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 69);
}

static void ok_id_helper_fn_param(void* grug_state, void* file_id) {
	assert_call_count(retrieve, 0);
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(retrieve, 1);
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 123);
}

static void ok_id_local_variable_get_and_set(void* grug_state, void* file_id) {
	assert_call_count(get_opponent, 0);
	assert_call_count(set_opponent, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_opponent, 1);
	assert_call_count(set_opponent, 1);

	assert_id(game_fn_set_opponent_target, 69);
}

static void ok_id_ne_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
	assert_call_count(retrieve, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);
	assert_call_count(retrieve, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_id_ne_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
	assert_call_count(retrieve, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);
	assert_call_count(retrieve, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_id_on_fn_param(void* grug_state, void* file_id) {
	assert_call_count(store, 0);
    call_export_fn(grug_state, file_id, "on_a", (const union grug_value[]){{._id=77}}, 1);
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 77);
}

static void ok_id_returned_from_helper(void* grug_state, void* file_id) {
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 42);
}

static void ok_id_with_d_to_new_id_and_id_to_old_id(void* grug_state, void* file_id) {
	assert_call_count(retrieve, 0);
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(retrieve, 1);
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 123);
}

static void ok_id_with_d_to_old_id(void* grug_state, void* file_id) {
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 42);
}

static void ok_id_with_id_to_new_id(void* grug_state, void* file_id) {
	assert_call_count(retrieve, 0);
	assert_call_count(store, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(retrieve, 1);
	assert_call_count(store, 1);

	assert_id(game_fn_store_id, 123);
}

static void ok_if_false(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_if_true(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 3);
}

static void ok_le_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_le_true_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_le_true_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_local_id_can_be_reassigned(void* grug_state, void* file_id) {
	assert_call_count(get_opponent, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_opponent, 2);
}

static void ok_lt_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_lt_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_max_args(void* grug_state, void* file_id) {
	assert_call_count(mega, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(mega, 1);

	assert_number(game_fn_mega_f1, 1.0);
	assert_number(game_fn_mega_i1, 21.0);
	assert_true(game_fn_mega_b1);
	assert_number(game_fn_mega_f2, 2.0);
	assert_number(game_fn_mega_f3, 3.0);
	assert_number(game_fn_mega_f4, 4.0);
	assert_false(game_fn_mega_b2);
	assert_number(game_fn_mega_i2, 1337.0);
	assert_number(game_fn_mega_f5, 5.0);
	assert_number(game_fn_mega_f6, 6.0);
	assert_number(game_fn_mega_f7, 7.0);
	assert_number(game_fn_mega_f8, 8.0);
	assert_id(game_fn_mega_id, 42);
	assert_string(game_fn_mega_str, "foo");
}

static void ok_me(void* grug_state, void* file_id) {
	assert_call_count(set_d, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(set_d, 1);

	assert_id(game_fn_set_d_target, 42);
}

static void ok_me_assigned_to_local_variable(void* grug_state, void* file_id) {
	assert_call_count(set_d, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(set_d, 1);

	assert_id(game_fn_set_d_target, 42);
}

static void ok_me_passed_to_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(set_d, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(set_d, 1);

	assert_id(game_fn_set_d_target, 42);
}

static void ok_mov_32_bits_global_i32(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_mov_32_bits_global_id(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_multiplication_as_two_arguments(void* grug_state, void* file_id) {
	assert_call_count(max, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(max, 1);

	assert_number(game_fn_max_x, 6.0);
	assert_number(game_fn_max_y, 20.0);
}

static void ok_ne_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_ne_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_negate_parenthesized_expr(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -5.0);
}

static void ok_negative_literal(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -42.0);
}

static void ok_nested_break(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 3);
}

static void ok_nested_continue(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_no_empty_line_between_globals(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_no_empty_line_between_statements(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_on_fn(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_on_fn_calling_game_fn_nothing(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_on_fn_calling_game_fn_nothing_twice(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_on_fn_calling_game_fn_plt_order(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
	assert_call_count(magic, 0);
	assert_call_count(initialize, 0);
	assert_call_count(identity, 0);
	assert_call_count(max, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
	assert_call_count(magic, 1);
	assert_call_count(initialize, 1);
	assert_call_count(identity, 1);
	assert_call_count(max, 1);

	assert_number(game_fn_initialize_x, 42.0);
	assert_number(game_fn_identity_x, 69.0);
	assert_number(game_fn_max_x, 1337.0);
	assert_number(game_fn_max_y, 8192.0);
}

static void ok_on_fn_calling_helper_fns(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 42.0);
}

static void ok_on_fn_calling_no_game_fn(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_on_fn_calling_no_game_fn_but_with_addition(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_on_fn_calling_no_game_fn_but_with_global(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_on_fn_overwriting_param(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
	assert_call_count(sin, 0);
    call_export_fn(grug_state, file_id, "on_a", (const union grug_value[]){{._number=2.0}, {._number=3.0}}, 2);
	assert_call_count(initialize, 1);
	assert_call_count(sin, 1);

	assert_number(game_fn_initialize_x, 20.0);
	assert_number(game_fn_sin_x, 30.0);
}

static void ok_on_fn_passing_argument_to_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 42.0);
}

static void ok_on_fn_passing_magic_to_initialize(void* grug_state, void* file_id) {
	assert_call_count(magic, 0);
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(magic, 1);
	assert_call_count(initialize, 1);
}

static void ok_on_fn_three(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
    call_export_fn_argless(grug_state, file_id, "on_b");
    call_export_fn_argless(grug_state, file_id, "on_c");
	assert_call_count(nothing, 3);
}

static void ok_on_fn_three_unused_first(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_b");
    call_export_fn_argless(grug_state, file_id, "on_c");
	assert_call_count(nothing, 2);
}

static void ok_on_fn_three_unused_second(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
    call_export_fn_argless(grug_state, file_id, "on_c");
	assert_call_count(nothing, 2);
}

static void ok_on_fn_three_unused_third(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
    call_export_fn_argless(grug_state, file_id, "on_b");
	assert_call_count(nothing, 2);
}

static void ok_or_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_or_short_circuit(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_or_true_1(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_or_true_2(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_or_true_3(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_pass_string_argument_to_game_fn(void* grug_state, void* file_id) {
	assert_call_count(say, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(say, 1);

	assert_string(game_fn_say_message, "foo");
}

static void ok_pass_string_argument_to_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(say, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(say, 1);

	assert_string(game_fn_say_message, "foo");
}

static void ok_print_csv(void* grug_state, void* file_id) {
	assert_call_count(print_csv, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(print_csv, 1);

	assert_string(game_fn_print_csv_path, "ok"SLASH"print_csv/foo.csv");
}

static void ok_resource_and_entity(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
	assert_call_count(spawn, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);
	assert_call_count(spawn, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_and_entity/foo.txt");
	assert_string(game_fn_spawn_name, "ok:foo");
}

static void ok_resource_can_contain_dot_1(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_can_contain_dot_1/.foo");
}

static void ok_resource_can_contain_dot_2(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_can_contain_dot_2/foo.bar");
}

static void ok_resource_can_contain_dot_dot_1(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_can_contain_dot_dot_1/..foo");
}

static void ok_resource_can_contain_dot_dot_2(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_can_contain_dot_dot_2/foo..bar");
}

static void ok_resource_can_contain_dot_dot_dot(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"...foo");
}

static void ok_resource_duplicate(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 4);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_duplicate/baz.txt");
}

static void ok_resource_is_a_directory(void* grug_state, void* file_id) {
	assert_call_count(draw, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(draw, 1);

	assert_string(game_fn_draw_sprite_path, "ok"SLASH"resource_is_a_directory");
}

static void ok_return(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 42.0);
}

static void ok_return_from_on_fn(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_return_from_on_fn_minimal(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");
}

static void ok_return_with_no_value(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_same_variable_name_in_different_functions(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 2);

	assert_number(game_fn_initialize_x, 69.0);
}

static void ok_spawn_d(void* grug_state, void* file_id) {
	assert_call_count(spawn_d, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(spawn_d, 1);

	assert_string(game_fn_spawn_d_name, "ok:input");
}

static void ok_spill_args_to_game_fn(void* grug_state, void* file_id) {
	assert_call_count(motherload, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(motherload, 1);

	assert_number(game_fn_motherload_i1, 1.0);
	assert_number(game_fn_motherload_i2, 2.0);
	assert_number(game_fn_motherload_i3, 3.0);
	assert_number(game_fn_motherload_i4, 4.0);
	assert_number(game_fn_motherload_i5, 5.0);
	assert_number(game_fn_motherload_i6, 6.0);
	assert_number(game_fn_motherload_i7, 7.0);
	assert_number(game_fn_motherload_f1, 1.0);
	assert_number(game_fn_motherload_f2, 2.0);
	assert_number(game_fn_motherload_f3, 3.0);
	assert_number(game_fn_motherload_f4, 4.0);
	assert_number(game_fn_motherload_f5, 5.0);
	assert_number(game_fn_motherload_f6, 6.0);
	assert_number(game_fn_motherload_f7, 7.0);
	assert_number(game_fn_motherload_f8, 8.0);
	assert_id(game_fn_motherload_id, 42);
	assert_number(game_fn_motherload_f9, 9.0);
}

static void ok_spill_args_to_game_fn_subless(void* grug_state, void* file_id) {
	assert_call_count(motherload_subless, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(motherload_subless, 1);

	assert_number(game_fn_motherload_subless_i1, 1.0);
	assert_number(game_fn_motherload_subless_i2, 2.0);
	assert_number(game_fn_motherload_subless_i3, 3.0);
	assert_number(game_fn_motherload_subless_i4, 4.0);
	assert_number(game_fn_motherload_subless_i5, 5.0);
	assert_number(game_fn_motherload_subless_i6, 6.0);
	assert_number(game_fn_motherload_subless_i7, 7.0);
	assert_number(game_fn_motherload_subless_f1, 1.0);
	assert_number(game_fn_motherload_subless_f2, 2.0);
	assert_number(game_fn_motherload_subless_f3, 3.0);
	assert_number(game_fn_motherload_subless_f4, 4.0);
	assert_number(game_fn_motherload_subless_f5, 5.0);
	assert_number(game_fn_motherload_subless_f6, 6.0);
	assert_number(game_fn_motherload_subless_f7, 7.0);
	assert_number(game_fn_motherload_subless_f8, 8.0);
	assert_number(game_fn_motherload_subless_f9, 9.0);
	assert_id(game_fn_motherload_subless_id, 42);
	assert_number(game_fn_motherload_subless_f10, 10.0);
}

static void ok_spill_args_to_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(motherload, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(motherload, 1);

	assert_number(game_fn_motherload_i1, 1.0);
	assert_number(game_fn_motherload_i2, 2.0);
	assert_number(game_fn_motherload_i3, 3.0);
	assert_number(game_fn_motherload_i4, 4.0);
	assert_number(game_fn_motherload_i5, 5.0);
	assert_number(game_fn_motherload_i6, 6.0);
	assert_number(game_fn_motherload_i7, 7.0);
	assert_number(game_fn_motherload_f1, 1.0);
	assert_number(game_fn_motherload_f2, 2.0);
	assert_number(game_fn_motherload_f3, 3.0);
	assert_number(game_fn_motherload_f4, 4.0);
	assert_number(game_fn_motherload_f5, 5.0);
	assert_number(game_fn_motherload_f6, 6.0);
	assert_number(game_fn_motherload_f7, 7.0);
	assert_number(game_fn_motherload_f8, 8.0);
	assert_id(game_fn_motherload_id, 42);
	assert_number(game_fn_motherload_f9, 9.0);
}

static void ok_spill_args_to_helper_fn_32_bit_f32(void* grug_state, void* file_id) {
	assert_call_count(offset_32_bit_f32, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(offset_32_bit_f32, 1);

	assert_string(game_fn_offset_32_bit_f32_s1, "1");
	assert_string(game_fn_offset_32_bit_f32_s2, "2");
	assert_string(game_fn_offset_32_bit_f32_s3, "3");
	assert_string(game_fn_offset_32_bit_f32_s4, "4");
	assert_string(game_fn_offset_32_bit_f32_s5, "5");
	assert_string(game_fn_offset_32_bit_f32_s6, "6");
	assert_string(game_fn_offset_32_bit_f32_s7, "7");
	assert_string(game_fn_offset_32_bit_f32_s8, "8");
	assert_string(game_fn_offset_32_bit_f32_s9, "9");
	assert_string(game_fn_offset_32_bit_f32_s10, "10");
	assert_string(game_fn_offset_32_bit_f32_s11, "11");
	assert_string(game_fn_offset_32_bit_f32_s12, "12");
	assert_string(game_fn_offset_32_bit_f32_s13, "13");
	assert_string(game_fn_offset_32_bit_f32_s14, "14");
	assert_string(game_fn_offset_32_bit_f32_s15, "15");
	assert_number(game_fn_offset_32_bit_f32_f1, 1.0);
	assert_number(game_fn_offset_32_bit_f32_f2, 2.0);
	assert_number(game_fn_offset_32_bit_f32_f3, 3.0);
	assert_number(game_fn_offset_32_bit_f32_f4, 4.0);
	assert_number(game_fn_offset_32_bit_f32_f5, 5.0);
	assert_number(game_fn_offset_32_bit_f32_f6, 6.0);
	assert_number(game_fn_offset_32_bit_f32_f7, 7.0);
	assert_number(game_fn_offset_32_bit_f32_f8, 8.0);
	assert_number(game_fn_offset_32_bit_f32_g, 1.0);
}

static void ok_spill_args_to_helper_fn_32_bit_i32(void* grug_state, void* file_id) {
	assert_call_count(offset_32_bit_i32, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(offset_32_bit_i32, 1);

	assert_number(game_fn_offset_32_bit_i32_f1, 1.0);
	assert_number(game_fn_offset_32_bit_i32_f2, 2.0);
	assert_number(game_fn_offset_32_bit_i32_f3, 3.0);
	assert_number(game_fn_offset_32_bit_i32_f4, 4.0);
	assert_number(game_fn_offset_32_bit_i32_f5, 5.0);
	assert_number(game_fn_offset_32_bit_i32_f6, 6.0);
	assert_number(game_fn_offset_32_bit_i32_f7, 7.0);
	assert_number(game_fn_offset_32_bit_i32_f8, 8.0);
	assert_number(game_fn_offset_32_bit_i32_f9, 9.0);
	assert_number(game_fn_offset_32_bit_i32_f10, 10.0);
	assert_number(game_fn_offset_32_bit_i32_f11, 11.0);
	assert_number(game_fn_offset_32_bit_i32_f12, 12.0);
	assert_number(game_fn_offset_32_bit_i32_f13, 13.0);
	assert_number(game_fn_offset_32_bit_i32_f14, 14.0);
	assert_number(game_fn_offset_32_bit_i32_f15, 15.0);
	assert_number(game_fn_offset_32_bit_i32_f16, 16.0);
	assert_number(game_fn_offset_32_bit_i32_f17, 17.0);
	assert_number(game_fn_offset_32_bit_i32_f18, 18.0);
	assert_number(game_fn_offset_32_bit_i32_f19, 19.0);
	assert_number(game_fn_offset_32_bit_i32_f20, 20.0);
	assert_number(game_fn_offset_32_bit_i32_f21, 21.0);
	assert_number(game_fn_offset_32_bit_i32_f22, 22.0);
	assert_number(game_fn_offset_32_bit_i32_f23, 23.0);
	assert_number(game_fn_offset_32_bit_i32_f24, 24.0);
	assert_number(game_fn_offset_32_bit_i32_f25, 25.0);
	assert_number(game_fn_offset_32_bit_i32_f26, 26.0);
	assert_number(game_fn_offset_32_bit_i32_f27, 27.0);
	assert_number(game_fn_offset_32_bit_i32_f28, 28.0);
	assert_number(game_fn_offset_32_bit_i32_f29, 29.0);
	assert_number(game_fn_offset_32_bit_i32_f30, 30.0);
	assert_number(game_fn_offset_32_bit_i32_i1, 1.0);
	assert_number(game_fn_offset_32_bit_i32_i2, 2.0);
	assert_number(game_fn_offset_32_bit_i32_i3, 3.0);
	assert_number(game_fn_offset_32_bit_i32_i4, 4.0);
	assert_number(game_fn_offset_32_bit_i32_i5, 5.0);
	assert_number(game_fn_offset_32_bit_i32_g, 6.0);
}

static void ok_spill_args_to_helper_fn_32_bit_string(void* grug_state, void* file_id) {
	assert_call_count(offset_32_bit_string, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(offset_32_bit_string, 1);

	assert_number(game_fn_offset_32_bit_string_f1, 1.0);
	assert_number(game_fn_offset_32_bit_string_f2, 2.0);
	assert_number(game_fn_offset_32_bit_string_f3, 3.0);
	assert_number(game_fn_offset_32_bit_string_f4, 4.0);
	assert_number(game_fn_offset_32_bit_string_f5, 5.0);
	assert_number(game_fn_offset_32_bit_string_f6, 6.0);
	assert_number(game_fn_offset_32_bit_string_f7, 7.0);
	assert_number(game_fn_offset_32_bit_string_f8, 8.0);
	assert_number(game_fn_offset_32_bit_string_f9, 9.0);
	assert_number(game_fn_offset_32_bit_string_f10, 10.0);
	assert_number(game_fn_offset_32_bit_string_f11, 11.0);
	assert_number(game_fn_offset_32_bit_string_f12, 12.0);
	assert_number(game_fn_offset_32_bit_string_f13, 13.0);
	assert_number(game_fn_offset_32_bit_string_f14, 14.0);
	assert_number(game_fn_offset_32_bit_string_f15, 15.0);
	assert_number(game_fn_offset_32_bit_string_f16, 16.0);
	assert_number(game_fn_offset_32_bit_string_f17, 17.0);
	assert_number(game_fn_offset_32_bit_string_f18, 18.0);
	assert_number(game_fn_offset_32_bit_string_f19, 19.0);
	assert_number(game_fn_offset_32_bit_string_f20, 20.0);
	assert_number(game_fn_offset_32_bit_string_f21, 21.0);
	assert_number(game_fn_offset_32_bit_string_f22, 22.0);
	assert_number(game_fn_offset_32_bit_string_f23, 23.0);
	assert_number(game_fn_offset_32_bit_string_f24, 24.0);
	assert_number(game_fn_offset_32_bit_string_f25, 25.0);
	assert_number(game_fn_offset_32_bit_string_f26, 26.0);
	assert_number(game_fn_offset_32_bit_string_f27, 27.0);
	assert_number(game_fn_offset_32_bit_string_f28, 28.0);
	assert_number(game_fn_offset_32_bit_string_f29, 29.0);
	assert_number(game_fn_offset_32_bit_string_f30, 30.0);
	assert_string(game_fn_offset_32_bit_string_s1, "1");
	assert_string(game_fn_offset_32_bit_string_s2, "2");
	assert_string(game_fn_offset_32_bit_string_s3, "3");
	assert_string(game_fn_offset_32_bit_string_s4, "4");
	assert_string(game_fn_offset_32_bit_string_s5, "5");
	assert_number(game_fn_offset_32_bit_string_g, 1.0);
}

static void ok_spill_args_to_helper_fn_subless(void* grug_state, void* file_id) {
	assert_call_count(motherload_subless, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(motherload_subless, 1);

	assert_number(game_fn_motherload_subless_i1, 1.0);
	assert_number(game_fn_motherload_subless_i2, 2.0);
	assert_number(game_fn_motherload_subless_i3, 3.0);
	assert_number(game_fn_motherload_subless_i4, 4.0);
	assert_number(game_fn_motherload_subless_i5, 5.0);
	assert_number(game_fn_motherload_subless_i6, 6.0);
	assert_number(game_fn_motherload_subless_i7, 7.0);
	assert_number(game_fn_motherload_subless_f1, 1.0);
	assert_number(game_fn_motherload_subless_f2, 2.0);
	assert_number(game_fn_motherload_subless_f3, 3.0);
	assert_number(game_fn_motherload_subless_f4, 4.0);
	assert_number(game_fn_motherload_subless_f5, 5.0);
	assert_number(game_fn_motherload_subless_f6, 6.0);
	assert_number(game_fn_motherload_subless_f7, 7.0);
	assert_number(game_fn_motherload_subless_f8, 8.0);
	assert_number(game_fn_motherload_subless_f9, 9.0);
	assert_id(game_fn_motherload_subless_id, 42);
	assert_number(game_fn_motherload_subless_f10, 10.0);
}

static void ok_stack_16_byte_alignment(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 42.0);
}

static void ok_stack_16_byte_alignment_midway(void* grug_state, void* file_id) {
	assert_call_count(magic, 0);
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(magic, 1);
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 42.0 + 42.0);
}

static void ok_string_can_be_passed_to_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(say, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(say, 1);

	assert_string(game_fn_say_message, "foo");
}

static void ok_string_duplicate(void* grug_state, void* file_id) {
	assert_call_count(talk, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(talk, 1);

	assert_string(game_fn_talk_message1, "foo");
	assert_string(game_fn_talk_message2, "bar");
	assert_string(game_fn_talk_message3, "bar");
	assert_string(game_fn_talk_message4, "baz");
}

static void ok_string_eq_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_string_eq_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_string_eq_true_empty(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_string_ne_false(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_string_ne_false_empty(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_false(game_fn_initialize_bool_b);
}

static void ok_string_ne_true(void* grug_state, void* file_id) {
	assert_call_count(initialize_bool, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize_bool, 1);

	assert_true(game_fn_initialize_bool_b);
}

static void ok_string_returned_by_game_fn(void* grug_state, void* file_id) {
	assert_call_count(get_os, 0);
	assert_call_count(has_string, 0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_os, 1);
	assert_call_count(has_string, 1);

	assert_string(game_fn_has_string_str, "foo");
}

static void ok_string_returned_by_game_fn_assigned_to_member(void* grug_state, void* file_id) {
	assert_call_count(get_os, 1); // Called by init_globals()
	assert_call_count(has_string, 0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_os, 1); // Called by init_globals()
	assert_call_count(has_string, 1);

	assert_string(game_fn_has_string_str, "foo");
}

static void ok_string_returned_by_helper_fn(void* grug_state, void* file_id) {
	assert_call_count(has_string, 0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(has_string, 1);

	assert_string(game_fn_has_string_str, "foo");
}

static void ok_string_returned_by_helper_fn_from_game_fn(void* grug_state, void* file_id) {
	assert_call_count(get_os, 0);
	assert_call_count(has_string, 0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(get_os, 1);
	assert_call_count(has_string, 1);

	assert_string(game_fn_has_string_str, "foo");
}

static void ok_sub_rsp_32_bits_local_variables_i32(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 30);

	assert_number(game_fn_initialize_x, 30.0);
}

static void ok_sub_rsp_32_bits_local_variables_id(void* grug_state, void* file_id) {
	assert_call_count(set_d, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(set_d, 15);

	assert_id(game_fn_set_d_target, 42);
}

static void ok_subtraction_negative_result(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, -3.0);
}

static void ok_subtraction_positive_result(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 3.0);
}

static void ok_unprintable_character_in_comment(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_unprintable_character_in_string(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;
}

static void ok_variable(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 42.0);
}

static void ok_variable_does_not_shadow_in_different_if_statement(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 2);

	assert_number(game_fn_initialize_x, 69.0);
}

static void ok_variable_reassignment(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 69.0);
}

static void ok_variable_reassignment_does_not_dealloc_outer_variable(void* grug_state, void* file_id) {
	assert_call_count(initialize, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(initialize, 1);

	assert_number(game_fn_initialize_x, 69.0);
}

static void ok_variable_string_global(void* grug_state, void* file_id) {
	assert_call_count(say, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(say, 1);

	assert_string(game_fn_say_message, "foo");
}

static void ok_variable_string_local(void* grug_state, void* file_id) {
	assert_call_count(say, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(say, 1);

	assert_string(game_fn_say_message, "foo");
}

static void ok_void_function_early_return(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 1);
}

static void ok_while_false(void* grug_state, void* file_id) {
	assert_call_count(nothing, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(nothing, 2);
}

static void ok_write_to_global_variable(void* grug_state, void* file_id) {
	assert_call_count(max, 0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(max, 1);

	assert_number(game_fn_max_x, 43.0);
	assert_number(game_fn_max_y, 69.0);
}

void grug_tests_runtime_error_handler(const char *reason, enum grug_runtime_error_type type, const char *on_fn_name, const char *on_fn_path) {
	had_runtime_error = true;
	error_handler_call_count++;

	strcpy(runtime_error_reason, reason);
	runtime_error_type = type;
	strcpy(runtime_error_on_fn_name, on_fn_name);
	strcpy(runtime_error_on_fn_path, on_fn_path);
}

static void runtime_error_all(void* grug_state, void* file_id) {
	call_export_fn_argless(grug_state, file_id, "on_a");

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_STACK_OVERFLOW);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"all"SLASH"input-D.grug");
}

static void runtime_error_game_fn_error(void* grug_state, void* file_id) {
	assert_call_count(cause_game_fn_error, 0);
	assert_error_handler_call_count(0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(cause_game_fn_error, 1);
	assert_error_handler_call_count(1);

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_GAME_FN_ERROR);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"game_fn_error"SLASH"input-D.grug");
}

static void runtime_error_game_fn_error_global_scope(void* grug_state, void* file_id) {
	(void)grug_state;
	(void)file_id;

	assert_call_count(cause_game_fn_error, 1);
	assert_error_handler_call_count(1);

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_GAME_FN_ERROR);

	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"game_fn_error_global_scope"SLASH"input-A.grug");
}

static void runtime_error_game_fn_error_once(void* grug_state, void* file_id) {
	assert_call_count(cause_game_fn_error, 0);
	assert_error_handler_call_count(0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(cause_game_fn_error, 1);
	assert_error_handler_call_count(1);

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_GAME_FN_ERROR);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"game_fn_error_once"SLASH"input-E.grug");

	had_runtime_error = false;

	assert_call_count(cause_game_fn_error, 1);
	assert_call_count(nothing, 0);
	assert_error_handler_call_count(1);
	call_export_fn_argless(grug_state, file_id, "on_b");
	assert_call_count(cause_game_fn_error, 1);
	assert_call_count(nothing, 1);
	assert_error_handler_call_count(1);

	assert_false(had_runtime_error);
}

static void runtime_error_on_fn_calls_erroring_on_fn(void* grug_state, void* file_id) {
	assert_call_count(call_on_b_fn, 0);
	assert_call_count(cause_game_fn_error, 0);
	assert_call_count(nothing, 0);
	assert_error_handler_call_count(0);
    call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(call_on_b_fn, 1);
	assert_call_count(cause_game_fn_error, 1);
	assert_call_count(nothing, 0);
	assert_error_handler_call_count(1);

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_GAME_FN_ERROR);

	assert_string(runtime_error_on_fn_name, "on_b");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"on_fn_calls_erroring_on_fn"SLASH"input-E.grug");
}

static void runtime_error_on_fn_errors_after_it_calls_other_on_fn(void* grug_state, void* file_id) {
	assert_call_count(call_on_b_fn, 0);
	assert_call_count(nothing, 0);
	assert_call_count(cause_game_fn_error, 0);
	assert_error_handler_call_count(0);
	call_export_fn_argless(grug_state, file_id, "on_a");
	assert_call_count(call_on_b_fn, 1);
	assert_call_count(nothing, 1);
	assert_call_count(cause_game_fn_error, 1);
	assert_error_handler_call_count(1);

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_GAME_FN_ERROR);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"on_fn_errors_after_it_calls_other_on_fn"SLASH"input-E.grug");
}

static void runtime_error_stack_overflow(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_STACK_OVERFLOW);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"stack_overflow"SLASH"input-D.grug");
}

static void runtime_error_time_limit_exceeded(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_TIME_LIMIT_EXCEEDED);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"time_limit_exceeded"SLASH"input-D.grug");
}

static void runtime_error_time_limit_exceeded_exponential_calls(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_TIME_LIMIT_EXCEEDED);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"time_limit_exceeded_exponential_calls"SLASH"input-D.grug");
}

static void runtime_error_time_limit_exceeded_fibonacci(void* grug_state, void* file_id) {
    call_export_fn_argless(grug_state, file_id, "on_a");

	assert_true(had_runtime_error);

	assert_runtime_error_type(GRUG_ON_FN_TIME_LIMIT_EXCEEDED);

	assert_string(runtime_error_on_fn_name, "on_a");
	assert_string(runtime_error_on_fn_path, "err_runtime"SLASH"time_limit_exceeded_fibonacci"SLASH"input-D.grug");
}

static void add_error_tests(void) {
	ADD_TEST_ERROR(assignment_isnt_expression, "D");
	ADD_TEST_ERROR(bool_cant_be_initialized_with_1, "D");
	ADD_TEST_ERROR(bool_ge, "D");
	ADD_TEST_ERROR(bool_gt, "D");
	ADD_TEST_ERROR(bool_le, "D");
	ADD_TEST_ERROR(bool_lt, "D");
	ADD_TEST_ERROR(bool_unary_minus, "D");
	ADD_TEST_ERROR(cant_add_strings, "D");
	ADD_TEST_ERROR(cant_break_outside_of_loop, "D");
	ADD_TEST_ERROR(cant_continue_outside_of_loop, "D");
	ADD_TEST_ERROR(cant_declare_me_globally, "A");
	ADD_TEST_ERROR(cant_declare_me_locally, "D");
	ADD_TEST_ERROR(cant_declare_variable_in_fn_call, "D");
	ADD_TEST_ERROR(cant_redefine_global, "D");
	ADD_TEST_ERROR(comment_at_the_end_of_another_statement, "D");
	ADD_TEST_ERROR(comment_empty, "A");
	ADD_TEST_ERROR(comment_lone_global_at_end, "A");
	ADD_TEST_ERROR(double_negation, "D");
	ADD_TEST_ERROR(double_not, "D");
	ADD_TEST_ERROR(empty_helper_fn, "D");
	ADD_TEST_ERROR(empty_line_after_group, "D");
	ADD_TEST_ERROR(empty_line_at_start_of_file, "D");
	ADD_TEST_ERROR(empty_line_before_group, "D");
	ADD_TEST_ERROR(empty_line_fn_group, "D");
	ADD_TEST_ERROR(empty_line_twice_at_end_of_file, "D");
	ADD_TEST_ERROR(empty_line_twice_between_local_statements, "D");
	ADD_TEST_ERROR(empty_line_while_group, "D");
	ADD_TEST_ERROR(empty_on_fn, "D");
	ADD_TEST_ERROR(entity_cant_be_empty_string, "D");
	ADD_TEST_ERROR(entity_cant_be_passed_to_helper_fn, "D");
	ADD_TEST_ERROR(entity_cant_be_passed_to_helper_fn_2, "D");
	ADD_TEST_ERROR(entity_has_invalid_entity_name_colon, "D");
	ADD_TEST_ERROR(entity_has_invalid_entity_name_uppercase, "D");
	ADD_TEST_ERROR(entity_has_invalid_mod_name_uppercase, "D");
	ADD_TEST_ERROR(entity_mod_name_and_entity_name_is_missing, "D");
	ADD_TEST_ERROR(entity_mod_name_cant_be_current_mod, "D");
	ADD_TEST_ERROR(entity_mod_name_is_missing, "D");
	ADD_TEST_ERROR(entity_name_is_missing, "D");
	ADD_TEST_ERROR(entity_string_must_be_prefixed, "D");
	ADD_TEST_ERROR(entity_type_for_helper_fn_return_type, "D");
	ADD_TEST_ERROR(f32_missing_digit_after_decimal_point, "D");
	ADD_TEST_ERROR(f32_too_big, "D");
	ADD_TEST_ERROR(f32_too_close_to_zero_negative, "D");
	ADD_TEST_ERROR(f32_too_close_to_zero_positive_1, "D");
	ADD_TEST_ERROR(f32_too_close_to_zero_positive_2, "D");
	ADD_TEST_ERROR(f32_too_small, "D");
	ADD_TEST_ERROR(file_name_entity_type_doesnt_start_uppercase, "a");
	ADD_TEST_ERROR(file_name_entity_type_invalid_character, "Foo#Bar");
	ADD_TEST_ERROR(file_name_missing_entity_type, "");
	ADD_TEST_ERROR_FILE_NAME(file_name_missing_dash, "input.grug");
	ADD_TEST_ERROR_FILE_NAME(file_name_missing_period, "input-A");
	ADD_TEST_ERROR(game_fn_call_gets_void, "D");
	ADD_TEST_ERROR(game_fn_does_not_exist, "D");
	ADD_TEST_ERROR(game_function_call_gets_wrong_arg_type, "D");
	ADD_TEST_ERROR(game_function_call_less_args_expected, "D");
	ADD_TEST_ERROR(game_function_call_more_args_expected, "D");
	ADD_TEST_ERROR(game_function_call_no_args_expected, "D");
	ADD_TEST_ERROR(global_cant_be_me, "A");
	ADD_TEST_ERROR(global_cant_call_helper_fn, "A");
	ADD_TEST_ERROR(global_cant_call_on_fn, "D");
	ADD_TEST_ERROR(global_cant_use_later_global, "A");
	ADD_TEST_ERROR(global_id_cant_be_reassigned, "D");
	ADD_TEST_ERROR(global_variable_after_on_fns, "D");
	ADD_TEST_ERROR(global_variable_already_uses_local_variable_name, "D");
	ADD_TEST_ERROR(global_variable_contains_double_negation, "A");
	ADD_TEST_ERROR(global_variable_contains_double_not, "A");
	ADD_TEST_ERROR(global_variable_contains_entity, "A");
	ADD_TEST_ERROR(global_variable_definition_cant_use_itself, "A");
	ADD_TEST_ERROR(global_variable_definition_missing_type, "A");
	ADD_TEST_ERROR(global_variable_definition_requires_value_i32, "D");
	ADD_TEST_ERROR(global_variable_definition_requires_value_string, "D");
	ADD_TEST_ERROR(helper_fn_call_gets_void, "D");
	ADD_TEST_ERROR(helper_fn_call_gets_wrong_arg_type, "D");
	ADD_TEST_ERROR(helper_fn_call_less_args_expected, "D");
	ADD_TEST_ERROR(helper_fn_call_more_args_expected, "D");
	ADD_TEST_ERROR(helper_fn_call_no_args_expected, "D");
	ADD_TEST_ERROR(helper_fn_defined_before_first_helper_fn_usage, "D");
	ADD_TEST_ERROR(helper_fn_defined_between_on_fns, "E");
	ADD_TEST_ERROR(helper_fn_different_return_value_expected, "D");
	ADD_TEST_ERROR(helper_fn_does_not_exist, "D");
	ADD_TEST_ERROR(helper_fn_duplicate, "D");
	ADD_TEST_ERROR(helper_fn_id_arg_gets_number, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_1, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_2, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_3, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_4, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_5, "D");
	ADD_TEST_ERROR(helper_fn_missing_return_statement, "D");
	ADD_TEST_ERROR(helper_fn_no_return_value_expected, "D");
	ADD_TEST_ERROR(helper_fn_return_value_expected, "D");
	ADD_TEST_ERROR(helper_fn_return_with_comment_after_it, "D");
	ADD_TEST_ERROR(i32_logical_not, "D");
	ADD_TEST_ERROR(id_invalid_binary_op, "D");
	ADD_TEST_ERROR(id_return, "D");
	ADD_TEST_ERROR(id_store_in_non_id_global, "A");
	ADD_TEST_ERROR(id_store_in_non_id_local, "D");
	ADD_TEST_ERROR(id_store_in_non_id_local_2, "D");
	ADD_TEST_ERROR(indentation_going_down_by_2, "D");
	ADD_TEST_ERROR(indented_call_argument, "D");
	ADD_TEST_ERROR(indented_call_arguments, "D");
	ADD_TEST_ERROR(indented_helper_fn_parameter, "D");
	ADD_TEST_ERROR(indented_helper_fn_parameters, "D");
	ADD_TEST_ERROR(indented_on_fn_parameter, "F");
	ADD_TEST_ERROR(indented_on_fn_parameters, "G");
	ADD_TEST_ERROR(invalid_end_of_block, "D");
	ADD_TEST_ERROR(line_continuation, "A");
	ADD_TEST_ERROR(local_variable_already_exists, "D");
	ADD_TEST_ERROR(local_variable_contains_entity, "D");
	ADD_TEST_ERROR(local_variable_definition_cant_use_itself, "D");
	ADD_TEST_ERROR(local_variable_definition_missing_type, "D");
	ADD_TEST_ERROR(me_cant_be_written_to, "D");
	ADD_TEST_ERROR(me_plus_1, "D");
	ADD_TEST_ERROR(me_plus_me, "D");
	ADD_TEST_ERROR(missing_empty_line_between_global_and_on_fn, "D");
	ADD_TEST_ERROR(missing_empty_line_between_on_fn_and_helper_fn, "D");
	ADD_TEST_ERROR(mixing_custom_ids_binary_expr, "D");
	ADD_TEST_ERROR(mixing_custom_ids_game_fn_param_type, "D");
	ADD_TEST_ERROR(mixing_custom_ids_game_fn_return_type, "D");
	ADD_TEST_ERROR(mixing_custom_ids_global_new, "A");
	ADD_TEST_ERROR(mixing_custom_ids_global_to_local, "D");
	ADD_TEST_ERROR(mixing_custom_ids_helper_fn_param_type, "D");
	ADD_TEST_ERROR(mixing_custom_ids_helper_fn_return_type, "D");
	ADD_TEST_ERROR(mixing_custom_ids_local_existing, "D");
	ADD_TEST_ERROR(mixing_custom_ids_local_new, "D");
	ADD_TEST_ERROR(mixing_custom_ids_on_fn_param_type, "T");
	ADD_TEST_ERROR(newline_statement, "D");
	ADD_TEST_ERROR(no_space_between_comment_character_and_comment, "D");
	ADD_TEST_ERROR(not_followed_by_negation, "D");
	ADD_TEST_ERROR(null_byte_in_comment, "A");
	ADD_TEST_ERROR(null_byte_in_string, "A");
	ADD_TEST_ERROR(number_and, "D");
	ADD_TEST_ERROR(number_assigned_to_global_id, "D");
	ADD_TEST_ERROR(number_assigned_to_local_id, "D");
	ADD_TEST_ERROR(number_or, "D");
	ADD_TEST_ERROR(number_period_twice_1, "A");
	ADD_TEST_ERROR(number_period_twice_2, "A");
	ADD_TEST_ERROR(on_fn_cant_be_called_by_helper_fn, "D");
	ADD_TEST_ERROR(on_fn_cant_be_called_by_on_fn, "D");
	ADD_TEST_ERROR(on_fn_defined_after_helper_fn, "D");
	ADD_TEST_ERROR(on_fn_duplicate, "D");
	ADD_TEST_ERROR(on_fn_was_not_declared_in_entity, "A");
	ADD_TEST_ERROR(on_fn_wrong_order, "E");
	ADD_TEST_ERROR(on_function_gets_wrong_arg_name, "F");
	ADD_TEST_ERROR(on_function_gets_wrong_arg_type, "F");
	ADD_TEST_ERROR(on_function_less_args_expected, "F");
	ADD_TEST_ERROR(on_function_more_args_expected, "G");
	ADD_TEST_ERROR(on_function_no_args_expected, "D");
	ADD_TEST_ERROR(on_function_no_return_value_expected, "D");
	ADD_TEST_ERROR(open_parenthesis_after_number, "D");
	ADD_TEST_ERROR(parameter_shadows_global_variable, "F");
	ADD_TEST_ERROR(pass_bool_to_i32_game_param, "D");
	ADD_TEST_ERROR(pass_bool_to_i32_helper_param, "D");
	ADD_TEST_ERROR(print_csv_wrong_extension_1, "D");
	ADD_TEST_ERROR(print_csv_wrong_extension_2, "D");
	ADD_TEST_ERROR(print_csv_wrong_extension_3, "D");
	ADD_TEST_ERROR(resource_cant_be_empty_string, "D");
	ADD_TEST_ERROR(resource_cant_be_passed_to_helper_fn, "D");
	ADD_TEST_ERROR(resource_cant_be_passed_to_helper_fn_2, "D");
	ADD_TEST_ERROR(resource_cant_contain_backslash, "D");
	ADD_TEST_ERROR(resource_cant_contain_double_slash, "D");
	ADD_TEST_ERROR(resource_cant_end_with_dot, "D");
	ADD_TEST_ERROR(resource_cant_go_up_to_parent_directory_1, "D");
	ADD_TEST_ERROR(resource_cant_go_up_to_parent_directory_2, "D");
	ADD_TEST_ERROR(resource_cant_go_up_to_parent_directory_3, "D");
	ADD_TEST_ERROR(resource_cant_go_up_to_parent_directory_4, "D");
	ADD_TEST_ERROR(resource_cant_have_leading_slash, "D");
	ADD_TEST_ERROR(resource_cant_have_trailing_slash, "D");
	ADD_TEST_ERROR(resource_cant_refer_to_current_directory_1, "D");
	ADD_TEST_ERROR(resource_cant_refer_to_current_directory_2, "D");
	ADD_TEST_ERROR(resource_cant_refer_to_current_directory_3, "D");
	ADD_TEST_ERROR(resource_cant_refer_to_current_directory_4, "D");
	ADD_TEST_ERROR(resource_string_must_be_prefixed, "D");
	ADD_TEST_ERROR(resource_type_for_global, "A");
	ADD_TEST_ERROR(resource_type_for_helper_fn_return_type, "D");
	ADD_TEST_ERROR(resource_type_for_local, "D");
	ADD_TEST_ERROR(spaces_per_indent, "D");
	ADD_TEST_ERROR(string_pointer_arithmetic, "D");
	ADD_TEST_ERROR(trailing_space_in_comment, "D");
	ADD_TEST_ERROR(unary_plus_does_not_exist, "D");
	ADD_TEST_ERROR(unclosed_double_quote, "A");
	ADD_TEST_ERROR(unknown_variable, "D");
	ADD_TEST_ERROR(unrecognized_character, "A");
	ADD_TEST_ERROR(unused_expr_result, "D");
	ADD_TEST_ERROR(variable_assignment_before_definition, "D");
	ADD_TEST_ERROR(variable_definition_requires_value_i32, "D");
	ADD_TEST_ERROR(variable_definition_requires_value_string, "D");
	ADD_TEST_ERROR(variable_not_accessible, "D");
	ADD_TEST_ERROR(variable_same_name_missing_type, "D");
	ADD_TEST_ERROR(variable_shadows_argument, "F");
	ADD_TEST_ERROR(variable_shadows_global_variable, "D");
	ADD_TEST_ERROR(variable_shadows_local_variable, "D");
	ADD_TEST_ERROR(variable_statement_missing_assignment, "D");
	ADD_TEST_ERROR(variable_used_before_definition, "D");
	ADD_TEST_ERROR(wrong_indentation, "D");
	ADD_TEST_ERROR(wrong_type_global_assignment, "D");
	ADD_TEST_ERROR(wrong_type_global_reassignment, "D");
	ADD_TEST_ERROR(wrong_type_local_assignment, "D");
	ADD_TEST_ERROR(wrong_type_local_reassignment, "D");
}

static void add_ok_tests(void) {
	ADD_TEST_OK(addition_as_argument, "D");
	ADD_TEST_OK(addition_as_two_arguments, "D");
	ADD_TEST_OK(addition_with_multiplication, "D");
	ADD_TEST_OK(addition_with_multiplication_2, "D");
	ADD_TEST_OK(and_false_1, "D");
	ADD_TEST_OK(and_false_2, "D");
	ADD_TEST_OK(and_false_3, "D");
	ADD_TEST_OK(and_short_circuit, "D");
	ADD_TEST_OK(and_true, "D");
	ADD_TEST_OK(blocked_alrm, "D");
	ADD_TEST_OK(bool_logical_not_false, "D");
	ADD_TEST_OK(bool_logical_not_true, "D");
	ADD_TEST_OK(bool_returned, "D");
	ADD_TEST_OK(bool_returned_global, "D");
	ADD_TEST_OK(bool_zero_extended_if_statement, "D");
	ADD_TEST_OK(bool_zero_extended_while_statement, "D");
	ADD_TEST_OK(break, "D");
	ADD_TEST_OK(calls_100, "D");
	ADD_TEST_OK(calls_1000, "D");
	ADD_TEST_OK(calls_in_call, "D");
	ADD_TEST_OK(comment_above_block, "D");
	ADD_TEST_OK(comment_above_block_twice, "D");
	ADD_TEST_OK(comment_above_globals, "A");
	ADD_TEST_OK(comment_above_helper_fn, "D");
	ADD_TEST_OK(comment_above_on_fn, "D");
	ADD_TEST_OK(comment_between_globals, "A");
	ADD_TEST_OK(comment_between_statements, "D");
	ADD_TEST_OK(comment_lone_block, "D");
	ADD_TEST_OK(comment_lone_block_at_end, "D");
	ADD_TEST_OK(comment_lone_global, "D");
	ADD_TEST_OK(continue, "D");
	ADD_TEST_OK(custom_id_decays_to_id, "D");
	ADD_TEST_OK(custom_id_transfer_between_globals, "D");
	ADD_TEST_OK(custom_id_with_digits, "A");
	ADD_TEST_OK(division_negative_result, "D");
	ADD_TEST_OK(division_positive_result, "D");
	ADD_TEST_OK(double_negation_with_parentheses, "D");
	ADD_TEST_OK(double_not_with_parentheses, "D");
	ADD_TEST_OK(else_after_else_if_false, "D");
	ADD_TEST_OK(else_after_else_if_true, "D");
	ADD_TEST_OK(else_false, "D");
	ADD_TEST_OK(else_if_false, "D");
	ADD_TEST_OK(else_if_true, "D");
	ADD_TEST_OK(else_true, "D");
	ADD_TEST_OK(empty_file, "A");
	ADD_TEST_OK(empty_line, "D");
	ADD_TEST_OK(entity_and_resource_as_subexpression, "D");
	ADD_TEST_OK(entity_duplicate, "D");
	ADD_TEST_OK(entity_in_on_fn, "D");
	ADD_TEST_OK(entity_in_on_fn_with_mod_specified, "D");
	ADD_TEST_OK(eq_false, "D");
	ADD_TEST_OK(eq_true, "D");
	ADD_TEST_OK(f32_addition, "D");
	ADD_TEST_OK(f32_argument, "D");
	ADD_TEST_OK(f32_division, "D");
	ADD_TEST_OK(f32_eq_false, "D");
	ADD_TEST_OK(f32_eq_true, "D");
	ADD_TEST_OK(f32_ge_false, "D");
	ADD_TEST_OK(f32_ge_true_1, "D");
	ADD_TEST_OK(f32_ge_true_2, "D");
	ADD_TEST_OK(f32_global_variable, "D");
	ADD_TEST_OK(f32_gt_false, "D");
	ADD_TEST_OK(f32_gt_true, "D");
	ADD_TEST_OK(f32_le_false, "D");
	ADD_TEST_OK(f32_le_true_1, "D");
	ADD_TEST_OK(f32_le_true_2, "D");
	ADD_TEST_OK(f32_local_variable, "D");
	ADD_TEST_OK(f32_lt_false, "D");
	ADD_TEST_OK(f32_lt_true, "D");
	ADD_TEST_OK(f32_multiplication, "D");
	ADD_TEST_OK(f32_ne_false, "D");
	ADD_TEST_OK(f32_negated, "D");
	ADD_TEST_OK(f32_ne_true, "D");
	ADD_TEST_OK(f32_passed_to_helper_fn, "D");
	ADD_TEST_OK(f32_passed_to_on_fn, "R");
	ADD_TEST_OK(f32_passing_sin_to_cos, "D");
	ADD_TEST_OK(f32_subtraction, "D");
	ADD_TEST_OK(fibonacci, "D");
	ADD_TEST_OK(ge_false, "D");
	ADD_TEST_OK(ge_true_1, "D");
	ADD_TEST_OK(ge_true_2, "D");
	ADD_TEST_OK(global_2_does_not_have_error_handling, "A");
	ADD_TEST_OK(global_call_using_me, "D");
	ADD_TEST_OK(global_can_use_earlier_global, "D");
	ADD_TEST_OK(global_containing_negation, "D");
	ADD_TEST_OK(global_id, "D");
	ADD_TEST_OK(global_parentheses, "D");
	ADD_TEST_OK(globals, "D");
	ADD_TEST_OK(globals_1000, "A");
	ADD_TEST_OK(globals_1000_string, "A");
	ADD_TEST_OK(globals_32, "A");
	ADD_TEST_OK(globals_64, "A");
	ADD_TEST_OK(gt_false, "D");
	ADD_TEST_OK(gt_true, "D");
	ADD_TEST_OK(helper_fn, "D");
	ADD_TEST_OK(helper_fn_called_in_if, "D");
	ADD_TEST_OK(helper_fn_called_indirectly, "D");
	ADD_TEST_OK(helper_fn_overwriting_param, "D");
	ADD_TEST_OK(helper_fn_returning_void_has_no_return, "D");
	ADD_TEST_OK(helper_fn_returning_void_returns_void, "D");
	ADD_TEST_OK(helper_fn_same_param_name_as_on_fn, "F");
	ADD_TEST_OK(helper_fn_same_param_name_as_other_helper_fn, "F");
	ADD_TEST_OK(i32_max, "D");
	ADD_TEST_OK(i32_min, "D");
	ADD_TEST_OK(i32_negated, "D");
	ADD_TEST_OK(i32_negative_is_smaller_than_positive, "D");
	ADD_TEST_OK(id_binary_expr_false, "D");
	ADD_TEST_OK(id_binary_expr_true, "D");
	ADD_TEST_OK(id_eq_1, "D");
	ADD_TEST_OK(id_eq_2, "D");
	ADD_TEST_OK(id_global_with_id_to_new_id, "D");
	ADD_TEST_OK(id_global_with_opponent_to_new_id, "D");
	ADD_TEST_OK(id_helper_fn_param, "D");
	ADD_TEST_OK(id_local_variable_get_and_set, "D");
	ADD_TEST_OK(id_ne_1, "D");
	ADD_TEST_OK(id_ne_2, "D");
	ADD_TEST_OK(id_on_fn_param, "U");
	ADD_TEST_OK(id_returned_from_helper, "D");
	ADD_TEST_OK(id_with_d_to_new_id_and_id_to_old_id, "D");
	ADD_TEST_OK(id_with_d_to_old_id, "D");
	ADD_TEST_OK(id_with_id_to_new_id, "D");
	ADD_TEST_OK(if_false, "D");
	ADD_TEST_OK(if_true, "D");
	ADD_TEST_OK(le_false, "D");
	ADD_TEST_OK(le_true_1, "D");
	ADD_TEST_OK(le_true_2, "D");
	ADD_TEST_OK(local_id_can_be_reassigned, "D");
	ADD_TEST_OK(lt_false, "D");
	ADD_TEST_OK(lt_true, "D");
	ADD_TEST_OK(max_args, "D");
	ADD_TEST_OK(me, "D");
	ADD_TEST_OK(me_assigned_to_local_variable, "D");
	ADD_TEST_OK(me_passed_to_helper_fn, "D");
	ADD_TEST_OK(mov_32_bits_global_i32, "A");
	ADD_TEST_OK(mov_32_bits_global_id, "A");
	ADD_TEST_OK(multiplication_as_two_arguments, "D");
	ADD_TEST_OK(ne_false, "D");
	ADD_TEST_OK(ne_true, "D");
	ADD_TEST_OK(negate_parenthesized_expr, "D");
	ADD_TEST_OK(negative_literal, "D");
	ADD_TEST_OK(nested_break, "D");
	ADD_TEST_OK(nested_continue, "D");
	ADD_TEST_OK(no_empty_line_between_globals, "A");
	ADD_TEST_OK(no_empty_line_between_statements, "D");
	ADD_TEST_OK(on_fn, "D");
	ADD_TEST_OK(on_fn_calling_game_fn_nothing, "D");
	ADD_TEST_OK(on_fn_calling_game_fn_nothing_twice, "D");
	ADD_TEST_OK(on_fn_calling_game_fn_plt_order, "D");
	ADD_TEST_OK(on_fn_calling_helper_fns, "D");
	ADD_TEST_OK(on_fn_calling_no_game_fn, "D");
	ADD_TEST_OK(on_fn_calling_no_game_fn_but_with_addition, "D");
	ADD_TEST_OK(on_fn_calling_no_game_fn_but_with_global, "D");
	ADD_TEST_OK(on_fn_overwriting_param, "S");
	ADD_TEST_OK(on_fn_passing_argument_to_helper_fn, "D");
	ADD_TEST_OK(on_fn_passing_magic_to_initialize, "D");
	ADD_TEST_OK(on_fn_three, "J");
	ADD_TEST_OK(on_fn_three_unused_first, "J");
	ADD_TEST_OK(on_fn_three_unused_second, "J");
	ADD_TEST_OK(on_fn_three_unused_third, "J");
	ADD_TEST_OK(or_false, "D");
	ADD_TEST_OK(or_short_circuit, "D");
	ADD_TEST_OK(or_true_1, "D");
	ADD_TEST_OK(or_true_2, "D");
	ADD_TEST_OK(or_true_3, "D");
	ADD_TEST_OK(pass_string_argument_to_game_fn, "D");
	ADD_TEST_OK(pass_string_argument_to_helper_fn, "D");
	ADD_TEST_OK(print_csv, "D");
	ADD_TEST_OK(resource_and_entity, "D");
	ADD_TEST_OK(resource_can_contain_dot_1, "D");
	ADD_TEST_OK(resource_can_contain_dot_2, "D");
	ADD_TEST_OK(resource_can_contain_dot_dot_1, "D");
	ADD_TEST_OK(resource_can_contain_dot_dot_2, "D");
	ADD_TEST_OK(resource_can_contain_dot_dot_dot, "D");
	ADD_TEST_OK(resource_duplicate, "D");
	ADD_TEST_OK(resource_is_a_directory, "D");
	ADD_TEST_OK(return, "D");
	ADD_TEST_OK(return_from_on_fn, "D");
	ADD_TEST_OK(return_from_on_fn_minimal, "D");
	ADD_TEST_OK(return_with_no_value, "D");
	ADD_TEST_OK(same_variable_name_in_different_functions, "E");
	ADD_TEST_OK(spawn_d, "D");
	ADD_TEST_OK(spill_args_to_game_fn, "D");
	ADD_TEST_OK(spill_args_to_game_fn_subless, "D");
	ADD_TEST_OK(spill_args_to_helper_fn, "D");
	ADD_TEST_OK(spill_args_to_helper_fn_32_bit_f32, "D");
	ADD_TEST_OK(spill_args_to_helper_fn_32_bit_i32, "D");
	ADD_TEST_OK(spill_args_to_helper_fn_32_bit_string, "D");
	ADD_TEST_OK(spill_args_to_helper_fn_subless, "D");
	ADD_TEST_OK(stack_16_byte_alignment, "D");
	ADD_TEST_OK(stack_16_byte_alignment_midway, "D");
	ADD_TEST_OK(string_can_be_passed_to_helper_fn, "D");
	ADD_TEST_OK(string_duplicate, "D");
	ADD_TEST_OK(string_eq_false, "D");
	ADD_TEST_OK(string_eq_true, "D");
	ADD_TEST_OK(string_eq_true_empty, "D");
	ADD_TEST_OK(string_ne_false, "D");
	ADD_TEST_OK(string_ne_false_empty, "D");
	ADD_TEST_OK(string_ne_true, "D");
	ADD_TEST_OK(string_returned_by_game_fn, "D");
	ADD_TEST_OK(string_returned_by_game_fn_assigned_to_member, "D");
	ADD_TEST_OK(string_returned_by_helper_fn, "D");
	ADD_TEST_OK(string_returned_by_helper_fn_from_game_fn, "D");
	ADD_TEST_OK(sub_rsp_32_bits_local_variables_i32, "D");
	ADD_TEST_OK(sub_rsp_32_bits_local_variables_id, "D");
	ADD_TEST_OK(subtraction_negative_result, "D");
	ADD_TEST_OK(subtraction_positive_result, "D");
	ADD_TEST_OK(unprintable_character_in_comment, "A");
	ADD_TEST_OK(unprintable_character_in_string, "A");
	ADD_TEST_OK(variable, "D");
	ADD_TEST_OK(variable_does_not_shadow_in_different_if_statement, "D");
	ADD_TEST_OK(variable_reassignment, "D");
	ADD_TEST_OK(variable_reassignment_does_not_dealloc_outer_variable, "D");
	ADD_TEST_OK(variable_string_global, "D");
	ADD_TEST_OK(variable_string_local, "D");
	ADD_TEST_OK(void_function_early_return, "D");
	ADD_TEST_OK(while_false, "D");
	ADD_TEST_OK(write_to_global_variable, "D");
}

static void add_runtime_error_tests(void) {
	ADD_TEST_RUNTIME_ERROR(all, "D");
	ADD_TEST_RUNTIME_ERROR(game_fn_error, "D");
	ADD_TEST_RUNTIME_ERROR(game_fn_error_global_scope, "A");
	ADD_TEST_RUNTIME_ERROR(game_fn_error_once, "E");
	ADD_TEST_RUNTIME_ERROR(on_fn_calls_erroring_on_fn, "E");
	ADD_TEST_RUNTIME_ERROR(on_fn_errors_after_it_calls_other_on_fn, "E");
	ADD_TEST_RUNTIME_ERROR(stack_overflow, "D");
	ADD_TEST_RUNTIME_ERROR(time_limit_exceeded, "D");
	ADD_TEST_RUNTIME_ERROR(time_limit_exceeded_exponential_calls, "D");
	ADD_TEST_RUNTIME_ERROR(time_limit_exceeded_fibonacci, "D");
}

void grug_tests_run(
	const char *tests_dir_path_, 
	const char *mod_api_path_, 
	struct grug_state_vtable vtable,
	const char *whitelisted_test_
) {
	// Set globals
	tests_dir_path             = tests_dir_path_;
	mod_api_path               = mod_api_path_;
	whitelisted_test           = whitelisted_test_;

	create_grug_state          = vtable.create_grug_state,
	destroy_grug_state         = vtable.destroy_grug_state,
	compile_grug_file          = vtable.compile_grug_file;
	init_globals               = vtable.init_globals;
	call_export_fn             = vtable.call_export_fn;
	dump_file_to_json          = vtable.dump_file_to_json;
	generate_file_from_json    = vtable.generate_file_from_json;
	game_fn_error              = vtable.game_fn_error;

	if (setvbuf(stdout, NULL, _IOLBF, 64) != 0) {
		fprintf(stderr, "Error: Could not buffer stdout\n");
		exit(EXIT_FAILURE);
	};

	// We only have a single grug_state for now
	void* grug_state = create_grug_state(
		mod_api_path,
		tests_dir_path
	);
	if (!grug_state) {
		fprintf(stderr, "Error: Failed to create grug state\n");
		exit(EXIT_FAILURE);
	}

	add_error_tests();
	add_runtime_error_tests();
	add_ok_tests();

#ifdef SHUFFLES
	#ifdef SEED
	unsigned int seed = SEED;
	#else
	unsigned int seed = (unsigned int)time(NULL);
	#endif

	printf("The seed is %u\n", seed);
	srand(seed);

	for (size_t shuffle = 0; shuffle < SHUFFLES; shuffle++) {
	SHUFFLE(error_test_datas, err_test_datas_size, struct error_test_data);
	SHUFFLE(ok_test_datas, ok_test_datas_size, struct ok_test_data);
	SHUFFLE(runtime_error_test_datas, err_runtime_test_datas_size, struct runtime_error_test_data);
#endif

	run_err_mod_api_tests();
    run_err_spaces_tests(grug_state);

	for (size_t i = 0; i < err_test_datas_size; i++) {
		struct error_test_data fn_data = error_test_datas[i];

		test_error(
			grug_state,
			fn_data.test_name_str,
			fn_data.grug_path,
			fn_data.expected_error_path,
			fn_data.results_path,
			fn_data.grug_output_path
		);
	}

	for (size_t i = 0; i < ok_test_datas_size; i++) {
		struct ok_test_data* fn_data = &ok_test_datas[i];

		printf("Running tests/ok/%s...\n", fn_data->test_name_str);
		reset();

		void* file_id = prologue(grug_state, fn_data->grug_path, fn_data->results_path);

		diff_roundtrip(grug_state, fn_data->grug_path, fn_data->dump_path, fn_data->applied_path);

		init_globals(grug_state, file_id);
		fn_data->file_id = file_id;
		current_file_id = file_id;
		fn_data->run(grug_state, file_id);
	}

	for (size_t i = 0; i < ok_test_datas_size; i++) {
		struct ok_test_data* fn_data = &ok_test_datas[i];

		printf("Rerunning tests/ok/%s...\n", fn_data->test_name_str);
		reset();

		init_globals(grug_state, fn_data->file_id);
		current_file_id = fn_data->file_id;
		fn_data->run(grug_state, fn_data->file_id);
	}

	for (size_t i = 0; i < err_runtime_test_datas_size; i++) {
		struct runtime_error_test_data* fn_data = &runtime_error_test_datas[i];

		printf("Running tests/err_runtime/%s...\n", fn_data->test_name_str);
		reset();

		void* file_id = prologue(grug_state, fn_data->grug_path, fn_data->results_path);

		diff_roundtrip(grug_state, fn_data->grug_path, fn_data->dump_path, fn_data->applied_path);

		init_globals(grug_state, file_id);
		fn_data->file_id = file_id;
		current_file_id = file_id;
		fn_data->run(grug_state, file_id);

		const char *expected_error = get_expected_error(fn_data->expected_error_path);

		if (!streq(runtime_error_reason, expected_error)) {
			fprintf(stderr, "\nError: The error message differs from the expected error message.\n");
			fprintf(stderr, "Output:\n");
			print_string_debug(runtime_error_reason);

			fprintf(stderr, "Expected:\n");
			print_string_debug(expected_error);

			exit(EXIT_FAILURE);
		}
	}

	for (size_t i = 0; i < err_runtime_test_datas_size; i++) {
		struct runtime_error_test_data* fn_data = &runtime_error_test_datas[i];

		printf("Rerunning tests/err_runtime/%s...\n", fn_data->test_name_str);
		reset();

		init_globals(grug_state, fn_data->file_id);
		current_file_id = fn_data->file_id;
		fn_data->run(grug_state, fn_data->file_id);

		const char *expected_error = get_expected_error(fn_data->expected_error_path);

		if (!streq(runtime_error_reason, expected_error)) {
			fprintf(stderr, "\nError: The error message differs from the expected error message.\n");
			fprintf(stderr, "Output:\n");
			print_string_debug(runtime_error_reason);

			fprintf(stderr, "Expected:\n");
			print_string_debug(expected_error);

			exit(EXIT_FAILURE);
		}
	}

#ifdef SHUFFLES
	}
#endif

	printf("\nAll tests passed! 🎉\n");
	fflush(stdout);
	destroy_grug_state(grug_state);
}
