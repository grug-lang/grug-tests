#include "tests.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// From https://stackoverflow.com/a/2114249/13279557
#ifdef __x86_64__
#define ASSERT_16_BYTE_STACK_ALIGNED() {\
	int64_t rsp;\
	\
	__asm__ volatile("mov %%rsp, %0\n\t" : "=r" (rsp));\
	\
	if ((rsp & 0xf) != 0) {\
		static char msg[] = "The stack was not 16-byte aligned!\n";\
		write(STDERR_FILENO, msg, sizeof(msg) - 1);\
		abort();\
	}\
}
#elif __aarch64__
#define ASSERT_16_BYTE_STACK_ALIGNED() {\
	int64_t rsp;\
	\
	__asm__ volatile("mov %0, sp\n\t" : "=r" (rsp));\
	\
	if ((rsp & 0xf) != 0) {\
		static char msg[] = "The stack was not 16-byte aligned!\n";\
		write(STDERR_FILENO, msg, sizeof(msg) - 1);\
		abort();\
	}\
}
#else
#error Unrecognized architecture
#endif

typedef int64_t i64;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static const char *tests_dir_path;
static compile_grug_file_t compile_grug_file;
static init_globals_fn_dispatcher_t init_globals_fn_dispatcher;
static on_fn_dispatcher_t on_fn_args_dispatcher;
static dump_file_to_json_t dump_file_to_json;
static generate_file_from_json_t generate_file_from_json;
static game_fn_error_t game_fn_error;
static const char *whitelisted_test;

struct error_test_data {
	const char *test_name_str;
	const char *grug_path;
	const char *expected_error_path;
	const char *results_path;
	const char *grug_output_path;
	const char *failed_file_path;
};
static struct error_test_data error_test_datas[420420];
static size_t err_test_datas_size;

struct ok_test_data {
	void (*run)(void);
	const char *test_name_str;
	const char *grug_path;
	const char *results_path;
	const char *dump_path;
	const char *applied_path;
	const char *failed_file_path;
	size_t expected_globals_size_value;
};
static struct ok_test_data ok_test_datas[420420];
static size_t ok_test_datas_size;

struct runtime_error_test_data {
	void (*run)(void);
	const char *test_name_str;
	const char *grug_path;
	const char *expected_error_path;
	const char *results_path;
	const char *dump_path;
	const char *applied_path;
	const char *failed_file_path;
	size_t expected_globals_size_value;
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
static size_t game_fn_get_evil_false_call_count;
static size_t game_fn_set_is_happy_call_count;
static size_t game_fn_mega_f32_call_count;
static size_t game_fn_mega_i32_call_count;
static size_t game_fn_draw_call_count;
static size_t game_fn_blocked_alrm_call_count;
static size_t game_fn_spawn_call_count;
static size_t game_fn_has_resource_call_count;
static size_t game_fn_has_entity_call_count;
static size_t game_fn_has_string_call_count;
static size_t game_fn_get_opponent_call_count;
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
static size_t game_fn_retrieve_call_count;
static size_t game_fn_box_i32_call_count;

static bool had_runtime_error = false;
static size_t error_handler_calls = 0;
static const char *runtime_error_reason = NULL;
static enum grug_runtime_error_type runtime_error_type = -1;
static const char *runtime_error_on_fn_name = NULL;
static const char *runtime_error_on_fn_path = NULL;

static bool streq(const char *a, const char *b) {
	return strcmp(a, b) == 0;
}

static void on_fn_dispatcher(const char *on_fn_name, const char *grug_path) {
	on_fn_args_dispatcher(on_fn_name, grug_path, NULL, 0);
}

void game_fn_nothing(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_nothing_call_count++;
}
int32_t game_fn_magic(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_magic_call_count++;

	return 42;
}
static int32_t game_fn_initialize_x;
void game_fn_initialize(int32_t x) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_initialize_call_count++;

