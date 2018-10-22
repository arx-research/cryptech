/*
 * mgmt-keystore.c
 * ---------------
 * CLI 'keystore' commands.
 *
 * Copyright (c) 2016-2017, NORDUnet A/S All rights reserved.
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
#include "stm-keystore.h"
#include "stm-fpgacfg.h"
#include "stm-uart.h"

#include "mgmt-cli.h"

#undef HAL_OK
#define LIBHAL_OK HAL_OK
#include "hal.h"
#warning Really should not be including hal_internal.h here, fix API instead of bypassing it
#include "hal_internal.h"
#undef HAL_OK

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>


static int cmd_keystore_set_pin(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_user_t user;
    hal_error_t status;
    hal_client_handle_t client = { -1 };

    command = command;

    if (argc != 2) {
	cli_print(cli, "Wrong number of arguments (%i).", argc);
	cli_print(cli, "Syntax: keystore set pin <user|so|wheel> <pin>");
	return CLI_ERROR;
    }

    if (strcmp(argv[0], "user") == 0)
	user = HAL_USER_NORMAL;
    else if (strcmp(argv[0], "so") == 0)
	user = HAL_USER_SO;
    else if (strcmp(argv[0], "wheel") == 0)
	user = HAL_USER_WHEEL;
    else {
	cli_print(cli, "First argument must be 'user', 'so' or 'wheel' - not '%s'", argv[0]);
	return CLI_ERROR;
    }

    status = hal_rpc_set_pin(client, user, argv[1], strlen(argv[1]));
    if (status != LIBHAL_OK) {
	cli_print(cli, "Failed setting PIN: %s", hal_error_string(status));
	return CLI_ERROR;
    }

    return CLI_OK;
}

static int cmd_keystore_clear_pin(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_user_t user;
    hal_error_t status;
    hal_client_handle_t client = { -1 };

    command = command;

    if (argc != 1) {
	cli_print(cli, "Wrong number of arguments (%i).", argc);
	cli_print(cli, "Syntax: keystore clear pin <user|so|wheel>");
	return CLI_ERROR;
    }

    user = HAL_USER_NONE;
    if (strcmp(argv[0], "user") == 0)
	user = HAL_USER_NORMAL;
    else if (strcmp(argv[0], "so") == 0)
	user = HAL_USER_SO;
    else if (strcmp(argv[0], "wheel") == 0)
	user = HAL_USER_WHEEL;
    else {
	cli_print(cli, "First argument must be 'user', 'so' or 'wheel' - not '%s'", argv[0]);
	return CLI_ERROR;
    }

    if ((status = hal_rpc_set_pin(client, user, "", 0)) != LIBHAL_OK) {
        cli_print(cli, "Failed clearing PIN: %s", hal_error_string(status));
        return CLI_ERROR;
    }

    return CLI_OK;
}

static int cmd_keystore_set_pin_iterations(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_error_t status;
    hal_client_handle_t client = { -1 };

    command = command;

    if (argc != 1) {
	cli_print(cli, "Wrong number of arguments (%i).", argc);
	cli_print(cli, "Syntax: keystore set pin iterations <number>");
	return CLI_ERROR;
    }

    status = hal_set_pin_default_iterations(client, strtoul(argv[0], NULL, 0));
    if (status != LIBHAL_OK) {
	cli_print(cli, "Failed setting iterations: %s", hal_error_string(status));
	return CLI_ERROR;
    }

    return CLI_OK;
}

static int cmd_keystore_delete_key(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    const hal_client_handle_t  client  = { -1 };
    const hal_session_handle_t session = { HAL_HANDLE_NONE };
    hal_pkey_handle_t pkey = { HAL_HANDLE_NONE };
    hal_error_t status;
    hal_uuid_t name;

    command = command;

    if (argc != 1) {
	cli_print(cli, "Wrong number of arguments (%i).", argc);
	cli_print(cli, "Syntax: keystore delete key <name>");
	return CLI_ERROR;
    }

    if ((status = hal_uuid_parse(&name, argv[0])) != LIBHAL_OK) {
	cli_print(cli, "Couldn't parse key name: %s", hal_error_string(status));
	return CLI_ERROR;
    }

    if ((status = hal_rpc_pkey_open(client, session, &pkey, &name)) != LIBHAL_OK) {
        cli_print(cli, "Couldn't find key: %s", hal_error_string(status));
	return CLI_ERROR;
    }

    if ((status = hal_rpc_pkey_delete(pkey)) != LIBHAL_OK) {
	cli_print(cli, "Failed deleting key: %s", hal_error_string(status));
	(void) hal_rpc_pkey_close(pkey);
	return CLI_ERROR;
    }

    cli_print(cli, "Deleted key %s", argv[0]);

    return CLI_OK;
}

#include "ks.h"

static int show_keys(struct cli_def *cli, const char *title)
{
    const hal_client_handle_t  client  = { -1 };
    const hal_session_handle_t session = { HAL_HANDLE_NONE };
    char key_name[HAL_UUID_TEXT_SIZE];
    hal_uuid_t previous_uuid = {{0}};
    hal_pkey_handle_t pkey;
    hal_curve_name_t curve;
    hal_key_flags_t flags;
    unsigned n, state = 0;
    hal_uuid_t uuids[50];
    hal_key_type_t type;
    hal_error_t status;
    int count = 0;
    int done = 0;

    cli_print(cli, title);

    size_t avail;
    if ((status = hal_ks_available(hal_ks_token, &avail)) == HAL_OK)
        cli_print(cli, "Token keystore: %d available", avail);
    else
        cli_print(cli, "Error reading token keystore: %s", hal_error_string(status));
    if ((status = hal_ks_available(hal_ks_volatile, &avail)) == HAL_OK)
        cli_print(cli, "Volatile keystore: %d available", avail);
    else
        cli_print(cli, "Error reading volatile keystore: %s", hal_error_string(status));

    while (!done) {

	if ((status = hal_rpc_pkey_match(client, session, HAL_KEY_TYPE_NONE, HAL_CURVE_NONE,
					 0, 0, NULL, 0, &state, uuids, &n,
					 sizeof(uuids)/sizeof(*uuids),
					 &previous_uuid)) != LIBHAL_OK) {
	    cli_print(cli, "Could not fetch UUID list: %s", hal_error_string(status));
	    return CLI_ERROR;
	}

	done = n < sizeof(uuids)/sizeof(*uuids);

	if (!done)
	    previous_uuid = uuids[sizeof(uuids)/sizeof(*uuids) - 1];

	for (unsigned i = 0; i < n; i++) {

	    if ((status = hal_uuid_format(&uuids[i], key_name, sizeof(key_name))) != LIBHAL_OK) {
		cli_print(cli, "Could not convert key name, skipping: %s",
			  hal_error_string(status));
		continue;
	    }

	    if ((status = hal_rpc_pkey_open(client, session, &pkey, &uuids[i])) != LIBHAL_OK) {
	        cli_print(cli, "Could not open key %s, skipping: %s",
			  key_name, hal_error_string(status));
		continue;
	    }

	    if ((status = hal_rpc_pkey_get_key_type(pkey, &type))   != LIBHAL_OK ||
		(status = hal_rpc_pkey_get_key_curve(pkey, &curve)) != LIBHAL_OK ||
		(status = hal_rpc_pkey_get_key_flags(pkey, &flags)) != LIBHAL_OK)
	        cli_print(cli, "Could not fetch metadata for key %s, skipping: %s",
			  key_name, hal_error_string(status));

	    if (status == LIBHAL_OK)
	        status = hal_rpc_pkey_close(pkey);
	    else
	        (void) hal_rpc_pkey_close(pkey);

	    if (status != LIBHAL_OK)
	        continue;

	    const char *type_name = "unknown";
	    switch (type) {
	    case HAL_KEY_TYPE_NONE:		type_name = "none";		break;
	    case HAL_KEY_TYPE_RSA_PRIVATE:	type_name = "RSA private";	break;
	    case HAL_KEY_TYPE_RSA_PUBLIC:	type_name = "RSA public";	break;
	    case HAL_KEY_TYPE_EC_PRIVATE:	type_name = "EC private";	break;
	    case HAL_KEY_TYPE_EC_PUBLIC:	type_name = "EC public";	break;
            case HAL_KEY_TYPE_HASHSIG_PRIVATE:  type_name = "hashsig private";  break;
            case HAL_KEY_TYPE_HASHSIG_PUBLIC:   type_name = "hashsig public";   break;
            case HAL_KEY_TYPE_HASHSIG_LMS:      type_name = "hashsig lms";      break;
            case HAL_KEY_TYPE_HASHSIG_LMOTS:    type_name = "hashsig lmots";    break;
	    }

	    const char *curve_name = "unknown";
	    switch (curve) {
	    case HAL_CURVE_NONE:		curve_name = "none";		break;
	    case HAL_CURVE_P256:		curve_name = "P-256";		break;
	    case HAL_CURVE_P384:		curve_name = "P-384";		break;
	    case HAL_CURVE_P521:		curve_name = "P-521";		break;
	    }

	    cli_print(cli, "Key %2i, name %s, type %s, curve %s, flags 0x%lx",
		      count++, key_name, type_name, curve_name, (unsigned long) flags);
	}
    }

    return CLI_OK;
}

static int show_pin(struct cli_def *cli, char *label, hal_user_t user)
{
    const hal_ks_pin_t *p;

    if (hal_get_pin(user, &p) != HAL_OK)
        return CLI_ERROR;

    /*
     * I'm not sure iterations is the most interesting thing to show, but
     * it's what we had before.
     */

    cli_print(cli, "%s iterations: 0x%lx", label, p->iterations);
    return CLI_OK;
}

