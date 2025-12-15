#include "grug.h"

#include "mod_api.h"

#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void game_fn_nothing(void) {}
int32_t game_fn_magic(void) { return 0; }
void game_fn_initialize(const union grug_value args[]) {}
void game_fn_initialize_bool(const union grug_value args[]) {}
int32_t game_fn_identity(const union grug_value args[]) { return 0; }
int32_t game_fn_max(const union grug_value args[]) { return 0; }
void game_fn_say(const union grug_value args[]) {}
float game_fn_sin(const union grug_value args[]) { return 0.0f; }
float game_fn_cos(const union grug_value args[]) { return 0.0f; }
void game_fn_mega(const union grug_value args[]) {}
int game_fn_get_evil_false(void) { return 0xff00; }
void game_fn_set_is_happy(const union grug_value args[]) {}
void game_fn_mega_f32(const union grug_value args[]) {}
void game_fn_mega_i32(const union grug_value args[]) {}
void game_fn_draw(const union grug_value args[]) {}
void game_fn_blocked_alrm(void) {}
void game_fn_spawn(const union grug_value args[]) {}
bool game_fn_has_resource(const union grug_value args[]) { return false; }
bool game_fn_has_entity(const union grug_value args[]) { return false; }
bool game_fn_has_string(const union grug_value args[]) { return false; }
uint64_t game_fn_get_opponent(void) { return 0; }
void game_fn_set_d(const union grug_value args[]) {}
void game_fn_set_opponent(const union grug_value args[]) {}
void game_fn_motherload(const union grug_value args[]) {}
void game_fn_motherload_subless(const union grug_value args[]) {}
void game_fn_offset_32_bit_f32(const union grug_value args[]) {}
void game_fn_offset_32_bit_i32(const union grug_value args[]) {}
void game_fn_offset_32_bit_string(const union grug_value args[]) {}
void game_fn_talk(const union grug_value args[]) {}
uint64_t game_fn_get_position(const union grug_value args[]) { return 0; }
void game_fn_set_position(const union grug_value args[]) {}
void game_fn_cause_game_fn_error(void) {}
void game_fn_call_on_b_fn(void) {}
void game_fn_store(const union grug_value args[]) {}
uint64_t game_fn_retrieve(void) { return 0; }
uint64_t game_fn_box_i32(const union grug_value args[]) { return 0; }

// Source: https://github.com/google/security-research-pocs/blob/d10780c3ddb8070dff6c5e5862c93c01392d1727/autofuzz/fuzz_utils.cc#L10
int ignore_stdout(void) {
	int fd = open("/dev/null", O_WRONLY);
	if (fd == -1) {
		warn("open(\"/dev/null\") failed");
		return -1;
	}

	int ret = 0;
	if (dup2(fd, STDOUT_FILENO) == -1) {
		warn("failed to redirect stdout to /dev/null\n");
		ret = -1;
	}

	if (close(fd) == -1) {
		warn("close");
		ret = -1;
	}

	return ret;
}

// Source: https://github.com/google/security-research-pocs/blob/d10780c3ddb8070dff6c5e5862c93c01392d1727/autofuzz/fuzz_utils.cc#L31
int delete_file(const char *pathname) {
	int ret = unlink(pathname);
	if (ret == -1) {
		warn("failed to delete \"%s\"", pathname);
	}

	return ret;
}

// Source: https://github.com/google/security-research-pocs/blob/d10780c3ddb8070dff6c5e5862c93c01392d1727/autofuzz/fuzz_utils.cc#L42
const char *buf_to_file(const uint8_t *buf, size_t size) {
	// We want "/dev/shm/fuzz_XXXXXX"
	static char p[] = "/dev/shm/fuzz_XXXXXX";
	// static char p[] = "./lol/fuzz_XXXXXX";
	static char suffix[] = "-D.grug";
	static char pathname[sizeof(p) + sizeof(suffix) - 1];
	memcpy(pathname, p, sizeof(p));

	// We want "/dev/shm/fuzz_123456"
	int fd = mkstemp(pathname);
	if (fd == -1) {
		warn("mkstemp(\"%s\")", pathname);
		return NULL;
	}

	static char old[sizeof(pathname)];
	memcpy(old, pathname, sizeof(pathname));

	// We want "/dev/shm/fuzz_123456-D.grug"
	memcpy(pathname + sizeof(p) - 1, suffix, sizeof(suffix));
	// fprintf(stderr, "pathname: %s\n", pathname);
	// exit(EXIT_FAILURE);

	// We need to rename "/dev/shm/fuzz_123456" to "/dev/shm/fuzz_123456-D.grug"
	const char *new = pathname;
	if (rename(old, new) == -1) {
		warn("rename(\"%s\", \"%s\")", old, new);
		return NULL;
	}

	size_t pos = 0;
	while (pos < size) {
		int nbytes = write(fd, &buf[pos], size - pos);
		if (nbytes <= 0) {
		if (nbytes == -1 && errno == EINTR) {
			continue;
		}
		warn("write");
		goto err;
		}
		pos += nbytes;
	}

	if (close(fd) == -1) {
		warn("close");
		goto err;
	}

	return pathname;

err:
	delete_file(pathname);
	return NULL;
}

static void *get(void *handle, const char *label) {
	void *p = dlsym(handle, label);
	if (!p) {
		fprintf(stderr, "dlsym: %s\n", dlerror());
		exit(EXIT_FAILURE);
	}
	return p;
}

static void runtime_error_handler(const char *reason, enum grug_runtime_error_type type, const char *on_fn_name, const char *on_fn_path) {}

// Source: https://github.com/google/security-research-pocs/blob/649b6ed74c842f533d15410f13d94aada96375ef/autofuzz/alembic_fuzzer.cc#L293
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	static bool initialized = false;

	if (!initialized) {
    	ignore_stdout();

		if (grug_init(runtime_error_handler, "mod_api.json", "fuzzing", 10, NULL)) {
			fprintf(stderr, "grug_init() error: %s (detected by grug.c:%d)\n", grug_error.msg, grug_error.grug_c_line_number);
			exit(EXIT_FAILURE);
		}

		initialized = true;
	}

	const char *grug_path = buf_to_file(data, size);
	if (grug_path == NULL) {
		exit(EXIT_FAILURE);
	}

	// If there wasn't an error generating the dll
	if (!grug_test_regenerate(grug_path, "fuzzing")) {
		void *handle = dlopen(dll_path, RTLD_NOW);
		if (!handle) {
			fprintf(stderr, "dlopen: %s\n", dlerror());
			exit(EXIT_FAILURE);
		}

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"

		size_t globals_size = *(size_t *)get(handle, "globals_size");

		void *g = malloc(globals_size);
		grug_init_globals_fn_t init_globals = get(handle, "init_globals");
		init_globals(g, 42);

		#pragma GCC diagnostic pop

		struct d_on_fns *on_fns = dlsym(handle, "on_fns");
		if (on_fns && on_fns->a) {
			on_fns->a(g);
		}

		free(g);

		if (dlclose(handle)) {
			fprintf(stderr, "dlclose: %s\n", dlerror());
			exit(EXIT_FAILURE);
		}
	}

	// fprintf(stderr, "grug_test_regenerate() error: %s\n", grug_error.msg);
	// exit(EXIT_FAILURE);

	if (delete_file(grug_path) != 0) {
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
