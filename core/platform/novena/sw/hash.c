/*
 * hash.c
 * ------
 * This program uses the coretest_hashes subsystem to produce a
 * cryptographic hash of a file or input stream. It is a generalization
 * of the hash_tester.c test program.
 *
 * Authors: Joachim Strömbergson, Paul Selkirk
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
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

#include "cryptech.h"

char *usage =
"Usage: %s [-d] [-v] [-q] [algorithm [file]]\n"
"algorithms: sha-1, sha-256, sha-512/224, sha-512/256, sha-384, sha-512\n";

int quiet = 0;
int verbose = 0;


/* ---------------- algorithm lookup code ---------------- */

struct ctrl {
    char *name;
    off_t base_addr;
    off_t block_addr;
    int   block_len;
    off_t digest_addr;
    int   digest_len;
    int   mode;
} ctrl[] = {
    { "sha-1",       0, SHA1_ADDR_BLOCK, SHA1_BLOCK_LEN,
                     SHA1_ADDR_DIGEST, SHA1_DIGEST_LEN, 0 },
    { "sha-256",     0, SHA256_ADDR_BLOCK, SHA256_BLOCK_LEN,
                     SHA256_ADDR_DIGEST, SHA256_DIGEST_LEN, 0 },
    { "sha-512/224", 0, SHA512_ADDR_BLOCK, SHA512_BLOCK_LEN,
                     SHA512_ADDR_DIGEST, SHA512_224_DIGEST_LEN, MODE_SHA_512_224 },
    { "sha-512/256", 0, SHA512_ADDR_BLOCK, SHA512_BLOCK_LEN,
                     SHA512_ADDR_DIGEST, SHA512_256_DIGEST_LEN, MODE_SHA_512_256 },
    { "sha-384",     0, SHA512_ADDR_BLOCK, SHA512_BLOCK_LEN,
                     SHA512_ADDR_DIGEST, SHA384_DIGEST_LEN, MODE_SHA_384 },
    { "sha-512",     0, SHA512_ADDR_BLOCK, SHA512_BLOCK_LEN,
                     SHA512_ADDR_DIGEST, SHA512_DIGEST_LEN, MODE_SHA_512 },
    { NULL, 0, 0, 0 }
};

/* return the control structure for the given algorithm */
struct ctrl *find_algo(char *algo)
{
    int i;

    for (i = 0; ctrl[i].name != NULL; ++i)
        if (strcmp(ctrl[i].name, algo) == 0)
            return &ctrl[i];

    fprintf(stderr, "algorithm \"%s\" not found\n\n", algo);
    fprintf(stderr, usage, "hash");
    return NULL;
}

/* ---------------- startup code ---------------- */

static int patch(char *name, off_t base_addr) {
    struct ctrl *ctrl;

    ctrl = find_algo(name);
    if (ctrl == NULL)
	return -1;

    ctrl->base_addr = base_addr;
    return 0;
}

static int inited = 0;

static int init(void)
{
    struct core_info *core;

    if (inited)
        return 0;

    for (core = tc_core_first("sha"); core; core = tc_core_next(core, "sha")) {
        if (strncmp(core->name, SHA1_NAME0 SHA1_NAME1, 8) == 0)
            patch("sha-1", core->base);
        else if (strncmp(core->name, SHA256_NAME0 SHA256_NAME1, 8) == 0)
            patch("sha-256", core->base);
        else if (strncmp(core->name, SHA512_NAME0 SHA512_NAME1, 8) == 0) {
            patch("sha-512/224", core->base);
            patch("sha-512/256", core->base);
            patch("sha-384", core->base);
            patch("sha-512", core->base);
	}
    }

    inited = 1;
    return 0;
}

/* ---------------- hash ---------------- */

static int transmit(off_t base, uint8_t *block, int blen, int mode, int first)
{
    uint8_t ctrl_cmd[4] = { 0 };
    int limit = 10;

    if (tc_write(base + ADDR_BLOCK, block, blen) != 0)
        return 1;

    ctrl_cmd[3] = (first ? CTRL_INIT : CTRL_NEXT) | mode;

    return
	tc_write(base + ADDR_CTRL, ctrl_cmd, 4) ||
	tc_wait(base + ADDR_STATUS, STATUS_READY, &limit);
}

