/*
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

/*
 * NOTE: This just exercises the interface to the TRNG cores, and displays
 * a few words of random data. It does not attempt to analyse the quality
 * of the data; for that, use something like dieharder.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h>

#include <hal.h>
#include <hal_internal.h>
#include <verilog_constants.h>

#ifndef WAIT_FOR_CSPRNG_VALID
#define WAIT_FOR_CSPRNG_VALID   1
#endif

static void show_core(const hal_core_t *core, const char *whinge)
{
  const hal_core_info_t *core_info = hal_core_info(core);
  if (core_info != NULL)
    printf("\"%8.8s\"  \"%4.4s\"\n", core_info->name, core_info->version);
  else if (whinge != NULL)
    printf("%s core not present\n", whinge);
}

static hal_error_t test_random(const char *name)
{
    const hal_core_t *core;
    hal_error_t err;
    uint32_t rnd;
    int i;

    core = hal_core_find(name, NULL);
    show_core(core, name);
    if (core == NULL)
        return HAL_ERROR_CORE_NOT_FOUND;

    for (i = 0; i < 8; ++i) {
	if (WAIT_FOR_CSPRNG_VALID && (err = hal_io_wait_valid(core)) != HAL_OK) {
	    printf("hal_io_wait_valid: %s\n", hal_error_string(err));
	    return err;
        }

	/* We use symbol CSPRNG_ADDR_RANDOM here, but the entropy sources
	 * present their data on the same register number.
	 */
        if ((err = hal_io_read(core, CSPRNG_ADDR_RANDOM, (uint8_t *) &rnd, sizeof(rnd))) != HAL_OK) {
            printf("hal_io_read: %s\n", hal_error_string(err));
            return err;
        }

        printf("%08lx ", (unsigned long) rnd);
    }
    printf("\n");

    return HAL_OK;
}

int main(void)
{
    hal_error_t err;
    uint32_t rnd[8];
    int i;

    /* Exercise the API function. This gets random data from the CSPRNG,
     * so we end up hitting that twice.
     */
    printf("hal_get_random\n");
    if ((err = hal_get_random(NULL, (void *) &rnd, sizeof(rnd))) != HAL_OK) {
	printf("hal_get_random: %s\n", hal_error_string(err));
    }
    else {
	for (i = 0; i < 8; ++i) {
	    printf("%08lx ", (unsigned long) rnd[i]);
	}
	printf("\n");
    }

    return
        test_random(AVALANCHE_ENTROPY_NAME) ||
        test_random(ROSC_ENTROPY_NAME) ||
        test_random(CSPRNG_NAME);
}
