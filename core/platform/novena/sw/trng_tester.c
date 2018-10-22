/*
 * trng_tester.c
 * --------------
 * This program sends several commands to the TRNG subsystem
 * in order to verify the avalanche_entropy, rosc_entropy, and csprng cores.
 *
 * Note: This version of the program talks to the FPGA over an EIM bus.
 *
 *
 * Author: Paul Selkirk
 * Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <ctype.h>
#include <signal.h>

#include "cryptech.h"

char *usage = "Usage: %s [-h] [-d] [-q] [-r] [-w] [-n #] tc...\n";

int debug = 0;
int quiet = 0;
int repeat = 0;
int num_words = 10;
int wait_stats = 0;

/* ---------------- startup code ---------------- */

static off_t board_addr_base = 0;
static off_t trng_addr_base, entropy1_addr_base, entropy2_addr_base, csprng_addr_base;

static int init(void)
{
    static int inited = 0;

    if (inited)
        return 0;

    trng_addr_base = tc_core_base("trng");
    entropy1_addr_base = tc_core_base("extnoise");
    entropy2_addr_base = tc_core_base("rosc ent");
    csprng_addr_base = tc_core_base("csprng");

    inited = 1;
    return 0;
}

/* ---------------- sanity test case ---------------- */

int TC0()
{
    uint8_t name0[4]   = NOVENA_BOARD_NAME0;
    uint8_t name1[4]   = NOVENA_BOARD_NAME1;
    uint8_t version[4] = NOVENA_BOARD_VERSION;
    uint8_t t[4];

    if (init() != 0)
        return -1;

    if (!quiet)
        printf("TC0: Reading board type, version, and dummy reg from global registers.\n");

    /* write current time into dummy register, then try to read it back
     * to make sure that we can actually write something into EIM
     */
    (void)time((time_t *)t);
    if (tc_write(board_addr_base + BOARD_ADDR_DUMMY, t, 4) != 0)
        return 1;

    return
	tc_expected(board_addr_base + BOARD_ADDR_NAME0,   name0,   4) ||
        tc_expected(board_addr_base + BOARD_ADDR_NAME1,   name1,   4) ||
        tc_expected(board_addr_base + BOARD_ADDR_VERSION, version, 4) ||
        tc_expected(board_addr_base + BOARD_ADDR_DUMMY,   t, 4);
}

/* ---------------- trng test cases ---------------- */

