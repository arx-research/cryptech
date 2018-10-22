/*
 * tc_i2c.c
 * --------
 * This module contains common code to talk to the FPGA over the I2C bus.
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "cryptech.h"

static int debug = 0;
static int i2cfd = -1;

/* ---------------- I2C low-level code ---------------- */

void tc_set_debug(int onoff)
{
    debug = onoff;
}

static void dump(char *label, const uint8_t *buf, size_t len)
{
    if (debug) {
        int i;
        printf("%s [", label);
        for (i = 0; i < len; ++i)
            printf(" %02x", buf[i]);
        printf(" ]\n");
    }
}

static void i2c_close(void)
{
    close(i2cfd);
}

static int i2c_open(void)
{
    if (i2cfd >= 0)
        return 0;

    i2cfd = open(I2C_dev, O_RDWR);
    if (i2cfd < 0) {
        fprintf(stderr, "Unable to open %s: ", I2C_dev);
        perror("");
        i2cfd = 0;
        return 1;
    }

    if (ioctl(i2cfd, I2C_SLAVE, I2C_addr) < 0) {
        fprintf(stderr, "Unable to set I2C slave device 0x%02x: ", I2C_addr);
        perror("");
        return 1;
    }

    if (atexit(i2c_close) != 0) {
        fprintf(stderr, "Unable to set I2C atexit handler.");
        return 1;
    }

    return 0;
}

static int i2c_write(const uint8_t *buf, size_t len)
{
    if (i2c_open() != 0)
        return 1;

    dump("write ", buf, len);

    if (write(i2cfd, buf, len) != len) {
        perror("i2c write failed");
        return 1;
    }

    return 0;
}

static int i2c_read(uint8_t *b)
{
    if (i2c_open() != 0)
        return 1;

    /* read() on the i2c device only returns one byte at a time,
     * and tc_get_resp() needs to parse the response one byte at a time
     */
    if (read(i2cfd, b, 1) != 1) {
        perror("i2c read failed");
        return 1;
    }

    return 0;
}

/* ---------------- test-case low-level code ---------------- */

/* coretest command codes */
#define SOC       0x55
#define EOC       0xaa
#define READ_CMD  0x10
#define WRITE_CMD 0x11
#define RESET_CMD 0x01

/* coretest response codes */
#define SOR       0xaa
#define EOR       0x55
#define READ_OK   0x7f
#define WRITE_OK  0x7e
#define RESET_OK  0x7d
#define UNKNOWN   0xfe
#define ERROR     0xfd

static int tc_send_write_cmd(off_t offset, const uint8_t *data)
{
    uint8_t buf[9] = { SOC, WRITE_CMD, (offset >> 8) & 0xff, offset & 0xff,
                       data[0], data[1], data[2], data[3], EOC };

    return i2c_write(buf, sizeof(buf));
}

static int tc_send_read_cmd(off_t offset)
{
    uint8_t buf[5] = { SOC, READ_CMD, (offset >> 8) & 0xff, offset & 0xff, EOC };

    return i2c_write(buf, sizeof(buf));
}

static int tc_get_resp(uint8_t *buf, size_t len)
{
    int i;

    for (i = 0; i < len; ++i) {
        if (i2c_read(&buf[i]) != 0)
            return 1;
        if ((i == 0) && (buf[i] != SOR)) {
            /* we've gotten out of sync, and there's probably nothing we can do */
            fprintf(stderr, "response byte 0: expected 0x%02x (SOR), got 0x%02x\n",
                    SOR, buf[0]);
            return 1;
        }
        else if (i == 1) {      /* response code */
            switch (buf[i]) {
            case READ_OK:
                len = 9;
                break;
            case WRITE_OK:
                len = 5;
                break;
            case RESET_OK:
                len = 3;
                break;
            case ERROR:
            case UNKNOWN:
                len = 4;
                break;
            default:
                /* we've gotten out of sync, and there's probably nothing we can do */
                fprintf(stderr, "unknown response code 0x%02x\n", buf[i]);
                return 1;
            }
        }
    }

    dump("read  ", buf, len);

    return 0;
}

