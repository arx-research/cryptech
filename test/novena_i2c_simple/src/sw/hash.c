/* 
 * hash.c
 * ------
 * This program uses the coretest_hashes subsystem to produce a
 * cryptographic hash of a file or input stream. It is a generalization
 * of the hash_tester.c test program.
 * 
 * Author: Paul Selkirk
 * Copyright (c) 2014, SUNET
 * 
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided that the following 
 * conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <assert.h>

char *usage = 
"Usage: %s [-d] [-v] [-q] [-i I2C_device] [-a I2C_addr] [algorithm [file]]\n"
"algorithms: sha-1, sha-256, sha-512/224, sha-512/256, sha-384, sha-512\n";

/* I2C configuration */
#define I2C_dev  "/dev/i2c-2"

int debug = 0;
int verbose = 0;

/* block and digest lengths are number of bytes */
#define SHA1_BLOCK_LEN          512/8
#define SHA1_DIGEST_LEN         160/8
#define SHA256_BLOCK_LEN        512/8
#define SHA256_DIGEST_LEN       256/8
#define SHA512_BLOCK_LEN        1024/8
#define SHA512_224_DIGEST_LEN   224/8
#define SHA512_256_DIGEST_LEN   256/8
#define SHA384_DIGEST_LEN       384/8
#define SHA512_DIGEST_LEN       512/8

/* ---------------- algorithm lookup code ---------------- */

struct ctrl {
    char *name;
    uint8_t i2c_addr;
    uint8_t block_len;
    uint8_t digest_len;
} ctrl[] = {
    { "sha-1",       0x1e, SHA1_BLOCK_LEN,   SHA1_DIGEST_LEN },
    { "sha-256",     0x1f, SHA256_BLOCK_LEN, SHA256_DIGEST_LEN },
    { "sha-512/224", 0x20, SHA512_BLOCK_LEN, SHA512_224_DIGEST_LEN },
    { "sha-512/256", 0x21, SHA512_BLOCK_LEN, SHA512_256_DIGEST_LEN },
    { "sha-384",     0x22, SHA512_BLOCK_LEN, SHA384_DIGEST_LEN },
    { "sha-512",     0x23, SHA512_BLOCK_LEN, SHA512_DIGEST_LEN },
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

/* ---------------- I2C low-level code ---------------- */

/* return file descriptor for i2c device */
int i2c_open(char *dev, int addr)
{
    int fd;

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Unable to open %s: ", dev);
        perror("");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        fprintf(stderr, "Unable to set I2C slave device 0x%02x: ", addr);
        perror("");
        close(fd);
        return -1;
    }

    return fd;
}

void i2c_close(int ifd)
{
    close(ifd);
}

/* ---------------- hash ---------------- */

int transmit(uint8_t *block, uint8_t blen, int fd)
{
    int i;

    if (debug) {
        printf("write [");
        for (i = 0; i < blen; ++i)
            printf(" %02x", block[i]);
        printf(" ]\n");
    }

    if (write(fd, block, blen) != blen) {
        return 1;
    }

    return 0;
}

int pad_transmit(uint8_t *block, uint8_t flen, uint8_t blen, int fd, long long tlen)
{
    assert(flen < blen);

    block[flen++] = 0x80;
    memset(block + flen, 0, blen - flen);

    if (blen - flen < ((blen == 64) ? 8 : 16)) {
        if (transmit(block, blen, fd) != 0)
            return 1;
        memset(block, 0, blen);
    }

    /* properly the length is 128 bits for sha-512, but we can't
     * actually count above 64 bits
     */
    ((uint32_t *)block)[blen/4 - 2] = htonl((tlen >> 32) & 0xffff);
    ((uint32_t *)block)[blen/4 - 1] = htonl(tlen & 0xffff);

    return transmit(block, blen, fd);
}

/* return number of digest bytes read */
int hash(char *dev, char *algo, char *file, uint8_t *digest)
{
    uint8_t block[SHA512_BLOCK_LEN];
    struct ctrl *ctrl;
    int i2c_fd, in_fd = 0;
    int addr, blen, dlen;
    int nblk, nread;
    int i, ret = -1;
    struct timeval start, stop, difftime;

    ctrl = find_algo(algo);
    if (ctrl == NULL)
        return -1;
    addr = ctrl->i2c_addr;
    blen = ctrl->block_len;
    dlen = ctrl->digest_len;

    i2c_fd = i2c_open(dev, addr);
    if (i2c_fd < 0)
        return -1;

    if (strcmp(file, "-") != 0) {
        in_fd = open(file, O_RDONLY);
        if (in_fd < 0) {
            perror("open");
            goto out2;
        }
    }

    if (verbose) {
        if (gettimeofday(&start, NULL) < 0) {
            perror("gettimeofday");
            goto out;
        }
    }

    for (nblk = 0; ; ++nblk) {
        nread = read(in_fd, block, blen);
        if (nread < 0) {
            /* read error */
            perror("read");
            goto out;
        }
        else if (nread < blen) {
            /* partial read = last block */
            if (pad_transmit(block, nread, blen, i2c_fd,
                             (nblk * blen + nread) * 8) != 0)
                goto out;
            break;
        }
        else {
            /* full block read */
            if (transmit(block, blen, i2c_fd) != 0)
                goto out;
        }
    }

    for (i = 0; i < dlen; ++i) {
        /* read() on the i2c device only returns one byte at a time */
        if (read(i2c_fd, &digest[i], 1) != 1) {
            perror("i2c read failed");
            goto out;
        }
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
out2:
    i2c_close(i2c_fd);
    return ret;
}

/* ---------------- main ---------------- */

int main(int argc, char *argv[])
{
    char *dev = I2C_dev;
    int addr = 0;
    int i, opt, quiet = 0;
    char *algo = "sha-1";
    char *file = "-";
    uint8_t digest[512/8];
    int dlen;

    while ((opt = getopt(argc, argv, "h?dvqi:a:")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            printf(usage, argv[0]);
            return 0;
        case 'd':
            debug = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        case 'i':
            dev = optarg;
            break;
        case 'a':
            addr = (int)strtol(optarg, NULL, 0);
            if ((addr < 0x03) || (addr > 0x77)) {
                fprintf(stderr, "addr must be between 0x03 and 0x77\n");
                return 1;
            }
            break;
        default:
            fprintf(stderr, usage, argv[0]);
            return 1;
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

    dlen = hash(dev, algo, file, digest);
    if (dlen < 0)
        return 1;

    for (i = 0; i < dlen; ++i) {
        printf("%02x", digest[i]);
        if (i % 16 == 15)
            printf("\n");
        else if (i % 4 == 3)
            printf(" ");
    }
    if (dlen % 16 != 0)
        printf("\n");

    return 0;
}