	game_fn_initialize_x = x;
}
static bool game_fn_initialize_bool_b;
void game_fn_initialize_bool(bool b) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_initialize_bool_call_count++;

	game_fn_initialize_bool_b = b;
}
static int32_t game_fn_identity_x;
int32_t game_fn_identity(int32_t x) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_identity_call_count++;

	game_fn_identity_x = x;

	return x;
}
static int32_t game_fn_max_x;
static int32_t game_fn_max_y;
int32_t game_fn_max(int32_t x, int32_t y) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_max_call_count++;

	game_fn_max_x = x;
	game_fn_max_y = y;

	return x > y ? x : y;
}
static const char *game_fn_say_message;
void game_fn_say(const char *message) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_say_call_count++;

	game_fn_say_message = message;
}
static float game_fn_sin_x;
float game_fn_sin(float x) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_sin_call_count++;

	game_fn_sin_x = x;

	return sinf(x);
}
static float game_fn_cos_x;
float game_fn_cos(float x) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_cos_call_count++;

	game_fn_cos_x = x;

	return cosf(x);
}
static float game_fn_mega_f1;
static int32_t game_fn_mega_i1;
static bool game_fn_mega_b1;
static float game_fn_mega_f2;
static float game_fn_mega_f3;
static float game_fn_mega_f4;
static bool game_fn_mega_b2;
static int32_t game_fn_mega_i2;
static float game_fn_mega_f5;
static float game_fn_mega_f6;
static float game_fn_mega_f7;
static float game_fn_mega_f8;
static int32_t game_fn_mega_id;
static const char *game_fn_mega_str;
void game_fn_mega(float f1, int32_t i1, bool b1, float f2, float f3, float f4, bool b2, int32_t i2, float f5, float f6, float f7, float f8, uint64_t id, const char *str) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_mega_call_count++;

	game_fn_mega_f1 = f1;
	game_fn_mega_i1 = i1;
	game_fn_mega_b1 = b1;
	game_fn_mega_f2 = f2;
	game_fn_mega_f3 = f3;
	game_fn_mega_f4 = f4;
	game_fn_mega_b2 = b2;
	game_fn_mega_i2 = i2;
	game_fn_mega_f5 = f5;
	game_fn_mega_f6 = f6;
	game_fn_mega_f7 = f7;
	game_fn_mega_f8 = f8;
	game_fn_mega_id = id;
	game_fn_mega_str = str;
}
int game_fn_get_evil_false(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_evil_false_call_count++;

	return 0xff00;
}
static bool game_fn_set_is_happy_is_happy;
void game_fn_set_is_happy(bool is_happy) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_is_happy_call_count++;

	game_fn_set_is_happy_is_happy = is_happy;
}
static float game_fn_mega_f32_f1;
static float game_fn_mega_f32_f2;
static float game_fn_mega_f32_f3;
static float game_fn_mega_f32_f4;
static float game_fn_mega_f32_f5;
static float game_fn_mega_f32_f6;
static float game_fn_mega_f32_f7;
static float game_fn_mega_f32_f8;
static float game_fn_mega_f32_f9;
void game_fn_mega_f32(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_mega_f32_call_count++;

	game_fn_mega_f32_f1 = f1;
	game_fn_mega_f32_f2 = f2;
	game_fn_mega_f32_f3 = f3;
	game_fn_mega_f32_f4 = f4;
	game_fn_mega_f32_f5 = f5;
	game_fn_mega_f32_f6 = f6;
	game_fn_mega_f32_f7 = f7;
	game_fn_mega_f32_f8 = f8;
	game_fn_mega_f32_f9 = f9;
}
static int32_t game_fn_mega_i32_i1;
static int32_t game_fn_mega_i32_i2;
static int32_t game_fn_mega_i32_i3;
static int32_t game_fn_mega_i32_i4;
static int32_t game_fn_mega_i32_i5;
static int32_t game_fn_mega_i32_i6;
static int32_t game_fn_mega_i32_i7;
void game_fn_mega_i32(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_mega_i32_call_count++;

	game_fn_mega_i32_i1 = i1;
	game_fn_mega_i32_i2 = i2;
	game_fn_mega_i32_i3 = i3;
	game_fn_mega_i32_i4 = i4;
	game_fn_mega_i32_i5 = i5;
	game_fn_mega_i32_i6 = i6;
	game_fn_mega_i32_i7 = i7;
}
static const char *game_fn_draw_sprite_path;
void game_fn_draw(const char *sprite_path) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_draw_call_count++;
	game_fn_draw_sprite_path = sprite_path;
}
void game_fn_blocked_alrm(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_blocked_alrm_call_count++;
}
static const char *game_fn_spawn_name;
void game_fn_spawn(const char *name) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_spawn_call_count++;

	game_fn_spawn_name = name;
}
static const char *game_fn_has_resource_path;
bool game_fn_has_resource(const char *path) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_has_resource_call_count++;

	game_fn_has_resource_path = path;

	return true;
}
static const char *game_fn_has_entity_name;
bool game_fn_has_entity(const char *name) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_has_entity_call_count++;

	game_fn_has_entity_name = name;

	return true;
}
static const char *game_fn_has_string_str;
bool game_fn_has_string(const char *str) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_has_string_call_count++;

	game_fn_has_string_str = str;

	return true;
}
uint64_t game_fn_get_opponent(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_opponent_call_count++;

	return 69;
}
static uint64_t game_fn_set_d_target;
void game_fn_set_d(uint64_t target) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_d_call_count++;

	game_fn_set_d_target = target;
}
static uint64_t game_fn_set_opponent_target;
void game_fn_set_opponent(uint64_t target) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_opponent_call_count++;

	game_fn_set_opponent_target = target;
}
static int32_t game_fn_motherload_i1;
static int32_t game_fn_motherload_i2;
static int32_t game_fn_motherload_i3;
static int32_t game_fn_motherload_i4;
static int32_t game_fn_motherload_i5;
static int32_t game_fn_motherload_i6;
static int32_t game_fn_motherload_i7;
static float game_fn_motherload_f1;
static float game_fn_motherload_f2;
static float game_fn_motherload_f3;
static float game_fn_motherload_f4;
static float game_fn_motherload_f5;
static float game_fn_motherload_f6;
static float game_fn_motherload_f7;
static float game_fn_motherload_f8;
static uint64_t game_fn_motherload_id;
static float game_fn_motherload_f9;
void game_fn_motherload(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, uint64_t id, float f9) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_motherload_call_count++;

	game_fn_motherload_i1 = i1;
	game_fn_motherload_i2 = i2;
	game_fn_motherload_i3 = i3;
	game_fn_motherload_i4 = i4;
	game_fn_motherload_i5 = i5;
	game_fn_motherload_i6 = i6;
	game_fn_motherload_i7 = i7;
	game_fn_motherload_f1 = f1;
	game_fn_motherload_f2 = f2;
	game_fn_motherload_f3 = f3;
	game_fn_motherload_f4 = f4;
	game_fn_motherload_f5 = f5;
	game_fn_motherload_f6 = f6;
	game_fn_motherload_f7 = f7;
	game_fn_motherload_f8 = f8;
	game_fn_motherload_id = id;
	game_fn_motherload_f9 = f9;
}
static int32_t game_fn_motherload_subless_i1;
static int32_t game_fn_motherload_subless_i2;
static int32_t game_fn_motherload_subless_i3;
static int32_t game_fn_motherload_subless_i4;
static int32_t game_fn_motherload_subless_i5;
static int32_t game_fn_motherload_subless_i6;
static int32_t game_fn_motherload_subless_i7;
static float game_fn_motherload_subless_f1;
static float game_fn_motherload_subless_f2;
static float game_fn_motherload_subless_f3;
static float game_fn_motherload_subless_f4;
static float game_fn_motherload_subless_f5;
static float game_fn_motherload_subless_f6;
static float game_fn_motherload_subless_f7;
static float game_fn_motherload_subless_f8;
static float game_fn_motherload_subless_f9;
static uint64_t game_fn_motherload_subless_id;
static float game_fn_motherload_subless_f10;
void game_fn_motherload_subless(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, uint64_t id, float f10) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_motherload_subless_call_count++;

	game_fn_motherload_subless_i1 = i1;
	game_fn_motherload_subless_i2 = i2;
	game_fn_motherload_subless_i3 = i3;
	game_fn_motherload_subless_i4 = i4;
	game_fn_motherload_subless_i5 = i5;
	game_fn_motherload_subless_i6 = i6;
	game_fn_motherload_subless_i7 = i7;
	game_fn_motherload_subless_f1 = f1;
	game_fn_motherload_subless_f2 = f2;
	game_fn_motherload_subless_f3 = f3;
	game_fn_motherload_subless_f4 = f4;
	game_fn_motherload_subless_f5 = f5;
	game_fn_motherload_subless_f6 = f6;
	game_fn_motherload_subless_f7 = f7;
	game_fn_motherload_subless_f8 = f8;
	game_fn_motherload_subless_f9 = f9;
	game_fn_motherload_subless_id = id;
	game_fn_motherload_subless_f10 = f10;
}
static const char *game_fn_offset_32_bit_f32_s1;
static const char *game_fn_offset_32_bit_f32_s2;
static const char *game_fn_offset_32_bit_f32_s3;
static const char *game_fn_offset_32_bit_f32_s4;
static const char *game_fn_offset_32_bit_f32_s5;
static const char *game_fn_offset_32_bit_f32_s6;
static const char *game_fn_offset_32_bit_f32_s7;
static const char *game_fn_offset_32_bit_f32_s8;
static const char *game_fn_offset_32_bit_f32_s9;
static const char *game_fn_offset_32_bit_f32_s10;
static const char *game_fn_offset_32_bit_f32_s11;
static const char *game_fn_offset_32_bit_f32_s12;
static const char *game_fn_offset_32_bit_f32_s13;
static const char *game_fn_offset_32_bit_f32_s14;
static const char *game_fn_offset_32_bit_f32_s15;
static float game_fn_offset_32_bit_f32_f1;
static float game_fn_offset_32_bit_f32_f2;
static float game_fn_offset_32_bit_f32_f3;
static float game_fn_offset_32_bit_f32_f4;
static float game_fn_offset_32_bit_f32_f5;
static float game_fn_offset_32_bit_f32_f6;
static float game_fn_offset_32_bit_f32_f7;
static float game_fn_offset_32_bit_f32_f8;
static int32_t game_fn_offset_32_bit_f32_g;
void game_fn_offset_32_bit_f32(const char *s1, const char *s2, const char *s3, const char *s4, const char *s5, const char *s6, const char *s7, const char *s8, const char *s9, const char *s10, const char *s11, const char *s12, const char *s13, const char *s14, const char *s15, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, int32_t g) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_offset_32_bit_f32_call_count++;

	game_fn_offset_32_bit_f32_s1 = s1;
	game_fn_offset_32_bit_f32_s2 = s2;
	game_fn_offset_32_bit_f32_s3 = s3;
	game_fn_offset_32_bit_f32_s4 = s4;
	game_fn_offset_32_bit_f32_s5 = s5;
	game_fn_offset_32_bit_f32_s6 = s6;
	game_fn_offset_32_bit_f32_s7 = s7;
	game_fn_offset_32_bit_f32_s8 = s8;
	game_fn_offset_32_bit_f32_s9 = s9;
	game_fn_offset_32_bit_f32_s10 = s10;
	game_fn_offset_32_bit_f32_s11 = s11;
	game_fn_offset_32_bit_f32_s12 = s12;
	game_fn_offset_32_bit_f32_s13 = s13;
	game_fn_offset_32_bit_f32_s14 = s14;
	game_fn_offset_32_bit_f32_s15 = s15;
	game_fn_offset_32_bit_f32_f1 = f1;
	game_fn_offset_32_bit_f32_f2 = f2;
	game_fn_offset_32_bit_f32_f3 = f3;
	game_fn_offset_32_bit_f32_f4 = f4;
	game_fn_offset_32_bit_f32_f5 = f5;
	game_fn_offset_32_bit_f32_f6 = f6;
	game_fn_offset_32_bit_f32_f7 = f7;
	game_fn_offset_32_bit_f32_f8 = f8;
	game_fn_offset_32_bit_f32_g = g;
}
static float game_fn_offset_32_bit_i32_f1;
static float game_fn_offset_32_bit_i32_f2;
static float game_fn_offset_32_bit_i32_f3;
static float game_fn_offset_32_bit_i32_f4;
static float game_fn_offset_32_bit_i32_f5;
static float game_fn_offset_32_bit_i32_f6;
static float game_fn_offset_32_bit_i32_f7;
static float game_fn_offset_32_bit_i32_f8;
static float game_fn_offset_32_bit_i32_f9;
static float game_fn_offset_32_bit_i32_f10;
static float game_fn_offset_32_bit_i32_f11;
static float game_fn_offset_32_bit_i32_f12;
static float game_fn_offset_32_bit_i32_f13;
static float game_fn_offset_32_bit_i32_f14;
static float game_fn_offset_32_bit_i32_f15;
static float game_fn_offset_32_bit_i32_f16;
static float game_fn_offset_32_bit_i32_f17;
static float game_fn_offset_32_bit_i32_f18;
static float game_fn_offset_32_bit_i32_f19;
static float game_fn_offset_32_bit_i32_f20;
static float game_fn_offset_32_bit_i32_f21;
static float game_fn_offset_32_bit_i32_f22;
static float game_fn_offset_32_bit_i32_f23;
static float game_fn_offset_32_bit_i32_f24;
static float game_fn_offset_32_bit_i32_f25;
static float game_fn_offset_32_bit_i32_f26;
static float game_fn_offset_32_bit_i32_f27;
static float game_fn_offset_32_bit_i32_f28;
static float game_fn_offset_32_bit_i32_f29;
static float game_fn_offset_32_bit_i32_f30;
static int32_t game_fn_offset_32_bit_i32_i1;
static int32_t game_fn_offset_32_bit_i32_i2;
static int32_t game_fn_offset_32_bit_i32_i3;
static int32_t game_fn_offset_32_bit_i32_i4;
static int32_t game_fn_offset_32_bit_i32_i5;
static int32_t game_fn_offset_32_bit_i32_g;
void game_fn_offset_32_bit_i32(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19, float f20, float f21, float f22, float f23, float f24, float f25, float f26, float f27, float f28, float f29, float f30, int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t g) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_offset_32_bit_i32_call_count++;

	game_fn_offset_32_bit_i32_f1 = f1;
	game_fn_offset_32_bit_i32_f2 = f2;
	game_fn_offset_32_bit_i32_f3 = f3;
	game_fn_offset_32_bit_i32_f4 = f4;
	game_fn_offset_32_bit_i32_f5 = f5;
	game_fn_offset_32_bit_i32_f6 = f6;
	game_fn_offset_32_bit_i32_f7 = f7;
	game_fn_offset_32_bit_i32_f8 = f8;
	game_fn_offset_32_bit_i32_f9 = f9;
	game_fn_offset_32_bit_i32_f10 = f10;
	game_fn_offset_32_bit_i32_f11 = f11;
	game_fn_offset_32_bit_i32_f12 = f12;
	game_fn_offset_32_bit_i32_f13 = f13;
	game_fn_offset_32_bit_i32_f14 = f14;
	game_fn_offset_32_bit_i32_f15 = f15;
	game_fn_offset_32_bit_i32_f16 = f16;
	game_fn_offset_32_bit_i32_f17 = f17;
	game_fn_offset_32_bit_i32_f18 = f18;
	game_fn_offset_32_bit_i32_f19 = f19;
	game_fn_offset_32_bit_i32_f20 = f20;
	game_fn_offset_32_bit_i32_f21 = f21;
	game_fn_offset_32_bit_i32_f22 = f22;
	game_fn_offset_32_bit_i32_f23 = f23;
	game_fn_offset_32_bit_i32_f24 = f24;
	game_fn_offset_32_bit_i32_f25 = f25;
	game_fn_offset_32_bit_i32_f26 = f26;
	game_fn_offset_32_bit_i32_f27 = f27;
	game_fn_offset_32_bit_i32_f28 = f28;
	game_fn_offset_32_bit_i32_f29 = f29;
	game_fn_offset_32_bit_i32_f30 = f30;
	game_fn_offset_32_bit_i32_i1 = i1;
	game_fn_offset_32_bit_i32_i2 = i2;
	game_fn_offset_32_bit_i32_i3 = i3;
	game_fn_offset_32_bit_i32_i4 = i4;
	game_fn_offset_32_bit_i32_i5 = i5;
	game_fn_offset_32_bit_i32_g = g;
}
static float game_fn_offset_32_bit_string_f1;
static float game_fn_offset_32_bit_string_f2;
static float game_fn_offset_32_bit_string_f3;
static float game_fn_offset_32_bit_string_f4;
static float game_fn_offset_32_bit_string_f5;
static float game_fn_offset_32_bit_string_f6;
static float game_fn_offset_32_bit_string_f7;
static float game_fn_offset_32_bit_string_f8;
static float game_fn_offset_32_bit_string_f9;
static float game_fn_offset_32_bit_string_f10;
static float game_fn_offset_32_bit_string_f11;
static float game_fn_offset_32_bit_string_f12;
static float game_fn_offset_32_bit_string_f13;
static float game_fn_offset_32_bit_string_f14;
static float game_fn_offset_32_bit_string_f15;
static float game_fn_offset_32_bit_string_f16;
static float game_fn_offset_32_bit_string_f17;
static float game_fn_offset_32_bit_string_f18;
static float game_fn_offset_32_bit_string_f19;
static float game_fn_offset_32_bit_string_f20;
static float game_fn_offset_32_bit_string_f21;
static float game_fn_offset_32_bit_string_f22;
static float game_fn_offset_32_bit_string_f23;
static float game_fn_offset_32_bit_string_f24;
static float game_fn_offset_32_bit_string_f25;
static float game_fn_offset_32_bit_string_f26;
static float game_fn_offset_32_bit_string_f27;
static float game_fn_offset_32_bit_string_f28;
static float game_fn_offset_32_bit_string_f29;
static float game_fn_offset_32_bit_string_f30;
static const char *game_fn_offset_32_bit_string_s1;
static const char *game_fn_offset_32_bit_string_s2;
static const char *game_fn_offset_32_bit_string_s3;
static const char *game_fn_offset_32_bit_string_s4;
static const char *game_fn_offset_32_bit_string_s5;
static int32_t game_fn_offset_32_bit_string_g;
void game_fn_offset_32_bit_string(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19, float f20, float f21, float f22, float f23, float f24, float f25, float f26, float f27, float f28, float f29, float f30, const char *s1, const char *s2, const char *s3, const char *s4, const char *s5, int32_t g) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_offset_32_bit_string_call_count++;

	game_fn_offset_32_bit_string_f1 = f1;
	game_fn_offset_32_bit_string_f2 = f2;
	game_fn_offset_32_bit_string_f3 = f3;
	game_fn_offset_32_bit_string_f4 = f4;
	game_fn_offset_32_bit_string_f5 = f5;
	game_fn_offset_32_bit_string_f6 = f6;
	game_fn_offset_32_bit_string_f7 = f7;
	game_fn_offset_32_bit_string_f8 = f8;
	game_fn_offset_32_bit_string_f9 = f9;
	game_fn_offset_32_bit_string_f10 = f10;
	game_fn_offset_32_bit_string_f11 = f11;
	game_fn_offset_32_bit_string_f12 = f12;
	game_fn_offset_32_bit_string_f13 = f13;
	game_fn_offset_32_bit_string_f14 = f14;
	game_fn_offset_32_bit_string_f15 = f15;
	game_fn_offset_32_bit_string_f16 = f16;
	game_fn_offset_32_bit_string_f17 = f17;
	game_fn_offset_32_bit_string_f18 = f18;
	game_fn_offset_32_bit_string_f19 = f19;
	game_fn_offset_32_bit_string_f20 = f20;
	game_fn_offset_32_bit_string_f21 = f21;
	game_fn_offset_32_bit_string_f22 = f22;
	game_fn_offset_32_bit_string_f23 = f23;
	game_fn_offset_32_bit_string_f24 = f24;
	game_fn_offset_32_bit_string_f25 = f25;
	game_fn_offset_32_bit_string_f26 = f26;
	game_fn_offset_32_bit_string_f27 = f27;
	game_fn_offset_32_bit_string_f28 = f28;
	game_fn_offset_32_bit_string_f29 = f29;
	game_fn_offset_32_bit_string_f30 = f30;
	game_fn_offset_32_bit_string_s1 = s1;
	game_fn_offset_32_bit_string_s2 = s2;
	game_fn_offset_32_bit_string_s3 = s3;
	game_fn_offset_32_bit_string_s4 = s4;
	game_fn_offset_32_bit_string_s5 = s5;
	game_fn_offset_32_bit_string_g = g;
}
static const char *game_fn_talk_message1;
static const char *game_fn_talk_message2;
static const char *game_fn_talk_message3;
static const char *game_fn_talk_message4;
void game_fn_talk(const char *message1, const char *message2, const char *message3, const char *message4) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_talk_call_count++;

	game_fn_talk_message1 = message1;
	game_fn_talk_message2 = message2;
	game_fn_talk_message3 = message3;
	game_fn_talk_message4 = message4;
}
uint64_t game_fn_get_position(uint64_t id) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_get_position_call_count++;

	(void)id;

	return 1337;
}
static uint64_t game_fn_set_position_pos;
void game_fn_set_position(uint64_t pos) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_set_position_call_count++;

	game_fn_set_position_pos = pos;
}
void game_fn_cause_game_fn_error(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_cause_game_fn_error_call_count++;

	game_fn_error("cause_game_fn_error(): Example game function error");
}
static char *saved_grug_path = NULL;
void game_fn_call_on_b_fn(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_call_on_b_fn_call_count++;

	on_fn_dispatcher("on_b", saved_grug_path);
}
static uint64_t game_fn_store_id;
void game_fn_store(uint64_t id) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_store_call_count++;

	game_fn_store_id = id;
}
uint64_t game_fn_retrieve(void) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_retrieve_call_count++;

	return 123;
}
uint64_t game_fn_box_i32(int32_t n) {
	ASSERT_16_BYTE_STACK_ALIGNED();
	game_fn_box_i32_call_count++;

	return n;
}

