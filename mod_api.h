#pragma once

#include <stdbool.h>
#include <stdint.h>

struct d_on_fns {
    void (*a)(void *globals);
};
struct e_on_fns {
    void (*a)(void *globals);
    void (*b)(void *globals);
};
struct j_on_fns {
    void (*a)(void *globals);
    void (*b)(void *globals);
    void (*c)(void *globals);
};
struct r_on_fns {
    void (*a)(void *globals, float f);
};
struct s_on_fns {
    void (*a)(void *globals, int32_t i, float f);
};
struct u_on_fns {
    void (*a)(void *globals, uint64_t id);
};

void game_fn_nothing(void);
int32_t game_fn_magic(void);
void game_fn_initialize(int32_t x);
void game_fn_initialize_bool(bool b);
int32_t game_fn_identity(int32_t x);
int32_t game_fn_max(int32_t x, int32_t y);
void game_fn_say(const char *message);
float game_fn_sin(float x);
float game_fn_cos(float x);
void game_fn_mega(float f1, int32_t i1, bool b1, float f2, float f3, float f4, bool b2, int32_t i2, float f5, float f6, float f7, float f8, uint64_t id, const char *str);
int game_fn_get_evil_false(void);
void game_fn_set_is_happy(bool is_happy);
void game_fn_mega_f32(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9);
void game_fn_mega_i32(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7);
void game_fn_draw(const char *sprite_path);
void game_fn_blocked_alrm(void);
void game_fn_nothing(void);
void game_fn_spawn(const char *name);
bool game_fn_has_resource(const char *path);
bool game_fn_has_entity(const char *name);
bool game_fn_has_string(const char *str);
uint64_t game_fn_get_opponent(void);
void game_fn_set_d(uint64_t target);
void game_fn_set_opponent(uint64_t target);
void game_fn_motherload(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, uint64_t id, float f9);
void game_fn_motherload_subless(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6, int32_t i7, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, uint64_t id, float f10);
void game_fn_offset_32_bit_f32(const char *s1, const char *s2, const char *s3, const char *s4, const char *s5, const char *s6, const char *s7, const char *s8, const char *s9, const char *s10, const char *s11, const char *s12, const char *s13, const char *s14, const char *s15, float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, int32_t g);
void game_fn_offset_32_bit_i32(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19, float f20, float f21, float f22, float f23, float f24, float f25, float f26, float f27, float f28, float f29, float f30, int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t g);
void game_fn_offset_32_bit_string(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11, float f12, float f13, float f14, float f15, float f16, float f17, float f18, float f19, float f20, float f21, float f22, float f23, float f24, float f25, float f26, float f27, float f28, float f29, float f30, const char *s1, const char *s2, const char *s3, const char *s4, const char *s5, int32_t g);
void game_fn_talk(const char *message1, const char *message2, const char *message3, const char *message4);
uint64_t game_fn_get_position(uint64_t id);
void game_fn_set_position(uint64_t pos);
void game_fn_cause_game_fn_error(void);
void game_fn_call_on_b_fn(void);
void game_fn_store(uint64_t id);
uint64_t game_fn_retrieve(void);
uint64_t game_fn_box_i32(int32_t n);