static int cmd_keystore_show_keys(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;
    argv = argv;
    argc = argc;

    int err = 0;

    err |= show_keys(cli, "Keystore:");

    cli_print(cli, "\nPins:");
    err |= show_pin(cli, "Wheel", HAL_USER_WHEEL);
    err |= show_pin(cli, "SO   ", HAL_USER_SO);
    err |= show_pin(cli, "User ", HAL_USER_NORMAL);

    return err ? CLI_ERROR : CLI_OK;
}

static int cmd_keystore_erase(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    hal_error_t err;
    HAL_StatusTypeDef status;
    int preserve_PINs = 0;

    command = command;

    if (argc < 1 || argc > 2 || strcmp(argv[0], "YesIAmSure") != 0) {
    usage:
	cli_print(cli, "Syntax: keystore erase YesIAmSure [preservePINs]");
	return CLI_ERROR;
    }
    if (argc == 2) {
        if (strcasecmp(argv[1], "preservePINs") != 0)
            goto usage;
        else
            preserve_PINs = 1;
    }

    hal_user_t users[3] = { HAL_USER_NORMAL, HAL_USER_SO, HAL_USER_WHEEL };
    hal_ks_pin_t pins[3];
    if (preserve_PINs) {
        for (size_t i = 0; i < 3; ++i) {
            const hal_ks_pin_t *pin;
            if (hal_get_pin(users[i], &pin) != HAL_OK) {
                cli_print(cli, "Failed to get the PINs");
                return CLI_ERROR;
            }
            memcpy(&pins[i], pin, sizeof(*pin));
        }
    }

    cli_print(cli, "OK, erasing keystore, this will take about 45 seconds...");
    if ((status = keystore_erase_bulk()) != CMSIS_HAL_OK) {
        cli_print(cli, "Failed erasing token keystore: %i", status);
	return CLI_ERROR;
    }

    if ((err = hal_ks_init(hal_ks_token, 0)) != LIBHAL_OK) {
        cli_print(cli, "Failed to reinitialize token keystore: %s", hal_error_string(err));
	return CLI_ERROR;
    }

    if ((err = hal_ks_init(hal_ks_volatile, 0)) != LIBHAL_OK) {
        cli_print(cli, "Failed to reinitialize memory keystore: %s", hal_error_string(err));
	return CLI_ERROR;
    }

    if (preserve_PINs) {
        for (size_t i = 0; i < 3; ++i) {
            if (hal_set_pin(users[i], &pins[i]) != HAL_OK) {
                cli_print(cli, "Failed to restore the PINs");
                return CLI_ERROR;
            }
        }
    }

    cli_print(cli, "Keystore erased");
    return CLI_OK;
}

