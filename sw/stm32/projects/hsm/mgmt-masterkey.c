/*
 * mgmt-masterkey.c
 * ----------------
 * Masterkey CLI functions.
 *
 * Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

/* Rename both CMSIS HAL_OK and libhal HAL_OK to disambiguate */
#define HAL_OK CMSIS_HAL_OK
#include "stm-init.h"
#include "stm-uart.h"
#include "mgmt-cli.h"
#include "mgmt-masterkey.h"

#undef HAL_OK
#define LIBHAL_OK HAL_OK
#include <hal.h>
#warning Refactor so we do not need to include hal_internal.h here
#include <hal_internal.h>
#undef HAL_OK

#include <stdlib.h>

static char * _status2str(const hal_error_t status)
{
    switch (status) {
    case LIBHAL_OK:
	return (char *) "Set";
    case HAL_ERROR_MASTERKEY_NOT_SET:
	return (char *) "Not set";
    default:
	return (char *) "Unknown";
    }
}

static int cmd_masterkey_status(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_error_t status;

    command = command;
    argv = argv;
    argc = argc;

    cli_print(cli, "Status of master key:\n");

    status = hal_mkm_volatile_read(NULL, 0);
    cli_print(cli, "  volatile: %s / %s", _status2str(status), hal_error_string(status));

    status = hal_mkm_flash_read(NULL, 0);
    cli_print(cli, "     flash: %s / %s", _status2str(status), hal_error_string(status));

    return CLI_OK;
}

static int str_to_hex_digit(char c)
{
    if (c >= '0' && c <= '9')
        c -= '0';
    else if (c >= 'a' && c <= 'f')
        c =  c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        c =  c - 'A' + 10;
    else
        return -1;

    return c;
}

static inline char hex_to_str_digit(const uint8_t c)
{
    return (c < 10) ? ((char)c + '0') : ((char)c + 'A' - 10);
}

static char *hexdump_kek(const uint8_t * const kek)
{
    /* This is only for dumping masterkey values, so has no length checks.
     * Do not use it for anything else.
     *
     * For convenience of possibly hand-copying and hand-retyping, the key
     * is divided into 8 4-byte (8-character) groups.
     */

    static char buf[2 * KEK_LENGTH + 8];
    char *dst = buf;

    for (size_t i = 0; i < KEK_LENGTH; ++i) {
        uint8_t b = kek[i];
        *dst++ = hex_to_str_digit(b >> 4);
        *dst++ = hex_to_str_digit(b & 0xf);
        if ((i & 3) == 3)
            *dst++ = ' ';
    }
    buf[sizeof(buf) - 1] = '\0';

    return buf;
}

static int _masterkey_set(struct cli_def *cli, char *argv[], int argc,
                          char *label, hal_error_t (*writer)(const uint8_t * const, const size_t))
{
    uint8_t buf[KEK_LENGTH] = {0};
    hal_error_t err;

    if (argc == 0) {
        /* fill master key with yummy randomness */
        if ((err = hal_get_random(NULL, buf, sizeof(buf))) != LIBHAL_OK) {
            cli_print(cli, "Error getting random key: %s", hal_error_string(err));
            return CLI_ERROR;
        }
        cli_print(cli, "Random key:\n%s", hexdump_kek(buf));
    }

    else {
        /* input is 32 hex bytes, arranged however the user wants */
        size_t len = 0;
        for (int i = 0; i < argc; ++i) {
            for (char *cp = argv[i]; *cp != '\0'; ) {
                int c;
                if ((c = str_to_hex_digit(*cp++)) < 0)
                    goto errout;
                buf[len] = c << 4;
                if ((c = str_to_hex_digit(*cp++)) < 0)
                    goto errout;
                buf[len] |= c & 0xf;
                if (++len > KEK_LENGTH)
                    goto errout;
            }
        }
        if (len < KEK_LENGTH) {
        errout:
            cli_print(cli, "Failed parsing master key, expected exactly %d hex bytes", KEK_LENGTH);
            return CLI_ERROR;
        }

        cli_print(cli, "Parsed key:\n%s", hexdump_kek(buf));
    }

    if ((err = writer(buf, sizeof(buf))) == LIBHAL_OK) {
	cli_print(cli, "Master key set in %s memory", label);
    } else {
	cli_print(cli, "Failed writing key to %s memory: %s", label, hal_error_string(err));
    }
    return CLI_OK;
}

static int cmd_masterkey_set(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;

    return _masterkey_set(cli, argv, argc, "volatile", hal_mkm_volatile_write);
}

static int cmd_masterkey_erase(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_error_t err;

    command = command;
    argv = argv;
    argc = argc;

    if ((err = hal_mkm_volatile_erase(KEK_LENGTH)) == LIBHAL_OK) {
	cli_print(cli, "Erased master key from volatile memory");
    } else {
	cli_print(cli, "Failed erasing master key from volatile memory: %s", hal_error_string(err));
    }
    return CLI_OK;
}

static int cmd_masterkey_unsecure_set(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;

    return _masterkey_set(cli, argv, argc, "flash", hal_mkm_flash_write);
}

static int cmd_masterkey_unsecure_erase(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_error_t err;

    command = command;
    argv = argv;
    argc = argc;

    if ((err = hal_mkm_flash_erase(KEK_LENGTH)) == LIBHAL_OK) {
	cli_print(cli, "Erased unsecure master key from flash");
    } else {
	cli_print(cli, "Failed erasing unsecure master key from flash: %s", hal_error_string(err));
    }
    return CLI_OK;
}

void configure_cli_masterkey(struct cli_def *cli)
{
    struct cli_command *c = cli_register_command(cli, NULL, "masterkey", NULL, 0, 0, NULL);

    /* masterkey status */
    cli_register_command(cli, c, "status", cmd_masterkey_status, 0, 0, "Show status of master key in RAM/flash");

    /* masterkey set */
    cli_register_command(cli, c, "set", cmd_masterkey_set, 0, 0, "Set the master key in the volatile Master Key Memory");

    /* masterkey erase */
    cli_register_command(cli, c, "erase", cmd_masterkey_erase, 0, 0, "Erase the master key from the volatile Master Key Memory");

    struct cli_command *c_unsecure = cli_register_command(cli, c, "unsecure", NULL, 0, 0, NULL);

    /* masterkey unsecure set */
    cli_register_command(cli, c_unsecure, "set", cmd_masterkey_unsecure_set, 0, 0, "Set master key in unprotected flash memory (if unsure, DON'T)");

    /* masterkey unsecure erase */
    cli_register_command(cli, c_unsecure, "erase", cmd_masterkey_unsecure_erase, 0, 0, "Erase master key from unprotected flash memory");
}
