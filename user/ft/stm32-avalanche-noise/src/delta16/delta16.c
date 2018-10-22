/* -*- mode:C; c-file-style: "bsd" -*- */
/*
 *
 *  Copyright (c) 2014 NORDUnet A/S
 *  All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or
 *    without modification, are permitted provided that the following
 *    conditions are met:
 *
 *      1. Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *      2. Redistributions in binary form must reproduce the above
 *         copyright notice, this list of conditions and the following
 *         disclaimer in the documentation and/or other materials provided
 *         with the distribution.
 *      3. Neither the name of the NORDUnet nor the names of its
 *         contributors may be used to endorse or promote products derived
 *         from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Author: Fredrik Thulin <fredrik@thulin.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* XXX should runtime check if this is a little-endian system already */
#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

#define INPUT_BLOCK_SIZE	1024
#define SYNC_BYTE		0xf0
#define NUM_SYNC_BYTES		16

enum {
	mode_random = 0,
	mode_random_VN,
	mode_deltas,
};

const char *usage =
	"This program parses the output of MODE_DELTAS in the STM32 code.\n"
	"\n"
        "Usage: delta16 [options] [infile] [outfile]\n"
	"\n"
	"Options:\n"
	"  -V  von Neumann de-biased random bytes output\n"
	"  -D  counter delta output\n"
	"  -v  verbose\n"
	"  -d  debug\n"
	"  -f  force overwriting of existing output file\n"
	""
	"\n"
	;
const char *optstring = "VDvfd";

int
parse_args(int argc, char **argv,
	   int *mode,
	   char **infile,
	   char **outfile,
	   int *verbose,
	   int *force,
	   int *debug,
	   int *exit_code)
{
        int c, V = 0, D = 0;

        while((c = getopt(argc, argv, optstring)) != -1) {
                switch (c) {
                case 'V':
                        V = 1;
                        break;
                case 'D':
                        D = 1;
                        break;
                case 'v':
                        *verbose = 1;
                        break;
                case 'f':
                        *force = 1;
                        break;
                case 'd':
                        *debug = 1;
                        break;

                default:
                        fputs(usage, stderr);
                        *exit_code = 0;
                        return 0;
                }
        }

	*mode = mode_random;

	if (V && D) {
		fprintf(stderr, "Options -V and -D can't be combined\n");
		return 0;
	}
	if (V) {
		*mode = mode_random_VN;
	}
	if (D) {
		*mode = mode_deltas;
	}

	*infile = "-";
	*outfile = "-";

        if (optind < argc) {
		*infile = argv[optind++];
        }

        if (optind < argc) {
		*outfile = argv[optind++];
        }

	return 1;
}

int
find_input_sync_position(FILE *in, const int verbose)
{
	int pos = 0;
	int match = 0;
	uint8_t ch;

	while (pos++ < 4096) {
		if (! fread(&ch, 1, 1, in)) {
			return 0;
		}
		if (ch == SYNC_BYTE) {
			match++;
			if (match == NUM_SYNC_BYTES) {
				return pos;
			}
		} else {
			match = 0;
		}
	}

	return -1;
}

int
fill_input_buffer(FILE *in, uint8_t *buf, size_t bufsize, int verbose, unsigned long *skipped_bytes)
{
	int i;
	unsigned long pos = 0, old_pos = 0;

	old_pos = ftell(in);

	if ((i = fread(buf, 1, bufsize, in)) != INPUT_BLOCK_SIZE) {
		if (verbose) {
			fprintf(stderr, "Short read, %d bytes, at position %ld / %lx\n", i, old_pos, old_pos);
		}
		return 2;
	}
	/* Expect next sync marker to immediately follow this one */
	if ((i = find_input_sync_position(in, verbose)) > NUM_SYNC_BYTES) {
		/*
		  fprintf(stderr, "SYNC: buf[0] from pos %ld / %lx was 0x%2x - sync found at + %d (now at %ld / %lx)\n",
		  old_pos, old_pos, buf[0], i, ftell(in), ftell(in));
		*/
		if (i < 0) {
			fprintf(stderr, "Sync completely lost from position %ld / %lx\n", old_pos, old_pos);
			return 0;
		}
		if (fseek(in, 0 - INPUT_BLOCK_SIZE, SEEK_CUR) < 0) {
			fprintf(stderr, "Failed seeking backwards when looking for sync from position %ld / %lx\n", old_pos, old_pos);
			return 0;
		}
		i = find_input_sync_position(in, verbose);
		pos = ftell(in);
		if (i < INPUT_BLOCK_SIZE - 1) {
			fprintf(stderr, "Short block (%d bytes, < %d) at position %ld / %lx. Resynced at %ld / 0x%lx.\n",
				i, INPUT_BLOCK_SIZE, old_pos, old_pos, pos, pos);
		} else {
			if (verbose) {
				fprintf(stderr, "Sync lost, skipping %ld bytes from position %ld / 0x%lx. Resynced at %ld / 0x%lx.\n",
					pos - old_pos,
					old_pos, old_pos,
					pos, pos);
			}
			*skipped_bytes += pos - old_pos;
			return -1;
		}
	}

	return 1;
}