void configure_cli_keystore(struct cli_def *cli)
{
    struct cli_command *c = cli_register_command(cli, NULL, "keystore", NULL, 0, 0, NULL);

    struct cli_command *c_show   = cli_register_command(cli, c, "show",   NULL, 0, 0, NULL);
    struct cli_command *c_set    = cli_register_command(cli, c, "set",    NULL, 0, 0, NULL);
    struct cli_command *c_clear  = cli_register_command(cli, c, "clear",  NULL, 0, 0, NULL);
    struct cli_command *c_delete = cli_register_command(cli, c, "delete", NULL, 0, 0, NULL);

    /* keystore show keys */
    cli_register_command(cli, c_show, "keys", cmd_keystore_show_keys, 0, 0, "Show what PINs and keys are in the keystore");

    /* keystore set pin */
    struct cli_command *c_set_pin = cli_register_command(cli, c_set, "pin", cmd_keystore_set_pin, 0, 0, "Set either 'wheel', 'user' or 'so' PIN");

    /* keystore set pin iterations */
    cli_register_command(cli, c_set_pin, "iterations", cmd_keystore_set_pin_iterations, 0, 0, "Set PBKDF2 iterations for PINs");

    /* keystore clear pin */
    cli_register_command(cli, c_clear, "pin", cmd_keystore_clear_pin, 0, 0, "Clear either 'wheel', 'user' or 'so' PIN");

    /* keystore delete key */
    cli_register_command(cli, c_delete, "key", cmd_keystore_delete_key, 0, 0, "Delete a key");

    /* keystore erase */
    cli_register_command(cli, c, "erase", cmd_keystore_erase, 0, 0, "Erase the whole keystore");
}
