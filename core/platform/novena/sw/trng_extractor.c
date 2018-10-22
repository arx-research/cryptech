/*
 * trng_extractor.c
 * ----------------
 * This program extracts raw data from the avalanche_entropy, rosc_entropy,
 * and csprng cores.
 *
 * Author: Paul Selkirk
 * Copyright (c) 2015, NORDUnet A/S All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>

#include "cryptech.h"

char *usage =
"%s [-a|r|c] [-n #] [-o file]\n\
\n\
-a      avalanche entropy\n\
-r      rosc entropy\n\
-c      csprng (default data source)\n\
-n      number of 4-byte samples (scale with K, M, or G suffix)\n\
-o      output file (defaults to stdout)\n\
-v      verbose operation\n\
";

/* ---------------- startup code ---------------- */

static off_t entropy1_addr_base, entropy2_addr_base, csprng_addr_base;

static void init(void)
{
    entropy1_addr_base = tc_core_base("extnoise");
    entropy2_addr_base = tc_core_base("rosc ent");
    csprng_addr_base = tc_core_base("csprng");
}

/* ---------------- extract one data sample ---------------- */
static int extract(off_t status_addr, off_t data_addr, uint32_t *data)
{
    if (tc_wait(status_addr, ENTROPY1_STATUS_VALID, NULL) != 0) {
        fprintf(stderr, "tc_wait failed\n");
        return 1;
    }

    if (tc_read(data_addr, (uint8_t *)data, 4) != 0) {
        fprintf(stderr, "tc_read failed\n");
        return 1;
    }

    return 0;
}

/* ---------------- main ---------------- */
int main(int argc, char *argv[])
{
    int opt;
    unsigned long num_words = 1, i;
    char *endptr;
    off_t status_addr = 0;
    off_t data_addr = 0;
    FILE *output = stdout;
    uint32_t data;
    int verbose = 0;

    init();

    /* parse command line */
    while ((opt = getopt(argc, argv, "h?varcn:o:")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            printf(usage, argv[0]);
            return EXIT_SUCCESS;
        case 'a':
            status_addr = entropy1_addr_base + ENTROPY1_ADDR_STATUS;
            data_addr = entropy1_addr_base + ENTROPY1_ADDR_ENTROPY;
            break;
        case 'r':
            status_addr = entropy2_addr_base + ENTROPY2_ADDR_STATUS;
            data_addr = entropy2_addr_base + ENTROPY2_ADDR_ENTROPY;
            break;
        case 'c':
            status_addr = csprng_addr_base + CSPRNG_ADDR_STATUS;
            data_addr = csprng_addr_base + CSPRNG_ADDR_RANDOM;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'n':
            num_words = strtoul(optarg, &endptr, 10);
            switch (toupper(*endptr)) {
            case '\0':
                break;
            case 'K':
                num_words *= 1000;
                break;
            case 'M':
                num_words *= 1000000;
                break;
            case 'G':
                num_words *= 1000000000;
                break;
            default:
                fprintf(stderr, "unsupported -n suffix %s\n", endptr);
                return EXIT_FAILURE;
            }
            break;
        case 'o':
            output = fopen(optarg, "wb+");
            if (output == NULL) {
                fprintf(stderr, "error opening output file %s: ", optarg);
                perror("");
                return EXIT_FAILURE;
            }
            break;
        default:
        errout:
            fprintf(stderr, usage, argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "%s: invalid argument%s --",
                argv[0], argc - optind > 1 ? "s" : "");
        do {
            fprintf(stderr, " %s", argv[optind]);
            ++optind;
        } while (optind < argc);
        fprintf(stderr, "\n");
        goto errout;
    }

    if (status_addr == 0) {
	status_addr = csprng_addr_base + CSPRNG_ADDR_STATUS;
	data_addr = csprng_addr_base + CSPRNG_ADDR_RANDOM;
    }

    /* get the data */
    for (i = 0; i < num_words; ++i) {
        if (extract(status_addr, data_addr, &data) != 0)
            return EXIT_FAILURE;
        if (fwrite(&data, sizeof(data), 1, output) != 1) {
            perror("fwrite");
            fclose(output);
            return EXIT_FAILURE;
        }
        if (verbose && ((i & 0xffff) == 0)) {
	    fprintf(stderr, ".");
	    fflush(stderr);
	}
    }

    fclose(output);
    return EXIT_SUCCESS;
}
