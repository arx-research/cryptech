/*
 * test-bus.c
 * ----------
 * Test raw read/write performance across the EIM or FMC bus.
 *
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
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>

#include <hal.h>

#define TEST_NUM_ROUNDS         2000000

/*
 * Sanity test - can we read and write the dummy register?
 */

static int sanity(const hal_core_t *board_core)
{
    uint32_t rnd = 0xdeadbeef, data;
    hal_error_t err;

    if (board_core == NULL) {
        printf("initialization failed (is the bitstream loaded?)\n");
        return 1;
    }

    if ((err = hal_io_write(board_core, 0xFF, (const uint8_t *) &rnd, sizeof(rnd))) != HAL_OK) {
        printf("writing dummy: %s\n", hal_error_string(err));
        return 1;
    }
    if ((err = hal_io_read(board_core, 0xFF, (uint8_t *) &data, sizeof(data))) != HAL_OK) {
        printf("reading dummy: %s\n", hal_error_string(err));
        return 1;
    }

    if (data != rnd) {
        printf("Data bus fail: expected %08lx, got %08lx, diff %08lx\n",
	       (unsigned long) rnd, (unsigned long) data, (unsigned long) (data ^ rnd));
        return 1;
    }

    return 0;
}

/*
 * Time a test.
 */

static void _time_check(char *label, const struct timeval t0, const int err)
{
  struct timeval t;
  float rounds;
  gettimeofday(&t, NULL);
  t.tv_sec -= t0.tv_sec;
  t.tv_usec -= t0.tv_usec;
  if (t.tv_usec < 0) {
    t.tv_usec += 1000000;
    t.tv_sec  -= 1;
  }
  rounds = (float)TEST_NUM_ROUNDS/((float)t.tv_sec + ((float)t.tv_usec / 1000000));
  printf("%s%lu.%06lu seconds, %u/sec\n", label,
	 (unsigned long)t.tv_sec, (unsigned long)t.tv_usec, (unsigned)rounds);
}

#define time_check(_label_, _expr_)             \
  do {                                          \
    struct timeval _t;                          \
    gettimeofday(&_t, NULL);                    \
    int _err = (_expr_);                        \
    _time_check(_label_, _t, _err);             \
    err |= _err;                                \
  } while (0)

/*
 * Read and write over and over again.
 */

static int test_read(const hal_core_t *board_core)
{
    uint32_t i, data;
    hal_error_t err;

    for (i = 0; i < TEST_NUM_ROUNDS; ++i) {
	if ((err = hal_io_read(board_core, 0xFF, (uint8_t *) &data, sizeof(data))) != HAL_OK) {
            printf("reading dummy: %s\n", hal_error_string(err));
            return 1;
        }
    }

    return 0;
}

static int test_write(const hal_core_t *board_core)
{
    uint32_t i;
    hal_error_t err;

    for (i = 0; i < TEST_NUM_ROUNDS; ++i) {
	if ((err = hal_io_write(board_core, 0xFF, (const uint8_t *) &i, sizeof(i))) != HAL_OK) {
            printf("writing dummy: %s\n", hal_error_string(err));
            return 1;
        }
    }

    return 0;
}

int main(void)
{
    const hal_core_t *board_core = hal_core_find(NOVENA_BOARD_NAME, NULL);
    int err = 0;

    if (sanity(board_core) != 0)
        return 1;

    time_check("read  ", test_read(board_core));
    time_check("write ", test_write(board_core));

    return err;
}