static int pad_transmit(off_t base, uint8_t *block, uint8_t flen, uint8_t blen,
			uint8_t mode, long long tlen, int first)
{
    assert(flen < blen);

    block[flen++] = 0x80;
    memset(block + flen, 0, blen - flen);

    if (blen - flen < ((blen == 64) ? 8 : 16)) {
        if (transmit(base, block, blen, mode, first) != 0)
            return 1;
        first = 0;
        memset(block, 0, blen);
    }

    /* properly the length is 128 bits for sha-512, but we can't
     * actually count above 64 bits
     */
    ((uint32_t *)block)[blen/4 - 2] = htonl((tlen >> 32) & 0xffff);
    ((uint32_t *)block)[blen/4 - 1] = htonl(tlen & 0xffff);

    return transmit(base, block, blen, mode, first);
}

/* return number of digest bytes read */
static int hash(char *algo, char *file, uint8_t *digest)
{
    uint8_t block[SHA512_BLOCK_LEN];
    struct ctrl *ctrl;
    int in_fd = 0;      /* stdin */
    off_t base, daddr;
    int blen, dlen, mode;
    int nblk, nread, first;
    int ret = -1;
    struct timeval start, stop, difftime;

    if (init() != 0)
        return -1;

    ctrl = find_algo(algo);
    if (ctrl == NULL)
        return -1;
    base = ctrl->base_addr;
    if (base == 0) {
	fprintf(stderr, "core for algorithm \"%s\" not installed\n", algo);
	return -1;
    }

    blen = ctrl->block_len;
    daddr = ctrl->base_addr + ctrl->digest_addr;
    dlen = ctrl->digest_len;
    mode = ctrl->mode;

    if (strcmp(file, "-") != 0) {
        in_fd = open(file, O_RDONLY);
        if (in_fd < 0) {
            perror("open");
            return -1;
        }
    }

    if (verbose) {
        if (gettimeofday(&start, NULL) < 0) {
            perror("gettimeofday");
            goto out;
        }
    }

    for (nblk = 0, first = 1; ; ++nblk, first = 0) {
        nread = read(in_fd, block, blen);
        if (nread < 0) {
            /* read error */
            perror("read");
            goto out;
        }
        else if (nread < blen) {
            /* partial read = last block */
            if (pad_transmit(base, block, nread, blen, mode,
                             (nblk * blen + nread) * 8, first) != 0)
                goto out;
            break;
        }
        else {
            /* full block read */
            if (transmit(base, block, blen, mode, first) != 0)
                goto out;
        }
    }

    /* Strictly speaking we should query "valid" status before reading digest,
     * but transmit() waits for "ready" status before returning, and the SHA
     * cores always assert valid before ready.
     */
    if (tc_read(daddr, digest, dlen) != 0) {
        perror("eim read failed");
        goto out;
    }

    if (verbose) {
        if (gettimeofday(&stop, NULL) < 0) {
            perror("gettimeofday");
            goto out;
        }
        timersub(&stop, &start, &difftime);
        printf("%d blocks written in %d.%03d sec (%.3f blocks/sec)\n",
               nblk, (int)difftime.tv_sec, (int)difftime.tv_usec/1000,
               (float)nblk / ((float)difftime.tv_sec + ((float)difftime.tv_usec)/1000000));
    }

    ret = dlen;
out:
    if (in_fd != 0)
        close(in_fd);
    return ret;
}

/* ---------------- main ---------------- */

int main(int argc, char *argv[])
{
    int i, opt;
    char *algo = "sha-1";
    char *file = "-";
    uint8_t digest[512/8];
    int dlen;

    while ((opt = getopt(argc, argv, "h?dvq")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            printf(usage, argv[0]);
            return EXIT_SUCCESS;
        case 'd':
            tc_set_debug(1);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        default:
            fprintf(stderr, usage, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        algo = argv[optind];
        ++optind;
    }
    else {
        if (!quiet)
            printf("defaulting to algorithm \"%s\"\n", algo);
    }

    if (optind < argc) {
        file = argv[optind];
        ++optind;
    }
    else {
        if (!quiet)
            printf("reading from stdin\n");
    }

    dlen = hash(algo, file, digest);
    if (dlen < 0)
        return EXIT_FAILURE;

    for (i = 0; i < dlen; ++i) {
        printf("%02x", digest[i]);
        if (i % 16 == 15)
            printf("\n");
        else if (i % 4 == 3)
            printf(" ");
    }
    if (dlen % 16 != 0)
        printf("\n");

    return EXIT_SUCCESS;
}