inline void
output_deltas(FILE *out, uint32_t raw, unsigned long *write_count, int debug)
{
	uint32_t first, second;
	static uint32_t last = 0, num = 0;

	first  = (raw & 0xffff0000) >> 16;
	second = (raw & 0x0000ffff);

	if (last > first) {
		/* Next batch of timer values start here */
		last = 0;
		num = 0;
	}

	/* Need to skip the first output of each batch - it's delta value is unknown */
	if (num) {
		fprintf(out, "%02d %04x %02x\n", num++, first, first - last);
		*write_count += 1;
	} else {
		num++;
	}
	fprintf(out, "%02d %04x %02x\n", num++, second, second - first);
	*write_count += 1;

	last = second;
}

inline void
output_random(FILE *out, uint32_t raw, unsigned long *write_count, int debug)
{
	uint32_t first, second;
	static uint32_t num = 0, bits = 0, old_bits;

	first  = (raw & 0xffff0000) >> 16;
	second = (raw & 0x0000ffff);

	old_bits = bits;

	bits <<= 1; bits += (first & 1);
	bits <<= 1; bits += (second & 1);

	num += 2;

	if (debug) {
		fprintf(stderr, "raw 0x%08x -> first 0x%04x second 0x%04x        bits 0x%x |= %d |= %d -> 0x%x (%d bits)\n",
			raw, first, second, old_bits, (first & 1), (second & 1), bits, num);
	}

	if (num == 32) {
		if (debug) {
			fprintf(stderr, "write %02x %02x %02x %02x\n",
				(bits & 0xff000000) >> 24,
				(bits & 0xff0000) >> 16,
				(bits & 0xff00) >> 8,
				bits & 0xff
				);
		}
		fprintf(out, "%c%c%c%c",
			(bits & 0xff000000) >> 24,
			(bits & 0xff0000) >> 16,
			(bits & 0xff00) >> 8,
			bits & 0xff
			);
		*write_count += 4;
		bits = 0;
		num = 0;
	}
}

inline void
output_random_VN(FILE *out, uint32_t raw, unsigned long *write_count, int debug)
{
	uint32_t first, second;
	static uint32_t num = 0, bits = 0, old_bits;

	first  = (raw & 0xffff0000) >> 16;
	second = (raw & 0x0000ffff);

	/* Discard both values unless their LSB is actually different.
	   This is the Von Neumann extractor. */
	if ((first & 1) != (second & 1)) {
		old_bits = bits;

		bits <<= 1; bits += (first & 1);
		num++;

		if (debug) {
			fprintf(stderr, "raw 0x%08x -> first 0x%04x second 0x%04x        bits 0x%x |= %d (skip second %d)-> 0x%x (%d bits)\n",
				raw, first, second, old_bits, (first & 1), (second & 1), bits, num);
		}
	} else if (debug) {
		fprintf(stderr, "raw 0x%08x -> first 0x%04x second 0x%04x\n",
			raw, first, second);
	}

	if (num == 32) {
		fprintf(out, "%c%c%c%c",
			(bits & 0xff000000) >> 24,
			(bits & 0xff0000) >> 16,
			(bits & 0xff00) >> 8,
			bits & 0xff
			);
		*write_count += 4;
		num = 0;
		bits = 0;
	}
}