static void reset_call_counts(void) {
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
	game_fn_get_evil_false_call_count = 0;
	game_fn_set_is_happy_call_count = 0;
	game_fn_mega_f32_call_count = 0;
	game_fn_mega_i32_call_count = 0;
	game_fn_draw_call_count = 0;
	game_fn_blocked_alrm_call_count = 0;
	game_fn_spawn_call_count = 0;
	game_fn_has_resource_call_count = 0;
	game_fn_has_entity_call_count = 0;
	game_fn_has_string_call_count = 0;
	game_fn_get_opponent_call_count = 0;
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
	game_fn_retrieve_call_count = 0;
	game_fn_box_i32_call_count = 0;
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

#define ADD_TEST_ERROR(test_name, entity_type) {\
	if (is_whitelisted_test(#test_name)) {\
		error_test_datas[err_test_datas_size++] = (struct error_test_data){\
			.test_name_str = #test_name,\
			.grug_path = "err/"#test_name"/input-"entity_type".grug",\
			.expected_error_path = "err/"#test_name"/expected_error.txt",\
			.results_path = "err/"#test_name"/results",\
			.grug_output_path = "err/"#test_name"/results/grug_output.txt",\
			.failed_file_path = "err/"#test_name"/results/failed"\
		};\
	}\
}

#define ADD_TEST_OK(test_name, entity_type, expected_globals_size) {\
	if (is_whitelisted_test(#test_name)) {\
		ok_test_datas[ok_test_datas_size++] = (struct ok_test_data){\
			.run = ok_##test_name,\
			.test_name_str = #test_name,\
			.grug_path = "ok/"#test_name"/input-"entity_type".grug",\
			.results_path = "ok/"#test_name"/results",\
			.dump_path = "ok/"#test_name"/results/dump.json",\
			.applied_path = "ok/"#test_name"/results/applied.grug",\
			.failed_file_path = "ok/"#test_name"/results/failed",\
			.expected_globals_size_value = expected_globals_size\
		};\
	}\
}

#define ADD_TEST_RUNTIME_ERROR(test_name, entity_type, expected_globals_size) {\
	if (is_whitelisted_test(#test_name)) {\
		runtime_error_test_datas[err_runtime_test_datas_size++] = (struct runtime_error_test_data){\
			.run = runtime_error_##test_name,\
			.test_name_str = #test_name,\
			.grug_path = "err_runtime/"#test_name"/input-"entity_type".grug",\
			.expected_error_path = "err_runtime/"#test_name"/expected_error.txt",\
			.results_path = "err_runtime/"#test_name"/results",\
			.dump_path = "err_runtime/"#test_name"/results/dump.json",\
			.applied_path = "err_runtime/"#test_name"/results/applied.grug",\
			.failed_file_path = "err_runtime/"#test_name"/results/failed",\
			.expected_globals_size_value = expected_globals_size\
		};\
	}\
}

// This is the Fisher-Yates shuffle:
// https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
// https://blog.codinghorror.com/the-danger-of-naivete/
#ifdef SHUFFLES
#define SHUFFLE(arr, size, T) {\
	for (int i = size; i > 0; i--) {\
		int n = rand() % i;\
		\
		T old = arr[i-1];\
		arr[i-1] = arr[n];\
		arr[n] = old;\
	}\
}
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
	check(ftell_result, "ftell", NULL);
	size_t len = ftell_result;

	check(fseek(f, 0, SEEK_SET), "fseek", NULL);

	if (fread(bytes, len, 1, f) < len && ferror(f)) {
		fprintf(stderr, "fread error\n");
		exit(EXIT_FAILURE);
	}

	if (fclose(f) == EOF) {
		perror("fclose");
		exit(EXIT_FAILURE);
	}

	return len;
}

static bool newer(const char *path1, const char *path2) {
	struct stat s1;
	if (stat(path1, &s1) == -1) {
		if (errno != ENOENT) {
			fprintf(stderr, "path1: \"%s\"\n", path1);
			perror("stat");
			exit(EXIT_FAILURE);
		}
		return false;
	}

	struct stat s2;
	if (stat(path2, &s2) < 0) {
		fprintf(stderr, "path2: \"%s\"\n", path2);
		perror("stat");
		exit(EXIT_FAILURE);
	}

	return s1.st_mtime >= s2.st_mtime;
}

static const char *get_expected_error(const char *expected_error_path) {
	static char expected_error[420420];
	size_t expected_error_len = read_file(expected_error_path, (uint8_t *)expected_error);

	if (expected_error[expected_error_len - 1] == '\n') {
		expected_error_len--;
		if (expected_error[expected_error_len - 1] == '\r') {
			expected_error_len--;
		}
	}

	expected_error[expected_error_len] = '\0';

	return expected_error;
}

static void create_failed_file(const char *failed_file_path) {
	int fd = open(prefix(failed_file_path), O_RDWR | O_CREAT | O_TRUNC, 0644);
	check(fd, "open", prefix(failed_file_path));
	close(fd);
}

static bool failed_file_doesnt_exist(const char *failed_file_path) {
	errno = 0;
	return access(failed_file_path, F_OK) == -1 && errno == ENOENT;
}

static void make_results_dir(const char *results_path) {
	if (mkdir(prefix(results_path), 0755) == -1 && errno != EEXIST) {
		perror("mkdir");
		fprintf(stderr, "prefix(results_path): \"%s\"\n", prefix(results_path));\
		exit(EXIT_FAILURE);
	}
}

static int remove_callback(const char *entry_path, const struct stat *entry_info, int entry_type, struct FTW *ftw) {
	(void)entry_info;
	(void)entry_type;
	(void)ftw;

	int rv = remove(entry_path);
	check(rv, "remove", entry_path);

	return rv;
}

static int rm_rf(const char *path) {
	return nftw(path, remove_callback, 42, FTW_DEPTH | FTW_PHYS);
}

static bool shuffles_was_not_defined(void) {
#ifdef SHUFFLES
	return false;
#endif
	return true;
}

static void test_error(
	const char *test_name,
	const char *grug_path,
	const char *expected_error_path,
	const char *results_path,
	const char *grug_output_path,
	const char *failed_file_path
) {
	if (whitelisted_test == NULL
	 && failed_file_doesnt_exist(failed_file_path)
	 && shuffles_was_not_defined()
	 && newer(grug_output_path, grug_path)
	 && newer(grug_output_path, expected_error_path)
	 && newer(grug_output_path, "mod_api.json")
	 && newer(grug_output_path, "tests.sh")
	 && newer(grug_output_path, "smoketest")
     && newer(grug_output_path, "smoketest.c")
	) {
		fprintf(stderr, "Skipping tests/err/%s...\n", test_name);
		return;
	}

	fprintf(stderr, "Running tests/err/%s...\n", test_name);

	rm_rf(results_path);
	make_results_dir(results_path);

	create_failed_file(failed_file_path);

	const char *msg = compile_grug_file(prefix(grug_path), "err");

	const char *expected_error = get_expected_error(expected_error_path);

	if (!msg) {
		fprintf(stderr, "\nCompilation succeeded, but expected this error message:\n");
		fprintf(stderr, "%s\n", expected_error);
		exit(EXIT_FAILURE);
	}

	FILE *f = fopen(prefix(grug_output_path), "w");

	size_t msg_len = strlen(msg);

	if (fwrite(msg, msg_len, 1, f) == 0) {
		fprintf(stderr, "fwrite error\n");
		exit(EXIT_FAILURE);
	}

	if (fclose(f) == EOF) {
		perror("fclose");
		exit(EXIT_FAILURE);
	}

	if (!streq(msg, expected_error)) {
		fprintf(stderr, "\nThe output differs from the expected output.\n");
		fprintf(stderr, "Output:\n");
		fprintf(stderr, "%s\n", msg);

		fprintf(stderr, "Expected:\n");
		fprintf(stderr, "%s\n", expected_error);

		exit(EXIT_FAILURE);
	}

	unlink(failed_file_path);
}

static void diff_roundtrip(
	const char *grug_path,
	const char *dump_path,
	const char *applied_path
) {
	static char buf[4096];
	if (dump_file_to_json(prefix(grug_path), prefix_buf(dump_path, buf))) {
		fprintf(stderr, "Failed to dump file AST\n");
		exit(EXIT_FAILURE);
	}

	if (generate_file_from_json(prefix(dump_path), prefix_buf(applied_path, buf))) {
		fprintf(stderr, "Failed to apply file AST\n");
		exit(EXIT_FAILURE);
	}

	static uint8_t grug_path_bytes[420420];
	size_t grug_path_bytes_len = read_file(grug_path, grug_path_bytes);
	grug_path_bytes[grug_path_bytes_len] = '\0';

	static uint8_t applied_path_bytes[420420];
	size_t applied_path_bytes_len = read_file(applied_path, applied_path_bytes);
	applied_path_bytes[applied_path_bytes_len] = '\0';

	if (!streq((const char *)grug_path_bytes, (const char *)applied_path_bytes)) {
		fprintf(stderr, "\nThe output differs from the expected output.\n");
		fprintf(stderr, "Output:\n");
		fprintf(stderr, "%s\n", applied_path_bytes);

		fprintf(stderr, "Expected:\n");
		fprintf(stderr, "%s\n", grug_path_bytes);

		exit(EXIT_FAILURE);
	}
}

static void prologue(
	const char *grug_path,
	const char *results_path,
	const char *failed_file_path,
	const char *mod_name
) {
	reset_call_counts();

	runtime_error_reason = NULL;
	had_runtime_error = false;
	error_handler_calls = 0;
	runtime_error_type = -1;
	runtime_error_on_fn_name = NULL;
	runtime_error_on_fn_path = NULL;

	rm_rf(results_path);
	make_results_dir(results_path);

	create_failed_file(failed_file_path);

	const char *msg = compile_grug_file(prefix(grug_path), mod_name);
	if (msg) {
		fprintf(stderr, "The test wasn't supposed to print anything, but did:\n");
		fprintf(stderr, "----\n");
		fprintf(stderr, "%s\n", msg);
		fprintf(stderr, "----\n");

		exit(EXIT_FAILURE);
	}

	init_globals_fn_dispatcher(grug_path);
}

static void ok_addition_as_argument(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/addition_as_argument/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(!had_runtime_error);

	assert(game_fn_initialize_x == 3);
}

static void ok_addition_as_two_arguments(void) {
	assert(game_fn_max_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/addition_as_two_arguments/input-D.grug");
	assert(game_fn_max_call_count == 1);

	assert(game_fn_max_x == 3);
	assert(game_fn_max_y == 9);
}

static void ok_addition_with_multiplication(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/addition_with_multiplication/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 14);
}

static void ok_addition_with_multiplication_2(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/addition_with_multiplication_2/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 10);
}