/* TC1: Read name and version from trng core. */
int TC1(void)
{
    uint8_t name0[4]   = TRNG_NAME0;
    uint8_t name1[4]   = TRNG_NAME1;
    uint8_t version[4] = TRNG_VERSION;

    if (init() != 0)
        return -1;
    if ((trng_addr_base == 0) && !quiet) {
        printf("TC1: TRNG not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC1: Reading name and version words from trng core.\n");

    return
        tc_expected(trng_addr_base + TRNG_ADDR_NAME0, name0, 4) ||
        tc_expected(trng_addr_base + TRNG_ADDR_NAME1, name1, 4) ||
        tc_expected(trng_addr_base + TRNG_ADDR_VERSION, version, 4);
}

/* XXX test cases for setting blinkenlights? */
/* XXX set 'discard' control bit, see if we read the same value */

/* ---------------- avalanche_entropy test cases ---------------- */

/* TC2: Read name and version from avalanche_entropy core. */
int TC2(void)
{
    uint8_t name0[4]   = AVALANCHE_ENTROPY_NAME0;
    uint8_t name1[4]   = AVALANCHE_ENTROPY_NAME1;
    uint8_t version[4] = AVALANCHE_ENTROPY_VERSION;

    if (init() != 0)
        return -1;
    if ((entropy1_addr_base == 0) && !quiet) {
        printf("TC2: AVALANCHE_ENTROPY not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC2: Reading name and version words from avalanche_entropy core.\n");

    return
        tc_expected(entropy1_addr_base + ENTROPY1_ADDR_NAME0, name0, 4) ||
        tc_expected(entropy1_addr_base + ENTROPY1_ADDR_NAME1, name1, 4) ||
        tc_expected(entropy1_addr_base + ENTROPY1_ADDR_VERSION, version, 4);
}

/* XXX clear 'enable' control bit, see if we read the same value */

/* TC3: Read random data from avalanche_entropy. */
int TC3(void)
{
    int i, n;
    uint32_t entropy;

    if (init() != 0)
        return -1;
    if ((entropy1_addr_base == 0) && !quiet) {
        printf("TC3: AVALANCHE_ENTROPY not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC3: Read random data from avalanche_entropy.\n");

    for (i = 0; i < num_words; ++i) {
        /* check status */
        n = 0;
        if (tc_wait(entropy1_addr_base + ENTROPY1_ADDR_STATUS, ENTROPY1_STATUS_VALID, &n) != 0)
            return 1;
        /* read entropy data */
        if (tc_read(entropy1_addr_base + ENTROPY1_ADDR_ENTROPY, (uint8_t *)&entropy, 4) != 0)
            return 1;
        /* display entropy data */
        if (!debug) {
            if (wait_stats)
                printf("%08x %d\n", entropy, n);
            else
                printf("%08x\n", entropy);
        }
    }

    return 0;
}

/* ---------------- rosc_entropy test cases ---------------- */

/* TC4: Read name and version from rosc_entropy core. */
int TC4(void)
{
    uint8_t name0[4]   = ROSC_ENTROPY_NAME0;
    uint8_t name1[4]   = ROSC_ENTROPY_NAME1;
    uint8_t version[4] = ROSC_ENTROPY_VERSION;

    if (init() != 0)
        return -1;
    if ((entropy2_addr_base == 0) && !quiet) {
        printf("TC4: ROSC_ENTROPY not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC4: Reading name and version words from rosc_entropy core.\n");

    return
        tc_expected(entropy2_addr_base + ENTROPY2_ADDR_NAME0, name0, 4) ||
        tc_expected(entropy2_addr_base + ENTROPY2_ADDR_NAME1, name1, 4) ||
        tc_expected(entropy2_addr_base + ENTROPY2_ADDR_VERSION, version, 4);
}

/* XXX clear 'enable' control bit, see if we read the same value */

/* TC5: Read random data from rosc_entropy. */
int TC5(void)
{
    int i, n;
    uint32_t entropy;

    if (init() != 0)
        return -1;
    if ((entropy2_addr_base == 0) && !quiet) {
        printf("TC5: ROSC_ENTROPY not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC5: Read random data from rosc_entropy.\n");

    for (i = 0; i < num_words; ++i) {
        /* check status */
        n = 0;
        if (tc_wait(entropy2_addr_base + ENTROPY2_ADDR_STATUS, ENTROPY2_STATUS_VALID, &n) != 0)
            return 1;
        /* read entropy data */
        if (tc_read(entropy2_addr_base + ENTROPY2_ADDR_ENTROPY, (uint8_t *)&entropy, 4) != 0)
            return 1;
        /* display entropy data */
        if (!debug) {
            if (wait_stats)
                printf("%08x %d\n", entropy, n);
            else
                printf("%08x\n", entropy);
        }
    }

    return 0;
}

/* ---------------- trng_csprng test cases ---------------- */

/* TC6: Read name and version from trng_csprng core. */
int TC6(void)
{
    uint8_t name0[4]   = CSPRNG_NAME0;
    uint8_t name1[4]   = CSPRNG_NAME1;
    uint8_t version[4] = CSPRNG_VERSION;

    if (init() != 0)
        return -1;
    if ((csprng_addr_base == 0) && !quiet) {
        printf("TC6: CSPRNG not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC6: Reading name and version words from trng_csprng core.\n");

    return
        tc_expected(csprng_addr_base + CSPRNG_ADDR_NAME0, name0, 4) ||
        tc_expected(csprng_addr_base + CSPRNG_ADDR_NAME1, name1, 4) ||
        tc_expected(csprng_addr_base + CSPRNG_ADDR_VERSION, version, 4);
}

/* XXX clear 'enable' control bit, see if we read the same value */
/* XXX set 'seed' control bit, see if we read the same value */

/* TC7: Read random data from trng_csprng. */
int TC7(void)
{
    int i, n;
    uint32_t random;

    if (init() != 0)
        return -1;
    if ((csprng_addr_base == 0) && !quiet) {
        printf("TC7: CSPRNG not present\n");
        return 0;
    }

    if (!quiet)
        printf("TC7: Read random data from trng_csprng.\n");

    for (i = 0; i < num_words; ++i) {
        /* check status */
        n = 0;
        if (tc_wait(csprng_addr_base + CSPRNG_ADDR_STATUS, CSPRNG_STATUS_VALID, &n) != 0)
            return 1;
        /* read random data */
        if (tc_read(csprng_addr_base + CSPRNG_ADDR_RANDOM, (uint8_t *)&random, 4) != 0)
            return 1;
        /* display random data */
        if (!debug) {
            if (wait_stats)
                printf("%08x %d\n", random, n);
            else
                printf("%08x\n", random);
        }
    }

    return 0;
}


/* ---------------- main ---------------- */

/* signal handler for ctrl-c to end repeat testing */
unsigned long iter = 0;
struct timeval tv_start, tv_end;
void sighandler(int unused)
{
    double tv_diff;

    gettimeofday(&tv_end, NULL);
    tv_diff = (double)(tv_end.tv_sec - tv_start.tv_sec) +
        (double)(tv_end.tv_usec - tv_start.tv_usec)/1000000;
    printf("\n%lu iterations in %.3f seconds (%.3f iterations/sec)\n",
           iter, tv_diff, (double)iter/tv_diff);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    typedef int (*tcfp)(void);
    tcfp all_tests[] = { TC0, TC1, TC2, TC3, TC4, TC5, TC6, TC7 };
    int i, j, opt;

    while ((opt = getopt(argc, argv, "h?dqrn:w")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            printf(usage, argv[0]);
            return EXIT_SUCCESS;
        case 'd':
            tc_set_debug(1);
            debug = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        case 'r':
            repeat = 1;
            break;
        case 'n':
            num_words = atoi(optarg);
            if (num_words <= 0) {
                fprintf(stderr, "-n requires a positive integer argument\n");
                return EXIT_FAILURE;
            }
            break;
        case 'w':
            wait_stats = 1;
            break;
        default:
            fprintf(stderr, usage, argv[0]);
            return EXIT_FAILURE;
        }
    }

    /* repeat one test until interrupted */
    if (repeat) {
        tcfp tc;
        if (optind != argc - 1) {
            fprintf(stderr, "only one test case can be repeated\n");
            return EXIT_FAILURE;
        }
        j = atoi(argv[optind]);
        if (j < 0 || j >= sizeof(all_tests)/sizeof(all_tests[0])) {
            fprintf(stderr, "invalid test number %s\n", argv[optind]);
            return EXIT_FAILURE;
        }
        tc = (all_tests[j]);
        srand(time(NULL));
        signal(SIGINT, sighandler);
        gettimeofday(&tv_start, NULL);
        while (1) {
            ++iter;
            if ((iter & 0xffff) == 0) {
                printf(".");
                fflush(stdout);
            }
            if (tc() != 0)
                sighandler(0);
        }
        return EXIT_SUCCESS;    /*NOTREACHED*/
    }

    /* no args == run all tests */
    if (optind >= argc) {
        for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
            if (all_tests[j]() != 0)
                return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    /* run one or more tests (by number) or groups of tests (by name) */
    for (i = optind; i < argc; ++i) {
        if (strcmp(argv[i], "all") == 0) {
            for (j = 0; j < sizeof(all_tests)/sizeof(all_tests[0]); ++j)
                if (all_tests[j]() != 0)
                    return EXIT_FAILURE;
        }
        else if (isdigit(argv[i][0]) &&
                 (((j = atoi(argv[i])) >= 0) &&
                  (j < sizeof(all_tests)/sizeof(all_tests[0])))) {
            if (all_tests[j]() != 0)
                return EXIT_FAILURE;
        }
        else {
            fprintf(stderr, "unknown test case %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