int
process_data(FILE *in, FILE *out,
	     const int verbose,
	     const int debug,
	     const int mode,
	     unsigned long *skipped_bytes
	)
{
	unsigned long blocks = 0, skipped_blocks = 0, write_count = 0;
	uint32_t i, raw = 0;
	uint32_t *buf32, *buf32_end;
	uint8_t buf[INPUT_BLOCK_SIZE];

	if (skipped_bytes) {
		skipped_blocks = 1;
	}

	while (1) {
		if ((i = fill_input_buffer(in, buf, sizeof(buf), verbose, skipped_bytes)) != 1) {
			if (i == -1) {
				skipped_blocks += 1;
				continue;
			}
			if (i == 2) {
				/* short read - EOF */
				unsigned long counters = (blocks * INPUT_BLOCK_SIZE / 2);

				fprintf(stderr, "------ delta16 ------\n");
				fprintf(stderr, "Skipped %ld blocks (%ld bytes total) because of loss of sync.\n",
					skipped_blocks, *skipped_bytes);

				fprintf(stderr, "Processed %ld * %d = %ld bytes (%ld MB). %ld counter values.\n",
					blocks, INPUT_BLOCK_SIZE,
					blocks * INPUT_BLOCK_SIZE,
					blocks * INPUT_BLOCK_SIZE / 1024 / 1024,
					counters
					);

				if (mode != mode_deltas) {
					fprintf(stderr, "Output count is %ld bytes (%ld MB), %.3f counters per output value.\n",
						write_count,
						write_count / 1024 / 1024,
						counters / (double) write_count);
				}

				fprintf(stderr, "------ delta16 ------\n\n");
				return 1;
			}
		}
		buf32 = (uint32_t *) buf;
		buf32_end = &buf32[INPUT_BLOCK_SIZE / sizeof(buf32[0])];
		while (buf32 < buf32_end) {
			/* fprintf(stderr, "Read %d @%p\n", i, &buf32); */
			raw = *buf32++;
			switch (mode)
			{
			case mode_deltas:
				output_deltas(out, raw, &write_count, debug);
				break;;
			case mode_random:
				output_random(out, raw, &write_count, debug);
				break;;
			case mode_random_VN:
				output_random_VN(out, raw, &write_count, debug);
				break;;
			default:
				fprintf(stderr, "Mode %d output not implemented, aborting\n", mode);
				return 0;
			}
		}

		blocks++;
		if (verbose && ! (blocks % 100000) && blocks) {
			fprintf(stderr, "Processed %ld blocks (file position %ld / 0x%lx / %ld MB)\n",
				blocks, ftell(in), ftell(in), ftell(in) / 1024 / 1024);
		}
	};

	/* NOT REACHED */
	return 0;
}

int
main(int argc, char **argv)
{
	int mode, debug = 0, verbose = 0, force = 0, exit_code = 1;
	unsigned long skipped_bytes = 0;
	char *infile, *outfile;
	FILE *in, *out;

	if (! parse_args(argc, argv,
			 &mode,
			 &infile,
			 &outfile,
			 &verbose,
			 &force,
			 &debug,
			 &exit_code
		    ))
		goto err;

	if (! strcmp(infile, "-")) {
		in = stdin;
	} else {
		if (verbose) {
			fprintf(stderr, "Opening input file '%s' for reading\n", infile);
		}
		if (! (in = fopen(infile, "rb"))) {
			fprintf(stderr, "Failed opening input file '%s' for reading: %s\n", infile, strerror(errno));
			goto err;
		}
	}

	if ((skipped_bytes = find_input_sync_position(in, verbose)) < 0) {
		fprintf(stderr, "Could not find sync sequence in input\n");
		goto err;
	}
	if (verbose) {
		fprintf(stderr, "Initial sync found at position %ld / 0x%lx\n", skipped_bytes, skipped_bytes);
	}

	if (! strcmp(outfile, "-")) {
		out = stdout;
	} else {
		if (! force) {
			/* Check that file does not already exist */
			if (access(outfile, F_OK) == F_OK) {
				fprintf(stderr, "Refusing to overwrite existing output file without -f (force)\n");
				goto err;
			}
		}
		if (verbose) {
			fprintf(stderr, "Opening output file '%s' for writing\n", outfile);
		}
		if (! (out = fopen(outfile, "wb"))) {
			fprintf(stderr, "Failed opening output file '%s' for writing: %s\n", outfile, strerror(errno));
			goto err;
		}
	}

	if (! process_data(in, out, verbose, debug, mode, &skipped_bytes)) {
		fprintf(stderr, "Failed processing data\n");
		goto err;
	}

	exit_code = 0;
err:
	exit(exit_code);
}