static void ok_and_false_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/and_false_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_and_false_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/and_false_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_and_false_3(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/and_false_3/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_and_short_circuit(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/and_short_circuit/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_and_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/and_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_blocked_alrm(void) {
	assert(game_fn_blocked_alrm_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/blocked_alrm/input-D.grug");
	assert(game_fn_blocked_alrm_call_count == 1);
}

static void ok_bool_logical_not_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/bool_logical_not_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_bool_logical_not_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/bool_logical_not_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_bool_returned(void) {
	assert(game_fn_set_is_happy_call_count == 0);
	assert(game_fn_get_evil_false_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/bool_returned/input-D.grug");
	assert(game_fn_set_is_happy_call_count == 1);
	assert(game_fn_get_evil_false_call_count == 1);

	assert(game_fn_set_is_happy_is_happy == false);
}

static void ok_bool_returned_global(void) {
	assert(game_fn_set_is_happy_call_count == 0);
	assert(game_fn_get_evil_false_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/bool_returned_global/input-D.grug");
	assert(game_fn_set_is_happy_call_count == 1);
	assert(game_fn_get_evil_false_call_count == 1);

	assert(game_fn_set_is_happy_is_happy == false);
}

static void ok_bool_zero_extended_if_statement(void) {
	assert(game_fn_nothing_call_count == 0);
	assert(game_fn_get_evil_false_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/bool_zero_extended_if_statement/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
	assert(game_fn_get_evil_false_call_count == 1);
}

static void ok_bool_zero_extended_while_statement(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/bool_zero_extended_while_statement/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_break(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/break/input-D.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_calls_100(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/calls_100/input-D.grug");
	assert(game_fn_nothing_call_count == 100);
}

static void ok_calls_1000(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/calls_1000/input-D.grug");
	assert(game_fn_nothing_call_count == 1000);
}

static void ok_calls_in_call(void) {
	assert(game_fn_max_call_count == 0);
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/calls_in_call/input-D.grug");
	assert(game_fn_max_call_count == 3);
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_max_x == 2);
	assert(game_fn_max_y == 4);
	assert(game_fn_initialize_x == 4);
}

static void ok_comment_above_block(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/comment_above_block/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_comment_above_block_twice(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/comment_above_block_twice/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_comment_above_globals(void) {
}

static void ok_comment_above_helper_fn(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/comment_above_helper_fn/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_comment_above_on_fn(void) {
    on_fn_dispatcher("on_a", "tests/ok/comment_above_on_fn/input-D.grug");
}

static void ok_comment_between_globals(void) {
}

static void ok_comment_between_statements(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/comment_between_statements/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_comment_lone_block(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/comment_lone_block/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_comment_lone_block_at_end(void) {
    on_fn_dispatcher("on_a", "tests/ok/comment_lone_block_at_end/input-D.grug");
}

static void ok_comment_lone_global(void) {
    on_fn_dispatcher("on_a", "tests/ok/comment_lone_global/input-D.grug");
}

static void ok_continue(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/continue/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_custom_id_decays_to_id(void) {
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/custom_id_decays_to_id/input-D.grug");
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 42);
}

static void ok_custom_id_transfer_between_globals(void) {
	assert(game_fn_get_opponent_call_count == 1); // Called by init_globals()
	assert(game_fn_set_opponent_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/custom_id_transfer_between_globals/input-D.grug");
	assert(game_fn_get_opponent_call_count == 1);
	assert(game_fn_set_opponent_call_count == 1);

	assert(game_fn_set_opponent_target == 69);
}

static void ok_custom_id_with_digits(void) {
	assert(game_fn_box_i32_call_count == 1);
}

static void ok_division_negative_result(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/division_negative_result/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -2);
}

static void ok_division_positive_result(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/division_positive_result/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 2);
}

static void ok_double_negation_with_parentheses(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/double_negation_with_parentheses/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 2);
}

static void ok_double_not_with_parentheses(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/double_not_with_parentheses/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_else_after_else_if_false(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/else_after_else_if_false/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_else_after_else_if_true(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/else_after_else_if_true/input-D.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_else_false(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/else_false/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_else_if_false(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/else_if_false/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_else_if_true(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/else_if_true/input-D.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_else_true(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/else_true/input-D.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_empty_file(void) {
}

static void ok_empty_line(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/empty_line/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_entity_and_resource_as_subexpression(void) {
	assert(game_fn_has_resource_call_count == 0);
	assert(game_fn_has_entity_call_count == 0);
	assert(game_fn_has_string_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/entity_and_resource_as_subexpression/input-D.grug");
	assert(game_fn_has_resource_call_count == 1);
	assert(game_fn_has_entity_call_count == 1);
	assert(game_fn_has_string_call_count == 1);

	assert(streq(game_fn_has_resource_path, "tests/ok/entity_and_resource_as_subexpression/foo.txt"));
	assert(streq(game_fn_has_entity_name, "ok:baz"));
	assert(streq(game_fn_has_string_str, "bar"));
}

static void ok_entity_duplicate(void) {
	assert(game_fn_spawn_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/entity_duplicate/input-D.grug");
	assert(game_fn_spawn_call_count == 4);

	assert(streq(game_fn_spawn_name, "ok:baz"));
}

static void ok_entity_in_on_fn(void) {
	assert(game_fn_spawn_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/entity_in_on_fn/input-D.grug");
	assert(game_fn_spawn_call_count == 1);

	assert(streq(game_fn_spawn_name, "ok:foo"));
}

static void ok_entity_in_on_fn_with_mod_specified(void) {
	assert(game_fn_spawn_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/entity_in_on_fn_with_mod_specified/input-D.grug");
	assert(game_fn_spawn_call_count == 1);

	assert(streq(game_fn_spawn_name, "wow:foo"));
}

static void ok_eq_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/eq_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_eq_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/eq_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_addition(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_addition/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 6.0f);
}

static void ok_f32_argument(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_argument/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 4.0f);
}

static void ok_f32_division(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_division/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 0.5f);
}

static void ok_f32_eq_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_eq_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_f32_eq_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_eq_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_ge_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_ge_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_f32_ge_true_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_ge_true_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_ge_true_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_ge_true_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_global_variable(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_global_variable/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 4.0f);
}

static void ok_f32_gt_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_gt_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_f32_gt_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_gt_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_le_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_le_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_f32_le_true_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_le_true_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_le_true_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_le_true_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_local_variable(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_local_variable/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 4.0f);
}

static void ok_f32_lt_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_lt_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_f32_lt_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_lt_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_multiplication(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_multiplication/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 8.0f);
}

static void ok_f32_ne_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_ne_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_f32_negated(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_negated/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == -4.0f);
}

static void ok_f32_ne_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_ne_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_f32_passed_to_helper_fn(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_passed_to_helper_fn/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 42.0f);
}

static void ok_f32_passed_to_on_fn(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_args_dispatcher("on_a", "tests/ok/f32_passed_to_on_fn/input-R.grug", (struct grug_value[]){{.type=grug_type_f32, .f32=42.0f}}, 1);
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == 42.0f);
}

static void ok_f32_passing_sin_to_cos(void) {
	assert(game_fn_sin_call_count == 0);
	assert(game_fn_cos_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_passing_sin_to_cos/input-D.grug");
	assert(game_fn_sin_call_count == 1);
	assert(game_fn_cos_call_count == 1);

	assert(game_fn_sin_x == 4.0f);
	assert(game_fn_cos_x == sinf(4.0f));
}

static void ok_f32_subtraction(void) {
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/f32_subtraction/input-D.grug");
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_sin_x == -2.0f);
}

static void ok_fibonacci(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/fibonacci/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 55);
}

static void ok_ge_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/ge_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_ge_true_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/ge_true_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_ge_true_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/ge_true_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_global_2_does_not_have_error_handling(void) {
}

static void ok_global_call_using_me(void) {
	assert(game_fn_get_position_call_count == 1);

	assert(game_fn_set_position_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/global_call_using_me/input-D.grug");
	assert(game_fn_set_position_call_count == 1);

	assert(game_fn_set_position_pos == 1337);
}

static void ok_global_can_use_earlier_global(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/global_can_use_earlier_global/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 5);
}

static void ok_global_containing_negation(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/global_containing_negation/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -2);
}

static void ok_global_id(void) {
	assert(game_fn_get_opponent_call_count == 1);

	assert(game_fn_set_position_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/global_id/input-D.grug");
	assert(game_fn_set_position_call_count == 1);

	assert(game_fn_set_position_pos == 69);
}

static void ok_globals(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/globals/input-D.grug");
	assert(game_fn_initialize_call_count == 2);

	assert(game_fn_initialize_x == 1337);
}

static void ok_globals_1000(void) {
}

static void ok_globals_1000_string(void) {
}

static void ok_globals_32(void) {
}

static void ok_globals_64(void) {
}

static void ok_gt_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/gt_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_gt_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/gt_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_helper_fn(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_helper_fn_called_in_if(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_called_in_if/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_helper_fn_called_indirectly(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_called_indirectly/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_helper_fn_overwriting_param(void) {
	assert(game_fn_initialize_call_count == 0);
	assert(game_fn_sin_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_overwriting_param/input-D.grug");
	assert(game_fn_initialize_call_count == 1);
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_initialize_x == 20);
	assert(game_fn_sin_x == 30.0f);
}

static void ok_helper_fn_returning_void_has_no_return(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_returning_void_has_no_return/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_helper_fn_returning_void_returns_void(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_returning_void_returns_void/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_helper_fn_same_param_name_as_on_fn(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_same_param_name_as_on_fn/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_helper_fn_same_param_name_as_other_helper_fn(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/helper_fn_same_param_name_as_other_helper_fn/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_i32_max(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/i32_max/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 2147483647);
}

static void ok_i32_min(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/i32_min/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -2147483648);
}

static void ok_i32_negated(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/i32_negated/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -42);
}

static void ok_i32_negative_is_smaller_than_positive(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/i32_negative_is_smaller_than_positive/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_id_binary_expr_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_binary_expr_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_id_binary_expr_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_binary_expr_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_id_eq_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
	assert(game_fn_retrieve_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_eq_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);
	assert(game_fn_retrieve_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_id_eq_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
	assert(game_fn_retrieve_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_eq_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);
	assert(game_fn_retrieve_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_id_global_with_id_to_new_id(void) {
	assert(game_fn_retrieve_call_count == 1); // Called by init_globals()
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_global_with_id_to_new_id/input-D.grug");
	assert(game_fn_retrieve_call_count == 1);
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 123);
}

static void ok_id_global_with_opponent_to_new_id(void) {
	assert(game_fn_get_opponent_call_count == 1); // Called by init_globals()
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_global_with_opponent_to_new_id/input-D.grug");
	assert(game_fn_get_opponent_call_count == 1);
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 69);
}

static void ok_id_helper_fn_param(void) {
	assert(game_fn_retrieve_call_count == 0);
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_helper_fn_param/input-D.grug");
	assert(game_fn_retrieve_call_count == 1);
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 123);
}

static void ok_id_local_variable_get_and_set(void) {
	assert(game_fn_get_opponent_call_count == 0);
	assert(game_fn_set_opponent_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_local_variable_get_and_set/input-D.grug");
	assert(game_fn_get_opponent_call_count == 1);
	assert(game_fn_set_opponent_call_count == 1);

	assert(game_fn_set_opponent_target == 69);
}

static void ok_id_ne_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
	assert(game_fn_retrieve_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_ne_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);
	assert(game_fn_retrieve_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_id_ne_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
	assert(game_fn_retrieve_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_ne_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);
	assert(game_fn_retrieve_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_id_on_fn_param(void) {
	assert(game_fn_store_call_count == 0);
    on_fn_args_dispatcher("on_a", "tests/ok/id_on_fn_param/input-U.grug", (struct grug_value[]){{.type=grug_type_id, .id=77}}, 1);
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 77);
}

static void ok_id_returned_from_helper(void) {
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_returned_from_helper/input-D.grug");
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 42);
}

static void ok_id_with_d_to_new_id_and_id_to_old_id(void) {
	assert(game_fn_retrieve_call_count == 0);
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_with_d_to_new_id_and_id_to_old_id/input-D.grug");
	assert(game_fn_retrieve_call_count == 1);
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 123);
}

static void ok_id_with_d_to_old_id(void) {
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_with_d_to_old_id/input-D.grug");
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 42);
}

static void ok_id_with_id_to_new_id(void) {
	assert(game_fn_retrieve_call_count == 0);
	assert(game_fn_store_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/id_with_id_to_new_id/input-D.grug");
	assert(game_fn_retrieve_call_count == 1);
	assert(game_fn_store_call_count == 1);

	assert(game_fn_store_id == 123);
}

static void ok_if_false(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/if_false/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_if_true(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/if_true/input-D.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_le_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/le_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_le_true_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/le_true_1/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_le_true_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/le_true_2/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_local_id_can_be_reassigned(void) {
	assert(game_fn_get_opponent_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/local_id_can_be_reassigned/input-D.grug");
	assert(game_fn_get_opponent_call_count == 2);
}

static void ok_lt_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/lt_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_lt_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/lt_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_max_args(void) {
	assert(game_fn_mega_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/max_args/input-D.grug");
	assert(game_fn_mega_call_count == 1);

	assert(game_fn_mega_f1 == 1.0f);
	assert(game_fn_mega_i1 == 21);
	assert(game_fn_mega_b1 == true);
	assert(game_fn_mega_f2 == 2.0f);
	assert(game_fn_mega_f3 == 3.0f);
	assert(game_fn_mega_f4 == 4.0f);
	assert(game_fn_mega_b2 == false);
	assert(game_fn_mega_i2 == 1337);
	assert(game_fn_mega_f5 == 5.0f);
	assert(game_fn_mega_f6 == 6.0f);
	assert(game_fn_mega_f7 == 7.0f);
	assert(game_fn_mega_f8 == 8.0f);
	assert(game_fn_mega_id == 42);
	assert(streq(game_fn_mega_str, "foo"));
}

static void ok_me(void) {
	assert(game_fn_set_d_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/me/input-D.grug");
	assert(game_fn_set_d_call_count == 1);

	assert(game_fn_set_d_target == 42);
}

static void ok_me_assigned_to_local_variable(void) {
	assert(game_fn_set_d_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/me_assigned_to_local_variable/input-D.grug");
	assert(game_fn_set_d_call_count == 1);

	assert(game_fn_set_d_target == 42);
}

static void ok_me_passed_to_helper_fn(void) {
	assert(game_fn_set_d_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/me_passed_to_helper_fn/input-D.grug");
	assert(game_fn_set_d_call_count == 1);

	assert(game_fn_set_d_target == 42);
}

static void ok_mov_32_bits_global_i32(void) {
}

static void ok_mov_32_bits_global_id(void) {
}

static void ok_multiplication_as_two_arguments(void) {
	assert(game_fn_max_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/multiplication_as_two_arguments/input-D.grug");
	assert(game_fn_max_call_count == 1);

	assert(game_fn_max_x == 6);
	assert(game_fn_max_y == 20);
}

static void ok_ne_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/ne_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_ne_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/ne_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_negate_parenthesized_expr(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/negate_parenthesized_expr/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -5);
}

static void ok_negative_literal(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/negative_literal/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -42);
}

static void ok_nested_break(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/nested_break/input-D.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_nested_continue(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/nested_continue/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_no_empty_line_between_globals(void) {
}

static void ok_no_empty_line_between_statements(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/no_empty_line_between_statements/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_on_fn(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_on_fn_calling_game_fn_nothing(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_game_fn_nothing/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_on_fn_calling_game_fn_nothing_twice(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_game_fn_nothing_twice/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_on_fn_calling_game_fn_plt_order(void) {
	assert(game_fn_nothing_call_count == 0);
	assert(game_fn_magic_call_count == 0);
	assert(game_fn_initialize_call_count == 0);
	assert(game_fn_identity_call_count == 0);
	assert(game_fn_max_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_game_fn_plt_order/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
	assert(game_fn_magic_call_count == 1);
	assert(game_fn_initialize_call_count == 1);
	assert(game_fn_identity_call_count == 1);
	assert(game_fn_max_call_count == 1);

	assert(game_fn_initialize_x == 42);
	assert(game_fn_identity_x == 69);
	assert(game_fn_max_x == 1337);
	assert(game_fn_max_y == 8192);
}

static void ok_on_fn_calling_helper_fns(void) {
	assert(game_fn_nothing_call_count == 0);
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_helper_fns/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 42);
}

static void ok_on_fn_calling_no_game_fn(void) {
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_no_game_fn/input-D.grug");
}

static void ok_on_fn_calling_no_game_fn_but_with_addition(void) {
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_no_game_fn_but_with_addition/input-D.grug");
}

static void ok_on_fn_calling_no_game_fn_but_with_global(void) {
    on_fn_dispatcher("on_a", "tests/ok/on_fn_calling_no_game_fn_but_with_global/input-D.grug");
}

static void ok_on_fn_overwriting_param(void) {
	assert(game_fn_initialize_call_count == 0);
	assert(game_fn_sin_call_count == 0);
    on_fn_args_dispatcher("on_a", "tests/ok/on_fn_overwriting_param/input-S.grug", (struct grug_value[]){{.type=grug_type_i32, .i32=2}, {.type=grug_type_f32, .f32=3.0f}}, 2);
	assert(game_fn_initialize_call_count == 1);
	assert(game_fn_sin_call_count == 1);

	assert(game_fn_initialize_x == 20);
	assert(game_fn_sin_x == 30.0f);
}

static void ok_on_fn_passing_argument_to_helper_fn(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_passing_argument_to_helper_fn/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 42);
}

static void ok_on_fn_passing_magic_to_initialize(void) {
	assert(game_fn_magic_call_count == 0);
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_passing_magic_to_initialize/input-D.grug");
	assert(game_fn_magic_call_count == 1);
	assert(game_fn_initialize_call_count == 1);
}

static void ok_on_fn_three(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_three/input-J.grug");
    on_fn_dispatcher("on_b", "tests/ok/on_fn_three/input-J.grug");
    on_fn_dispatcher("on_c", "tests/ok/on_fn_three/input-J.grug");
	assert(game_fn_nothing_call_count == 3);
}

static void ok_on_fn_three_unused_first(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_b", "tests/ok/on_fn_three_unused_first/input-J.grug");
    on_fn_dispatcher("on_c", "tests/ok/on_fn_three_unused_first/input-J.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_on_fn_three_unused_second(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_three_unused_second/input-J.grug");
    on_fn_dispatcher("on_c", "tests/ok/on_fn_three_unused_second/input-J.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_on_fn_three_unused_third(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/on_fn_three_unused_third/input-J.grug");
    on_fn_dispatcher("on_b", "tests/ok/on_fn_three_unused_third/input-J.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_or_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/or_false/input-J.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_or_short_circuit(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/or_short_circuit/input-J.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_or_true_1(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/or_true_1/input-J.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_or_true_2(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/or_true_2/input-J.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_or_true_3(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/or_true_3/input-J.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_pass_string_argument_to_game_fn(void) {
	assert(game_fn_say_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/pass_string_argument_to_game_fn/input-J.grug");
	assert(game_fn_say_call_count == 1);

	assert(streq(game_fn_say_message, "foo"));
}

static void ok_pass_string_argument_to_helper_fn(void) {
	assert(game_fn_say_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/pass_string_argument_to_helper_fn/input-J.grug");
	assert(game_fn_say_call_count == 1);

	assert(streq(game_fn_say_message, "foo"));
}

static void ok_remainder_negative_negative(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/remainder_negative_negative/input-J.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -1);
}

static void ok_remainder_negative_positive(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/remainder_negative_positive/input-J.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -1);
}

static void ok_remainder_positive_negative(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/remainder_positive_negative/input-J.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 1);
}

static void ok_remainder_positive_positive(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/remainder_positive_positive/input-J.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 1);
}

static void ok_resource_and_entity(void) {
	assert(game_fn_draw_call_count == 0);
	assert(game_fn_spawn_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/resource_and_entity/input-D.grug");
	assert(game_fn_draw_call_count == 1);
	assert(game_fn_spawn_call_count == 1);

	assert(streq(game_fn_draw_sprite_path, "tests/ok/resource_and_entity/foo.txt"));
	assert(streq(game_fn_spawn_name, "ok:foo"));
}

static void ok_resource_can_contain_dot_1(void) {
	assert(game_fn_draw_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/resource_can_contain_dot_1/input-D.grug");
	assert(game_fn_draw_call_count == 1);

	assert(streq(game_fn_draw_sprite_path, "tests/ok/resource_can_contain_dot_1/.foo"));
}

static void ok_resource_can_contain_dot_3(void) {
	assert(game_fn_draw_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/resource_can_contain_dot_3/input-D.grug");
	assert(game_fn_draw_call_count == 1);

	assert(streq(game_fn_draw_sprite_path, "tests/ok/resource_can_contain_dot_3/foo.bar"));
}

static void ok_resource_can_contain_dot_dot_1(void) {
	assert(game_fn_draw_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/resource_can_contain_dot_dot_1/input-D.grug");
	assert(game_fn_draw_call_count == 1);

	assert(streq(game_fn_draw_sprite_path, "tests/ok/resource_can_contain_dot_dot_1/..foo"));
}

static void ok_resource_can_contain_dot_dot_3(void) {
	assert(game_fn_draw_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/resource_can_contain_dot_dot_3/input-D.grug");
	assert(game_fn_draw_call_count == 1);

	assert(streq(game_fn_draw_sprite_path, "tests/ok/resource_can_contain_dot_dot_3/foo..bar"));
}

static void ok_resource_duplicate(void) {
	assert(game_fn_draw_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/resource_duplicate/input-D.grug");
	assert(game_fn_draw_call_count == 4);

	assert(streq(game_fn_draw_sprite_path, "tests/ok/resource_duplicate/baz.txt"));
}

static void ok_return(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/return/input-J.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 42);
}

static void ok_return_from_on_fn(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/return_from_on_fn/input-J.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_return_from_on_fn_minimal(void) {
    on_fn_dispatcher("on_a", "tests/ok/return_from_on_fn_minimal/input-J.grug");
}

static void ok_return_with_no_value(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/return_with_no_value/input-J.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_same_variable_name_in_different_functions(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/same_variable_name_in_different_functions/input-J.grug");
	assert(game_fn_initialize_call_count == 2);

	assert(game_fn_initialize_x == 69);
}

static void ok_spill_args_to_game_fn(void) {
	assert(game_fn_motherload_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_game_fn/input-D.grug");
	assert(game_fn_motherload_call_count == 1);

	assert(game_fn_motherload_i1 == 1);
	assert(game_fn_motherload_i2 == 2);
	assert(game_fn_motherload_i3 == 3);
	assert(game_fn_motherload_i4 == 4);
	assert(game_fn_motherload_i5 == 5);
	assert(game_fn_motherload_i6 == 6);
	assert(game_fn_motherload_i7 == 7);
	assert(game_fn_motherload_f1 == 1.0f);
	assert(game_fn_motherload_f2 == 2.0f);
	assert(game_fn_motherload_f3 == 3.0f);
	assert(game_fn_motherload_f4 == 4.0f);
	assert(game_fn_motherload_f5 == 5.0f);
	assert(game_fn_motherload_f6 == 6.0f);
	assert(game_fn_motherload_f7 == 7.0f);
	assert(game_fn_motherload_f8 == 8.0f);
	assert(game_fn_motherload_id == 42);
	assert(game_fn_motherload_f9 == 9.0f);
}

static void ok_spill_args_to_game_fn_subless(void) {
	assert(game_fn_motherload_subless_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_game_fn_subless/input-D.grug");
	assert(game_fn_motherload_subless_call_count == 1);

	assert(game_fn_motherload_subless_i1 == 1);
	assert(game_fn_motherload_subless_i2 == 2);
	assert(game_fn_motherload_subless_i3 == 3);
	assert(game_fn_motherload_subless_i4 == 4);
	assert(game_fn_motherload_subless_i5 == 5);
	assert(game_fn_motherload_subless_i6 == 6);
	assert(game_fn_motherload_subless_i7 == 7);
	assert(game_fn_motherload_subless_f1 == 1.0f);
	assert(game_fn_motherload_subless_f2 == 2.0f);
	assert(game_fn_motherload_subless_f3 == 3.0f);
	assert(game_fn_motherload_subless_f4 == 4.0f);
	assert(game_fn_motherload_subless_f5 == 5.0f);
	assert(game_fn_motherload_subless_f6 == 6.0f);
	assert(game_fn_motherload_subless_f7 == 7.0f);
	assert(game_fn_motherload_subless_f8 == 8.0f);
	assert(game_fn_motherload_subless_f9 == 9.0f);
	assert(game_fn_motherload_subless_id == 42);
	assert(game_fn_motherload_subless_f10 == 10.0f);
}

static void ok_spill_args_to_helper_fn(void) {
	assert(game_fn_motherload_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_helper_fn/input-D.grug");
	assert(game_fn_motherload_call_count == 1);

	assert(game_fn_motherload_i1 == 1);
	assert(game_fn_motherload_i2 == 2);
	assert(game_fn_motherload_i3 == 3);
	assert(game_fn_motherload_i4 == 4);
	assert(game_fn_motherload_i5 == 5);
	assert(game_fn_motherload_i6 == 6);
	assert(game_fn_motherload_i7 == 7);
	assert(game_fn_motherload_f1 == 1.0f);
	assert(game_fn_motherload_f2 == 2.0f);
	assert(game_fn_motherload_f3 == 3.0f);
	assert(game_fn_motherload_f4 == 4.0f);
	assert(game_fn_motherload_f5 == 5.0f);
	assert(game_fn_motherload_f6 == 6.0f);
	assert(game_fn_motherload_f7 == 7.0f);
	assert(game_fn_motherload_f8 == 8.0f);
	assert(game_fn_motherload_id == 42);
	assert(game_fn_motherload_f9 == 9.0f);
}

static void ok_spill_args_to_helper_fn_32_bit_f32(void) {
	assert(game_fn_offset_32_bit_f32_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_helper_fn_32_bit_f32/input-D.grug");
	assert(game_fn_offset_32_bit_f32_call_count == 1);

	assert(streq(game_fn_offset_32_bit_f32_s1, "1"));
	assert(streq(game_fn_offset_32_bit_f32_s2, "2"));
	assert(streq(game_fn_offset_32_bit_f32_s3, "3"));
	assert(streq(game_fn_offset_32_bit_f32_s4, "4"));
	assert(streq(game_fn_offset_32_bit_f32_s5, "5"));
	assert(streq(game_fn_offset_32_bit_f32_s6, "6"));
	assert(streq(game_fn_offset_32_bit_f32_s7, "7"));
	assert(streq(game_fn_offset_32_bit_f32_s8, "8"));
	assert(streq(game_fn_offset_32_bit_f32_s9, "9"));
	assert(streq(game_fn_offset_32_bit_f32_s10, "10"));
	assert(streq(game_fn_offset_32_bit_f32_s11, "11"));
	assert(streq(game_fn_offset_32_bit_f32_s12, "12"));
	assert(streq(game_fn_offset_32_bit_f32_s13, "13"));
	assert(streq(game_fn_offset_32_bit_f32_s14, "14"));
	assert(streq(game_fn_offset_32_bit_f32_s15, "15"));
	assert(game_fn_offset_32_bit_f32_f1 == 1.0f);
	assert(game_fn_offset_32_bit_f32_f2 == 2.0f);
	assert(game_fn_offset_32_bit_f32_f3 == 3.0f);
	assert(game_fn_offset_32_bit_f32_f4 == 4.0f);
	assert(game_fn_offset_32_bit_f32_f5 == 5.0f);
	assert(game_fn_offset_32_bit_f32_f6 == 6.0f);
	assert(game_fn_offset_32_bit_f32_f7 == 7.0f);
	assert(game_fn_offset_32_bit_f32_f8 == 8.0f);
	assert(game_fn_offset_32_bit_f32_g == 1);
}

static void ok_spill_args_to_helper_fn_32_bit_i32(void) {
	assert(game_fn_offset_32_bit_i32_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_helper_fn_32_bit_i32/input-D.grug");
	assert(game_fn_offset_32_bit_i32_call_count == 1);

	assert(game_fn_offset_32_bit_i32_f1 == 1.0f);
	assert(game_fn_offset_32_bit_i32_f2 == 2.0f);
	assert(game_fn_offset_32_bit_i32_f3 == 3.0f);
	assert(game_fn_offset_32_bit_i32_f4 == 4.0f);
	assert(game_fn_offset_32_bit_i32_f5 == 5.0f);
	assert(game_fn_offset_32_bit_i32_f6 == 6.0f);
	assert(game_fn_offset_32_bit_i32_f7 == 7.0f);
	assert(game_fn_offset_32_bit_i32_f8 == 8.0f);
	assert(game_fn_offset_32_bit_i32_f9 == 9.0f);
	assert(game_fn_offset_32_bit_i32_f10 == 10.0f);
	assert(game_fn_offset_32_bit_i32_f11 == 11.0f);
	assert(game_fn_offset_32_bit_i32_f12 == 12.0f);
	assert(game_fn_offset_32_bit_i32_f13 == 13.0f);
	assert(game_fn_offset_32_bit_i32_f14 == 14.0f);
	assert(game_fn_offset_32_bit_i32_f15 == 15.0f);
	assert(game_fn_offset_32_bit_i32_f16 == 16.0f);
	assert(game_fn_offset_32_bit_i32_f17 == 17.0f);
	assert(game_fn_offset_32_bit_i32_f18 == 18.0f);
	assert(game_fn_offset_32_bit_i32_f19 == 19.0f);
	assert(game_fn_offset_32_bit_i32_f20 == 20.0f);
	assert(game_fn_offset_32_bit_i32_f21 == 21.0f);
	assert(game_fn_offset_32_bit_i32_f22 == 22.0f);
	assert(game_fn_offset_32_bit_i32_f23 == 23.0f);
	assert(game_fn_offset_32_bit_i32_f24 == 24.0f);
	assert(game_fn_offset_32_bit_i32_f25 == 25.0f);
	assert(game_fn_offset_32_bit_i32_f26 == 26.0f);
	assert(game_fn_offset_32_bit_i32_f27 == 27.0f);
	assert(game_fn_offset_32_bit_i32_f28 == 28.0f);
	assert(game_fn_offset_32_bit_i32_f29 == 29.0f);
	assert(game_fn_offset_32_bit_i32_f30 == 30.0f);
	assert(game_fn_offset_32_bit_i32_i1 == 1);
	assert(game_fn_offset_32_bit_i32_i2 == 2);
	assert(game_fn_offset_32_bit_i32_i3 == 3);
	assert(game_fn_offset_32_bit_i32_i4 == 4);
	assert(game_fn_offset_32_bit_i32_i5 == 5);
	assert(game_fn_offset_32_bit_i32_g == 6);
}

static void ok_spill_args_to_helper_fn_32_bit_string(void) {
	assert(game_fn_offset_32_bit_string_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_helper_fn_32_bit_string/input-D.grug");
	assert(game_fn_offset_32_bit_string_call_count == 1);

	assert(game_fn_offset_32_bit_string_f1 == 1.0f);
	assert(game_fn_offset_32_bit_string_f2 == 2.0f);
	assert(game_fn_offset_32_bit_string_f3 == 3.0f);
	assert(game_fn_offset_32_bit_string_f4 == 4.0f);
	assert(game_fn_offset_32_bit_string_f5 == 5.0f);
	assert(game_fn_offset_32_bit_string_f6 == 6.0f);
	assert(game_fn_offset_32_bit_string_f7 == 7.0f);
	assert(game_fn_offset_32_bit_string_f8 == 8.0f);
	assert(game_fn_offset_32_bit_string_f9 == 9.0f);
	assert(game_fn_offset_32_bit_string_f10 == 10.0f);
	assert(game_fn_offset_32_bit_string_f11 == 11.0f);
	assert(game_fn_offset_32_bit_string_f12 == 12.0f);
	assert(game_fn_offset_32_bit_string_f13 == 13.0f);
	assert(game_fn_offset_32_bit_string_f14 == 14.0f);
	assert(game_fn_offset_32_bit_string_f15 == 15.0f);
	assert(game_fn_offset_32_bit_string_f16 == 16.0f);
	assert(game_fn_offset_32_bit_string_f17 == 17.0f);
	assert(game_fn_offset_32_bit_string_f18 == 18.0f);
	assert(game_fn_offset_32_bit_string_f19 == 19.0f);
	assert(game_fn_offset_32_bit_string_f20 == 20.0f);
	assert(game_fn_offset_32_bit_string_f21 == 21.0f);
	assert(game_fn_offset_32_bit_string_f22 == 22.0f);
	assert(game_fn_offset_32_bit_string_f23 == 23.0f);
	assert(game_fn_offset_32_bit_string_f24 == 24.0f);
	assert(game_fn_offset_32_bit_string_f25 == 25.0f);
	assert(game_fn_offset_32_bit_string_f26 == 26.0f);
	assert(game_fn_offset_32_bit_string_f27 == 27.0f);
	assert(game_fn_offset_32_bit_string_f28 == 28.0f);
	assert(game_fn_offset_32_bit_string_f29 == 29.0f);
	assert(game_fn_offset_32_bit_string_f30 == 30.0f);
	assert(streq(game_fn_offset_32_bit_string_s1, "1"));
	assert(streq(game_fn_offset_32_bit_string_s2, "2"));
	assert(streq(game_fn_offset_32_bit_string_s3, "3"));
	assert(streq(game_fn_offset_32_bit_string_s4, "4"));
	assert(streq(game_fn_offset_32_bit_string_s5, "5"));
	assert(game_fn_offset_32_bit_string_g == 1);
}

static void ok_spill_args_to_helper_fn_subless(void) {
	assert(game_fn_motherload_subless_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/spill_args_to_helper_fn_subless/input-D.grug");
	assert(game_fn_motherload_subless_call_count == 1);

	assert(game_fn_motherload_subless_i1 == 1);
	assert(game_fn_motherload_subless_i2 == 2);
	assert(game_fn_motherload_subless_i3 == 3);
	assert(game_fn_motherload_subless_i4 == 4);
	assert(game_fn_motherload_subless_i5 == 5);
	assert(game_fn_motherload_subless_i6 == 6);
	assert(game_fn_motherload_subless_i7 == 7);
	assert(game_fn_motherload_subless_f1 == 1.0f);
	assert(game_fn_motherload_subless_f2 == 2.0f);
	assert(game_fn_motherload_subless_f3 == 3.0f);
	assert(game_fn_motherload_subless_f4 == 4.0f);
	assert(game_fn_motherload_subless_f5 == 5.0f);
	assert(game_fn_motherload_subless_f6 == 6.0f);
	assert(game_fn_motherload_subless_f7 == 7.0f);
	assert(game_fn_motherload_subless_f8 == 8.0f);
	assert(game_fn_motherload_subless_f9 == 9.0f);
	assert(game_fn_motherload_subless_id == 42);
	assert(game_fn_motherload_subless_f10 == 10.0f);
}

static void ok_stack_16_byte_alignment(void) {
	assert(game_fn_nothing_call_count == 0);
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/stack_16_byte_alignment/input-J.grug");
	assert(game_fn_nothing_call_count == 1);
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 42);
}

static void ok_stack_16_byte_alignment_midway(void) {
	assert(game_fn_magic_call_count == 0);
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/stack_16_byte_alignment_midway/input-J.grug");
	assert(game_fn_magic_call_count == 1);
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 42 + 42);
}

static void ok_string_can_be_passed_to_helper_fn(void) {
	assert(game_fn_say_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_can_be_passed_to_helper_fn/input-D.grug");
	assert(game_fn_say_call_count == 1);

	assert(streq(game_fn_say_message, "foo"));
}

static void ok_string_duplicate(void) {
	assert(game_fn_talk_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_duplicate/input-D.grug");
	assert(game_fn_talk_call_count == 1);

	assert(streq(game_fn_talk_message1, "foo"));
	assert(streq(game_fn_talk_message2, "bar"));
	assert(streq(game_fn_talk_message3, "bar"));
	assert(streq(game_fn_talk_message4, "baz"));
}

static void ok_string_eq_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_eq_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_string_eq_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_eq_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_string_eq_true_empty(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_eq_true_empty/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_string_ne_false(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_ne_false/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_string_ne_false_empty(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_ne_false_empty/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == false);
}

static void ok_string_ne_true(void) {
	assert(game_fn_initialize_bool_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/string_ne_true/input-D.grug");
	assert(game_fn_initialize_bool_call_count == 1);

	assert(game_fn_initialize_bool_b == true);
}

static void ok_sub_rsp_32_bits_local_variables_i32(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/sub_rsp_32_bits_local_variables_i32/input-D.grug");
	assert(game_fn_initialize_call_count == 30);

	assert(game_fn_initialize_x == 30);
}

static void ok_sub_rsp_32_bits_local_variables_id(void) {
	assert(game_fn_set_d_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/sub_rsp_32_bits_local_variables_id/input-D.grug");
	assert(game_fn_set_d_call_count == 15);

	assert(game_fn_set_d_target == 42);
}

static void ok_subtraction_negative_result(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/subtraction_negative_result/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == -3);
}

static void ok_subtraction_positive_result(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/subtraction_positive_result/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 3);
}

static void ok_variable(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/variable/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 42);
}

static void ok_variable_does_not_shadow_in_different_if_statement(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/variable_does_not_shadow_in_different_if_statement/input-D.grug");
	assert(game_fn_initialize_call_count == 2);

	assert(game_fn_initialize_x == 69);
}

static void ok_variable_reassignment(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/variable_reassignment/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 69);
}

static void ok_variable_reassignment_does_not_dealloc_outer_variable(void) {
	assert(game_fn_initialize_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/variable_reassignment_does_not_dealloc_outer_variable/input-D.grug");
	assert(game_fn_initialize_call_count == 1);

	assert(game_fn_initialize_x == 69);
}

static void ok_variable_string_global(void) {
	assert(game_fn_say_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/variable_string_global/input-D.grug");
	assert(game_fn_say_call_count == 1);

	assert(streq(game_fn_say_message, "foo"));
}

static void ok_variable_string_local(void) {
	assert(game_fn_say_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/variable_string_local/input-D.grug");
	assert(game_fn_say_call_count == 1);

	assert(streq(game_fn_say_message, "foo"));
}

static void ok_void_function_early_return(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/void_function_early_return/input-D.grug");
	assert(game_fn_nothing_call_count == 1);
}

static void ok_while_false(void) {
	assert(game_fn_nothing_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/while_false/input-D.grug");
	assert(game_fn_nothing_call_count == 2);
}

static void ok_write_to_global_variable(void) {
	assert(game_fn_max_call_count == 0);
    on_fn_dispatcher("on_a", "tests/ok/write_to_global_variable/input-D.grug");
	assert(game_fn_max_call_count == 1);

	assert(game_fn_max_x == 43);
	assert(game_fn_max_y == 69);
}

void grug_tests_runtime_error_handler(const char *reason, enum grug_runtime_error_type type, const char *on_fn_name, const char *on_fn_path) {
	had_runtime_error = true;
	error_handler_calls++;

	runtime_error_reason = reason;
	runtime_error_type = type;
	runtime_error_on_fn_name = on_fn_name;
	runtime_error_on_fn_path = on_fn_path;
}

static void runtime_error_all(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/all/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_DIVISION_BY_ZERO);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/all/input-D.grug"));
}

static void runtime_error_division_by_0(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/division_by_0/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_DIVISION_BY_ZERO);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/division_by_0/input-D.grug"));
}

static void runtime_error_game_fn_error(void) {
	assert(game_fn_cause_game_fn_error_call_count == 0);
	assert(error_handler_calls == 0);
	on_fn_dispatcher("on_a", "tests/err_runtime/game_fn_error/input-D.grug");
	assert(game_fn_cause_game_fn_error_call_count == 1);
	assert(error_handler_calls == 1);

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_GAME_FN_ERROR);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/game_fn_error/input-D.grug"));
}

static void runtime_error_game_fn_error_once(void) {
	assert(game_fn_cause_game_fn_error_call_count == 0);
	assert(error_handler_calls == 0);
	on_fn_dispatcher("on_a", "tests/err_runtime/game_fn_error_once/input-E.grug");
	assert(game_fn_cause_game_fn_error_call_count == 1);
	assert(error_handler_calls == 1);

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_GAME_FN_ERROR);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/game_fn_error_once/input-E.grug"));

	had_runtime_error = false;

	assert(game_fn_cause_game_fn_error_call_count == 1);
	assert(game_fn_nothing_call_count == 0);
	assert(error_handler_calls == 1);
	on_fn_dispatcher("on_b", "tests/err_runtime/game_fn_error_once/input-E.grug");
	assert(game_fn_cause_game_fn_error_call_count == 1);
	assert(game_fn_nothing_call_count == 1);
	assert(error_handler_calls == 1);

	assert(!had_runtime_error);
}

static void runtime_error_i32_overflow_addition(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_overflow_addition/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_overflow_addition/input-D.grug"));
}

static void runtime_error_i32_overflow_division(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_overflow_division/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_overflow_division/input-D.grug"));
}

static void runtime_error_i32_overflow_multiplication(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_overflow_multiplication/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_overflow_multiplication/input-D.grug"));
}

static void runtime_error_i32_overflow_negation(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_overflow_negation/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_overflow_negation/input-D.grug"));
}

static void runtime_error_i32_overflow_remainder(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_overflow_remainder/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_overflow_remainder/input-D.grug"));
}

static void runtime_error_i32_overflow_subtraction(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_overflow_subtraction/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_overflow_subtraction/input-D.grug"));
}

static void runtime_error_i32_underflow_addition(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_underflow_addition/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_underflow_addition/input-D.grug"));
}

static void runtime_error_i32_underflow_multiplication(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_underflow_multiplication/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_underflow_multiplication/input-D.grug"));
}

static void runtime_error_i32_underflow_subtraction(void) {
	on_fn_dispatcher("on_a", "tests/err_runtime/i32_underflow_subtraction/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/i32_underflow_subtraction/input-D.grug"));
}

static void runtime_error_on_fn_calls_erroring_on_fn(void) {
	saved_grug_path = "tests/err_runtime/on_fn_calls_erroring_on_fn/input-E.grug";

	assert(game_fn_call_on_b_fn_call_count == 0);
	assert(game_fn_cause_game_fn_error_call_count == 0);
	assert(game_fn_nothing_call_count == 0);
	assert(error_handler_calls == 0);
    on_fn_dispatcher("on_a", "tests/err_runtime/on_fn_calls_erroring_on_fn/input-E.grug");
	assert(game_fn_call_on_b_fn_call_count == 1);
	assert(game_fn_cause_game_fn_error_call_count == 1);
	assert(game_fn_nothing_call_count == 0);
	assert(error_handler_calls == 1);

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_GAME_FN_ERROR);

	assert(streq(runtime_error_on_fn_name, "on_b"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/on_fn_calls_erroring_on_fn/input-E.grug"));
}

static void runtime_error_on_fn_errors_after_it_calls_other_on_fn(void) {
	saved_grug_path = "tests/err_runtime/on_fn_errors_after_it_calls_other_on_fn/input-E.grug";

	assert(game_fn_call_on_b_fn_call_count == 0);
	assert(game_fn_nothing_call_count == 0);
	assert(game_fn_cause_game_fn_error_call_count == 0);
	assert(error_handler_calls == 0);
	on_fn_dispatcher("on_a", "tests/err_runtime/on_fn_errors_after_it_calls_other_on_fn/input-E.grug");
	assert(game_fn_call_on_b_fn_call_count == 1);
	assert(game_fn_nothing_call_count == 1);
	assert(game_fn_cause_game_fn_error_call_count == 1);
	assert(error_handler_calls == 1);

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_GAME_FN_ERROR);

	assert(streq(runtime_error_on_fn_name, "on_b"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/on_fn_errors_after_it_calls_other_on_fn/input-E.grug"));
}

static void runtime_error_remainder_by_0(void) {
    on_fn_dispatcher("on_a", "tests/err_runtime/remainder_by_0/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_DIVISION_BY_ZERO);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/remainder_by_0/input-D.grug"));
}

static void runtime_error_stack_overflow(void) {
    on_fn_dispatcher("on_a", "tests/err_runtime/stack_overflow/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_STACK_OVERFLOW);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/stack_overflow/input-D.grug"));
}

static void runtime_error_time_limit_exceeded(void) {
    on_fn_dispatcher("on_a", "tests/err_runtime/time_limit_exceeded/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_TIME_LIMIT_EXCEEDED);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/time_limit_exceeded/input-D.grug"));
}

static void runtime_error_time_limit_exceeded_exponential_calls(void) {
    on_fn_dispatcher("on_a", "tests/err_runtime/time_limit_exceeded_exponential_calls/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_TIME_LIMIT_EXCEEDED);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/time_limit_exceeded_exponential_calls/input-D.grug"));
}

static void runtime_error_time_limit_exceeded_fibonacci(void) {
    on_fn_dispatcher("on_a", "tests/err_runtime/time_limit_exceeded_fibonacci/input-D.grug");

	assert(had_runtime_error);

	assert(runtime_error_type == GRUG_ON_FN_TIME_LIMIT_EXCEEDED);

	assert(streq(runtime_error_on_fn_name, "on_a"));
	assert(streq(runtime_error_on_fn_path, "tests/err_runtime/time_limit_exceeded_fibonacci/input-D.grug"));
}

#define CHECK_THAT_EVERY_TEST_DIRECTORY_HAS_A_FUNCTION(test_dirname) {\
	size_t entries = 0;\
	\
	DIR *dirp = opendir(prefix(#test_dirname));\
	if (dirp == NULL) {\
		perror("opendir");\
		fprintf(stderr, "prefix(#test_dirname): \"%s\"\n", prefix(#test_dirname));\
		exit(EXIT_FAILURE);\
	}\
	\
	struct dirent *dp;\
	while ((dp = readdir(dirp))) {\
		if (streq(dp->d_name, ".") || streq(dp->d_name, "..")) {\
			continue;\
		}\
		entries++;\
	}\
	\
	if (entries != test_dirname ## _test_datas_size) {\
		fprintf(stderr, "The tests/" #test_dirname "/ directory contains %zu entries, which doesn't match it having %zu test functions\n", entries, test_dirname ## _test_datas_size);\
		exit(EXIT_FAILURE);\
	}\
	\
	assert(closedir(dirp) == 0);\
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
	ADD_TEST_ERROR(entity_has_invalid_entity_name_colon, "D");
	ADD_TEST_ERROR(entity_has_invalid_entity_name_uppercase, "D");
	ADD_TEST_ERROR(entity_has_invalid_mod_name_uppercase, "D");
	ADD_TEST_ERROR(entity_mod_name_and_entity_name_is_missing, "D");
	ADD_TEST_ERROR(entity_mod_name_cant_be_current_mod, "D");
	ADD_TEST_ERROR(entity_mod_name_is_missing, "D");
	ADD_TEST_ERROR(entity_name_is_missing, "D");
	ADD_TEST_ERROR(f32_missing_digit_after_decimal_point, "D");
	ADD_TEST_ERROR(f32_too_big, "D");
	ADD_TEST_ERROR(f32_too_close_to_zero_negative, "D");
	ADD_TEST_ERROR(f32_too_close_to_zero_positive, "D");
	ADD_TEST_ERROR(f32_too_small, "D");
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
	ADD_TEST_ERROR(global_variable_contains_resource, "A");
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
	ADD_TEST_ERROR(helper_fn_is_not_called_1, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_2, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_3, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_4, "D");
	ADD_TEST_ERROR(helper_fn_is_not_called_5, "D");
	ADD_TEST_ERROR(helper_fn_missing_return_statement, "D");
	ADD_TEST_ERROR(helper_fn_no_return_value_expected, "D");
	ADD_TEST_ERROR(helper_fn_return_with_comment_after_it, "D");
	ADD_TEST_ERROR(i32_logical_not, "D");
	ADD_TEST_ERROR(i32_too_big, "D");
	ADD_TEST_ERROR(i32_too_small, "D");
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
	ADD_TEST_ERROR(local_variable_already_exists, "D");
	ADD_TEST_ERROR(local_variable_definition_cant_use_itself, "D");
	ADD_TEST_ERROR(local_variable_definition_missing_type, "D");
	ADD_TEST_ERROR(max_expr_recursion_depth_exceeded, "D");
	ADD_TEST_ERROR(max_statement_recursion_depth_exceeded, "D");
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
	ADD_TEST_ERROR(pass_bool_to_i32_game_param, "D");
	ADD_TEST_ERROR(pass_bool_to_i32_helper_param, "D");
	ADD_TEST_ERROR(remainder_by_float, "D");
	ADD_TEST_ERROR(resource_cant_be_empty_string, "D");
	ADD_TEST_ERROR(resource_cant_be_passed_to_helper_fn, "D");
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
	ADD_TEST_ERROR(resource_type_for_global, "A");
	ADD_TEST_ERROR(resource_type_for_helper_fn_argument, "D");
	ADD_TEST_ERROR(resource_type_for_helper_fn_return_type, "D");
	ADD_TEST_ERROR(resource_type_for_local, "D");
	ADD_TEST_ERROR(resource_type_for_on_fn_argument, "D");
	ADD_TEST_ERROR(string_pointer_arithmetic, "D");
	ADD_TEST_ERROR(trailing_space_in_comment, "D");
	ADD_TEST_ERROR(unary_plus_does_not_exist, "D");
	ADD_TEST_ERROR(unclosed_double_quote, "A");
	ADD_TEST_ERROR(unknown_variable, "D");
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
	ADD_TEST_ERROR(wrong_type_global_assignment, "D");
	ADD_TEST_ERROR(wrong_type_global_reassignment, "D");
	ADD_TEST_ERROR(wrong_type_local_assignment, "D");
	ADD_TEST_ERROR(wrong_type_local_reassignment, "D");
}

static void add_ok_tests(void) {
	ADD_TEST_OK(addition_as_argument, "D", 8);
	ADD_TEST_OK(addition_as_two_arguments, "D", 8);
	ADD_TEST_OK(addition_with_multiplication, "D", 8);
	ADD_TEST_OK(addition_with_multiplication_2, "D", 8);
	ADD_TEST_OK(and_false_1, "D", 8);
	ADD_TEST_OK(and_false_2, "D", 8);
	ADD_TEST_OK(and_false_3, "D", 8);
	ADD_TEST_OK(and_short_circuit, "D", 8);
	ADD_TEST_OK(and_true, "D", 8);
	ADD_TEST_OK(blocked_alrm, "D", 8);
	ADD_TEST_OK(bool_logical_not_false, "D", 8);
	ADD_TEST_OK(bool_logical_not_true, "D", 8);
	ADD_TEST_OK(bool_returned, "D", 8);
	ADD_TEST_OK(bool_returned_global, "D", 9);
	ADD_TEST_OK(bool_zero_extended_if_statement, "D", 8);
	ADD_TEST_OK(bool_zero_extended_while_statement, "D", 8);
	ADD_TEST_OK(break, "D", 8);
	ADD_TEST_OK(calls_100, "D", 8);
	ADD_TEST_OK(calls_1000, "D", 8);
	ADD_TEST_OK(calls_in_call, "D", 8);
	ADD_TEST_OK(comment_above_block, "D", 8);
	ADD_TEST_OK(comment_above_block_twice, "D", 8);
	ADD_TEST_OK(comment_above_globals, "A", 20);
	ADD_TEST_OK(comment_above_helper_fn, "D", 8);
	ADD_TEST_OK(comment_above_on_fn, "D", 8);
	ADD_TEST_OK(comment_between_globals, "A", 16);
	ADD_TEST_OK(comment_between_statements, "D", 8);
	ADD_TEST_OK(comment_lone_block, "D", 8);
	ADD_TEST_OK(comment_lone_block_at_end, "D", 8);
	ADD_TEST_OK(comment_lone_global, "D", 8);
	ADD_TEST_OK(continue, "D", 8);
	ADD_TEST_OK(custom_id_decays_to_id, "D", 8);
	ADD_TEST_OK(custom_id_transfer_between_globals, "D", 24);
	ADD_TEST_OK(custom_id_with_digits, "A", 16);
	ADD_TEST_OK(division_negative_result, "D", 8);
	ADD_TEST_OK(division_positive_result, "D", 8);
	ADD_TEST_OK(double_negation_with_parentheses, "D", 8);
	ADD_TEST_OK(double_not_with_parentheses, "D", 8);
	ADD_TEST_OK(else_after_else_if_false, "D", 8);
	ADD_TEST_OK(else_after_else_if_true, "D", 8);
	ADD_TEST_OK(else_false, "D", 8);
	ADD_TEST_OK(else_if_false, "D", 8);
	ADD_TEST_OK(else_if_true, "D", 8);
	ADD_TEST_OK(else_true, "D", 8);
	ADD_TEST_OK(empty_file, "A", 8);
	ADD_TEST_OK(empty_line, "D", 8);
	ADD_TEST_OK(entity_and_resource_as_subexpression, "D", 8);
	ADD_TEST_OK(entity_duplicate, "D", 8);
	ADD_TEST_OK(entity_in_on_fn, "D", 8);
	ADD_TEST_OK(entity_in_on_fn_with_mod_specified, "D", 8);
	ADD_TEST_OK(eq_false, "D", 8);
	ADD_TEST_OK(eq_true, "D", 8);
	ADD_TEST_OK(f32_addition, "D", 8);
	ADD_TEST_OK(f32_argument, "D", 8);
	ADD_TEST_OK(f32_division, "D", 8);
	ADD_TEST_OK(f32_eq_false, "D", 8);
	ADD_TEST_OK(f32_eq_true, "D", 8);
	ADD_TEST_OK(f32_ge_false, "D", 8);
	ADD_TEST_OK(f32_ge_true_1, "D", 8);
	ADD_TEST_OK(f32_ge_true_2, "D", 8);
	ADD_TEST_OK(f32_global_variable, "D", 12);
	ADD_TEST_OK(f32_gt_false, "D", 8);
	ADD_TEST_OK(f32_gt_true, "D", 8);
	ADD_TEST_OK(f32_le_false, "D", 8);
	ADD_TEST_OK(f32_le_true_1, "D", 8);
	ADD_TEST_OK(f32_le_true_2, "D", 8);
	ADD_TEST_OK(f32_local_variable, "D", 8);
	ADD_TEST_OK(f32_lt_false, "D", 8);
	ADD_TEST_OK(f32_lt_true, "D", 8);
	ADD_TEST_OK(f32_multiplication, "D", 8);
	ADD_TEST_OK(f32_ne_false, "D", 8);
	ADD_TEST_OK(f32_negated, "D", 8);
	ADD_TEST_OK(f32_ne_true, "D", 8);
	ADD_TEST_OK(f32_passed_to_helper_fn, "D", 8);
	ADD_TEST_OK(f32_passed_to_on_fn, "R", 8);
	ADD_TEST_OK(f32_passing_sin_to_cos, "D", 8);
	ADD_TEST_OK(f32_subtraction, "D", 8);
	ADD_TEST_OK(fibonacci, "D", 8);
	ADD_TEST_OK(ge_false, "D", 8);
	ADD_TEST_OK(ge_true_1, "D", 8);
	ADD_TEST_OK(ge_true_2, "D", 8);
	ADD_TEST_OK(global_2_does_not_have_error_handling, "A", 16);
	ADD_TEST_OK(global_call_using_me, "D", 16);
	ADD_TEST_OK(global_can_use_earlier_global, "D", 16);
	ADD_TEST_OK(global_containing_negation, "D", 12);
	ADD_TEST_OK(global_id, "D", 16);
	ADD_TEST_OK(globals, "D", 16);
	ADD_TEST_OK(globals_1000, "A", 4008);
	ADD_TEST_OK(globals_1000_string, "A", 8008);
	ADD_TEST_OK(globals_32, "A", 136);
	ADD_TEST_OK(globals_64, "A", 264);
	ADD_TEST_OK(gt_false, "D", 8);
	ADD_TEST_OK(gt_true, "D", 8);
	ADD_TEST_OK(helper_fn, "D", 8);
	ADD_TEST_OK(helper_fn_called_in_if, "D", 8);
	ADD_TEST_OK(helper_fn_called_indirectly, "D", 8);
	ADD_TEST_OK(helper_fn_overwriting_param, "D", 8);
	ADD_TEST_OK(helper_fn_returning_void_has_no_return, "D", 8);
	ADD_TEST_OK(helper_fn_returning_void_returns_void, "D", 8);
	ADD_TEST_OK(helper_fn_same_param_name_as_on_fn, "F", 8);
	ADD_TEST_OK(helper_fn_same_param_name_as_other_helper_fn, "F", 8);
	ADD_TEST_OK(i32_max, "D", 8);
	ADD_TEST_OK(i32_min, "D", 8);
	ADD_TEST_OK(i32_negated, "D", 8);
	ADD_TEST_OK(i32_negative_is_smaller_than_positive, "D", 8);
	ADD_TEST_OK(id_binary_expr_false, "D", 8);
	ADD_TEST_OK(id_binary_expr_true, "D", 8);
	ADD_TEST_OK(id_eq_1, "D", 8);
	ADD_TEST_OK(id_eq_2, "D", 8);
	ADD_TEST_OK(id_global_with_id_to_new_id, "D", 16);
	ADD_TEST_OK(id_global_with_opponent_to_new_id, "D", 16);
	ADD_TEST_OK(id_helper_fn_param, "D", 8);
	ADD_TEST_OK(id_local_variable_get_and_set, "D", 8);
	ADD_TEST_OK(id_ne_1, "D", 8);
	ADD_TEST_OK(id_ne_2, "D", 8);
	ADD_TEST_OK(id_on_fn_param, "U", 8);
	ADD_TEST_OK(id_returned_from_helper, "D", 8);
	ADD_TEST_OK(id_with_d_to_new_id_and_id_to_old_id, "D", 8);
	ADD_TEST_OK(id_with_d_to_old_id, "D", 8);
	ADD_TEST_OK(id_with_id_to_new_id, "D", 8);
	ADD_TEST_OK(if_false, "D", 8);
	ADD_TEST_OK(if_true, "D", 8);
	ADD_TEST_OK(le_false, "D", 8);
	ADD_TEST_OK(le_true_1, "D", 8);
	ADD_TEST_OK(le_true_2, "D", 8);
	ADD_TEST_OK(local_id_can_be_reassigned, "D", 8);
	ADD_TEST_OK(lt_false, "D", 8);
	ADD_TEST_OK(lt_true, "D", 8);
	ADD_TEST_OK(max_args, "D", 8);
	ADD_TEST_OK(me, "D", 8);
	ADD_TEST_OK(me_assigned_to_local_variable, "D", 8);
	ADD_TEST_OK(me_passed_to_helper_fn, "D", 8);
	ADD_TEST_OK(mov_32_bits_global_i32, "A", 0x84);
	ADD_TEST_OK(mov_32_bits_global_id, "A", 0x88);
	ADD_TEST_OK(multiplication_as_two_arguments, "D", 8);
	ADD_TEST_OK(ne_false, "D", 8);
	ADD_TEST_OK(ne_true, "D", 8);
	ADD_TEST_OK(negate_parenthesized_expr, "D", 8);
	ADD_TEST_OK(negative_literal, "D", 8);
	ADD_TEST_OK(nested_break, "D", 8);
	ADD_TEST_OK(nested_continue, "D", 8);
	ADD_TEST_OK(no_empty_line_between_globals, "A", 16);
	ADD_TEST_OK(no_empty_line_between_statements, "D", 8);
	ADD_TEST_OK(on_fn, "D", 8);
	ADD_TEST_OK(on_fn_calling_game_fn_nothing, "D", 8);
	ADD_TEST_OK(on_fn_calling_game_fn_nothing_twice, "D", 8);
	ADD_TEST_OK(on_fn_calling_game_fn_plt_order, "D", 8);
	ADD_TEST_OK(on_fn_calling_helper_fns, "D", 8);
	ADD_TEST_OK(on_fn_calling_no_game_fn, "D", 8);
	ADD_TEST_OK(on_fn_calling_no_game_fn_but_with_addition, "D", 8);
	ADD_TEST_OK(on_fn_calling_no_game_fn_but_with_global, "D", 12);
	ADD_TEST_OK(on_fn_overwriting_param, "S", 8);
	ADD_TEST_OK(on_fn_passing_argument_to_helper_fn, "D", 8);
	ADD_TEST_OK(on_fn_passing_magic_to_initialize, "D", 8);
	ADD_TEST_OK(on_fn_three, "J", 8);
	ADD_TEST_OK(on_fn_three_unused_first, "J", 8);
	ADD_TEST_OK(on_fn_three_unused_second, "J", 8);
	ADD_TEST_OK(on_fn_three_unused_third, "J", 8);
	ADD_TEST_OK(or_false, "D", 8);
	ADD_TEST_OK(or_short_circuit, "D", 8);
	ADD_TEST_OK(or_true_1, "D", 8);
	ADD_TEST_OK(or_true_2, "D", 8);
	ADD_TEST_OK(or_true_3, "D", 8);
	ADD_TEST_OK(pass_string_argument_to_game_fn, "D", 8);
	ADD_TEST_OK(pass_string_argument_to_helper_fn, "D", 8);
	ADD_TEST_OK(remainder_negative_negative, "D", 8);
	ADD_TEST_OK(remainder_negative_positive, "D", 8);
	ADD_TEST_OK(remainder_positive_negative, "D", 8);
	ADD_TEST_OK(remainder_positive_positive, "D", 8);
	ADD_TEST_OK(resource_and_entity, "D", 8);
	ADD_TEST_OK(resource_can_contain_dot_1, "D", 8);
	ADD_TEST_OK(resource_can_contain_dot_3, "D", 8);
	ADD_TEST_OK(resource_can_contain_dot_dot_1, "D", 8);
	ADD_TEST_OK(resource_can_contain_dot_dot_3, "D", 8);
	ADD_TEST_OK(resource_duplicate, "D", 8);
	ADD_TEST_OK(return, "D", 8);
	ADD_TEST_OK(return_from_on_fn, "D", 8);
	ADD_TEST_OK(return_from_on_fn_minimal, "D", 8);
	ADD_TEST_OK(return_with_no_value, "D", 8);
	ADD_TEST_OK(same_variable_name_in_different_functions, "E", 8);
	ADD_TEST_OK(spill_args_to_game_fn, "D", 8);
	ADD_TEST_OK(spill_args_to_game_fn_subless, "D", 8);
	ADD_TEST_OK(spill_args_to_helper_fn, "D", 12);
	ADD_TEST_OK(spill_args_to_helper_fn_32_bit_f32, "D", 12);
	ADD_TEST_OK(spill_args_to_helper_fn_32_bit_i32, "D", 12);
	ADD_TEST_OK(spill_args_to_helper_fn_32_bit_string, "D", 12);
	ADD_TEST_OK(spill_args_to_helper_fn_subless, "D", 12);
	ADD_TEST_OK(stack_16_byte_alignment, "D", 8);
	ADD_TEST_OK(stack_16_byte_alignment_midway, "D", 8);
	ADD_TEST_OK(string_can_be_passed_to_helper_fn, "D", 8);
	ADD_TEST_OK(string_duplicate, "D", 8);
	ADD_TEST_OK(string_eq_false, "D", 8);
	ADD_TEST_OK(string_eq_true, "D", 8);
	ADD_TEST_OK(string_eq_true_empty, "D", 8);
	ADD_TEST_OK(string_ne_false, "D", 8);
	ADD_TEST_OK(string_ne_false_empty, "D", 8);
	ADD_TEST_OK(string_ne_true, "D", 8);
	ADD_TEST_OK(sub_rsp_32_bits_local_variables_i32, "D", 8);
	ADD_TEST_OK(sub_rsp_32_bits_local_variables_id, "D", 8);
	ADD_TEST_OK(subtraction_negative_result, "D", 8);
	ADD_TEST_OK(subtraction_positive_result, "D", 8);
	ADD_TEST_OK(variable, "D", 8);
	ADD_TEST_OK(variable_does_not_shadow_in_different_if_statement, "D", 8);
	ADD_TEST_OK(variable_reassignment, "D", 8);
	ADD_TEST_OK(variable_reassignment_does_not_dealloc_outer_variable, "D", 8);
	ADD_TEST_OK(variable_string_global, "D", 16);
	ADD_TEST_OK(variable_string_local, "D", 8);
	ADD_TEST_OK(void_function_early_return, "D", 8);
	ADD_TEST_OK(while_false, "D", 8);
	ADD_TEST_OK(write_to_global_variable, "D", 16);
}

static void add_runtime_error_tests(void) {
	ADD_TEST_RUNTIME_ERROR(all, "D", 8);
	ADD_TEST_RUNTIME_ERROR(division_by_0, "D", 8);
	ADD_TEST_RUNTIME_ERROR(game_fn_error, "D", 8);
	ADD_TEST_RUNTIME_ERROR(game_fn_error_once, "E", 8);
	ADD_TEST_RUNTIME_ERROR(i32_overflow_addition, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_overflow_division, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_overflow_multiplication, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_overflow_negation, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_overflow_remainder, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_overflow_subtraction, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_underflow_addition, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_underflow_multiplication, "D", 8);
	ADD_TEST_RUNTIME_ERROR(i32_underflow_subtraction, "D", 8);
	ADD_TEST_RUNTIME_ERROR(on_fn_calls_erroring_on_fn, "E", 8);
	ADD_TEST_RUNTIME_ERROR(on_fn_errors_after_it_calls_other_on_fn, "E", 8);
	ADD_TEST_RUNTIME_ERROR(remainder_by_0, "D", 8);
	ADD_TEST_RUNTIME_ERROR(stack_overflow, "D", 8);
	ADD_TEST_RUNTIME_ERROR(time_limit_exceeded, "D", 8);
	ADD_TEST_RUNTIME_ERROR(time_limit_exceeded_exponential_calls, "D", 8);
	ADD_TEST_RUNTIME_ERROR(time_limit_exceeded_fibonacci, "D", 8);
}

void grug_tests_run(const char *tests_dir_path_, compile_grug_file_t compile_grug_file_, init_globals_fn_dispatcher_t init_globals_fn_dispatcher_, on_fn_dispatcher_t on_fn_dispatcher_, dump_file_to_json_t dump_file_to_json_, generate_file_from_json_t generate_file_from_json_, game_fn_error_t game_fn_error_, const char *whitelisted_test_) {
	tests_dir_path = tests_dir_path_;
	compile_grug_file = compile_grug_file_;
	init_globals_fn_dispatcher = init_globals_fn_dispatcher_;
	on_fn_args_dispatcher = on_fn_dispatcher_;
	dump_file_to_json = dump_file_to_json_;
	generate_file_from_json = generate_file_from_json_;
	game_fn_error = game_fn_error_;
	whitelisted_test = whitelisted_test_;

	add_error_tests();
	add_runtime_error_tests();
	add_ok_tests();

	if (whitelisted_test == NULL) {
		CHECK_THAT_EVERY_TEST_DIRECTORY_HAS_A_FUNCTION(err);
		CHECK_THAT_EVERY_TEST_DIRECTORY_HAS_A_FUNCTION(ok);
		CHECK_THAT_EVERY_TEST_DIRECTORY_HAS_A_FUNCTION(err_runtime);
	}

	if (err_test_datas_size + ok_test_datas_size + err_runtime_test_datas_size == 0) {
		fprintf(stderr, "No tests to execute\n");
		exit(EXIT_FAILURE);
	}

#ifdef SHUFFLES
	// TODO: Allow this seed to be set using an ifdef.
	unsigned int seed = time(NULL);
	printf("The seed is %u\n", seed);
	srand(seed);

	for (size_t shuffle = 0; shuffle < SHUFFLES; shuffle++) {
	SHUFFLE(error_test_datas, err_test_datas_size, struct error_test_data);
	SHUFFLE(ok_test_datas, ok_test_datas_size, struct ok_test_data);
	SHUFFLE(runtime_error_test_datas, err_runtime_test_datas_size, struct runtime_error_test_data);
#endif

	for (size_t i = 0; i < err_test_datas_size; i++) {
		struct error_test_data fn_data = error_test_datas[i];

		test_error(
			fn_data.test_name_str,
			fn_data.grug_path,
			fn_data.expected_error_path,
			fn_data.results_path,
			fn_data.grug_output_path,
			fn_data.failed_file_path
		);
	}

	for (size_t i = 0; i < ok_test_datas_size; i++) {
		struct ok_test_data fn_data = ok_test_datas[i];

		if (whitelisted_test == NULL
		&& failed_file_doesnt_exist(fn_data.failed_file_path)
		&& shuffles_was_not_defined()
		&& newer(fn_data.applied_path, fn_data.grug_path)
		&& newer(fn_data.applied_path, "mod_api.json")
		&& newer(fn_data.applied_path, "tests.sh")
		&& newer(fn_data.applied_path, "smoketest")
		&& newer(fn_data.applied_path, "smoketest.c")
		) {
			printf("Skipping tests/ok/%s...\n", fn_data.test_name_str);
			continue;
		}

		printf("Running tests/ok/%s...\n", fn_data.test_name_str);

		prologue(fn_data.grug_path, fn_data.results_path, fn_data.failed_file_path, "ok");

		diff_roundtrip(fn_data.grug_path, fn_data.dump_path, fn_data.applied_path);

		fn_data.run();

		unlink(fn_data.failed_file_path);
	}

	for (size_t i = 0; i < err_runtime_test_datas_size; i++) {
		struct runtime_error_test_data fn_data = runtime_error_test_datas[i];

		if (whitelisted_test == NULL
		&& failed_file_doesnt_exist(fn_data.failed_file_path)
		&& shuffles_was_not_defined()
		&& newer(fn_data.applied_path, fn_data.grug_path)
		&& newer(fn_data.applied_path, fn_data.expected_error_path)
		&& newer(fn_data.applied_path, "mod_api.json")
		&& newer(fn_data.applied_path, "tests.sh")
		&& newer(fn_data.applied_path, "smoketest")
		&& newer(fn_data.applied_path, "smoketest.c")
		) {
			printf("Skipping tests/err_runtime/%s...\n", fn_data.test_name_str);
			continue;
		}

		printf("Running tests/err_runtime/%s...\n", fn_data.test_name_str);

		prologue(fn_data.grug_path, fn_data.results_path, fn_data.failed_file_path, "err_runtime");

		diff_roundtrip(fn_data.grug_path, fn_data.dump_path, fn_data.applied_path);

		fn_data.run();

		const char *expected_error = get_expected_error(fn_data.expected_error_path);

		if (!streq(runtime_error_reason, expected_error)) {
			fprintf(stderr, "\nThe error message differs from the expected error message.\n");
			fprintf(stderr, "Output:\n");
			fprintf(stderr, "%s\n", runtime_error_reason);

			fprintf(stderr, "Expected:\n");
			fprintf(stderr, "%s\n", expected_error);

			exit(EXIT_FAILURE);
		}

		unlink(fn_data.failed_file_path);
	}

#ifdef SHUFFLES
	}
#endif

	printf("\nAll tests passed! 🎉\n");
}