static int tc_compare(uint8_t *buf, const uint8_t *expected, size_t len)
{
    int i;

    /* start at byte 1 because SOR has already been tested */
    for (i = 1; i < len; ++i) {
        if (buf[i] != expected[i]) {
            fprintf(stderr, "response byte %d: expected 0x%02x, got 0x%02x\n",
                    i, expected[i], buf[i]);
            return 1;
        }
    }

    return 0;
}

static int tc_get_write_resp(off_t offset)
{
    uint8_t buf[5];
    uint8_t expected[5] = { SOR, WRITE_OK, (offset >> 8) & 0xff, offset & 0xff, EOR };

    return
        tc_get_resp(buf, sizeof(buf)) ||
        tc_compare(buf, expected, sizeof(expected));
}

static int tc_get_read_resp(off_t offset, uint8_t *data)
{
    uint8_t buf[9];
    uint8_t expected[4] = { SOR, READ_OK, (offset >> 8) & 0xff, offset & 0xff };

    if ((tc_get_resp(buf, sizeof(buf)) != 0) ||
        (tc_compare(buf, expected, 4) != 0) || buf[8] != EOR)
        return 1;

    data[0] = buf[4];
    data[1] = buf[5];
    data[2] = buf[6];
    data[3] = buf[7];

    return 0;
}

static int tc_get_read_resp_expected(off_t offset, const uint8_t *data)
{
    uint8_t buf[9];
    uint8_t expected[9] = { SOR, READ_OK, (offset >> 8) & 0xff, offset & 0xff,
                            data[0], data[1], data[2], data[3], EOR };

    dump("expect", expected, 9);

    return (tc_get_resp(buf, sizeof(buf)) ||
            tc_compare(buf, expected, sizeof(buf)));
}

int tc_write(off_t offset, const uint8_t *buf, size_t len)
{
    for (; len > 0; offset++, buf += 4, len -= 4) {
        if (tc_send_write_cmd(offset, buf) ||
            tc_get_write_resp(offset))
            return 1;
    }

    return 0;
}

int tc_read(off_t offset, uint8_t *buf, size_t len)
{
    for (; len > 0; offset++, buf += 4, len -= 4) {
        if (tc_send_read_cmd(offset) ||
            tc_get_read_resp(offset, buf))
            return 1;
    }

    return 0;
}

int tc_expected(off_t offset, const uint8_t *buf, size_t len)
{
    for (; len > 0; offset++, buf += 4, len -= 4) {
        if (tc_send_read_cmd(offset) ||
            tc_get_read_resp_expected(offset, buf))
            return 1;
    }

    return 0;
}

int tc_init(off_t offset)
{
    uint8_t buf[4] = { 0, 0, 0, CTRL_INIT };

    return tc_write(offset, buf, 4);
}

int tc_next(off_t offset)
{
    uint8_t buf[4] = { 0, 0, 0, CTRL_NEXT };

    return tc_write(offset, buf, 4);
}

int tc_wait(off_t offset, uint8_t status, int *count)
{
    uint8_t buf[4];
    int i;

    for (i = 1; ; ++i) {
        if (count && (*count > 0) && (i >= *count)) {
            fprintf(stderr, "tc_wait timed out\n");
            return 1;
        }
        if (tc_read(offset, buf, 4) != 0)
            return -1;
        if (buf[3] & status) {
            if (count)
                *count = i;
            return 0;
        }
    }
}

int tc_wait_ready(off_t offset)
{
    int limit = 10;
    return tc_wait(offset, STATUS_READY, &limit);
}

int tc_wait_valid(off_t offset)
{
    int limit = 10;
    return tc_wait(offset, STATUS_VALID, &limit);
}
