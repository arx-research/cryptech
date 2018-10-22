#include <stdio.h>
#include <stdlib.h>

#include "novena-eim.h"

int
main(int argc, char *argv[])
{
    off_t offset;
    uint32_t value, result;

    if (argc < 3 || argc > 4) {
    usage:
	fprintf(stderr, "usage: %s offset r\n", argv[0]);
	fprintf(stderr, "usage: %s offset w value\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    if (eim_setup() != 0) {
        fprintf(stderr, "EIM setup failed\n");
	exit(EXIT_FAILURE);
    }

    offset = (off_t)strtoul(argv[1], NULL, 0);
    if (argv[2][0] == 'r') {
	eim_read_32(offset, &result);
	printf("%08x\n", result);
    }
    else if (argv[2][0] == 'w') {
	value = strtoul(argv[3], NULL, 0);
	eim_write_32(offset, &value);
    }
    else {
	fprintf(stderr, "unknown command '%s'\n", argv[2]);
	goto usage;
    }

    exit(EXIT_SUCCESS);
}

