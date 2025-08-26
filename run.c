#include "grug.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <grug_path>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *grug_path = argv[1];

	if (grug_test_regenerate(grug_path)) {
		// For debugging
		// fprintf(stderr, "%s:%d: %s (detected by grug.c:%d)\n", grug_error.path, grug_error.line_number, grug_error.msg, grug_error.grug_c_line_number);

		fprintf(stderr, "%s\n", grug_error.msg);

		exit(EXIT_FAILURE);
	}
}
