/*
 * test-rpc_login.c
 * ----------------
 * Test code for RPC interface to Cryptech hash cores.
 *
 * Copyright (c) 2016, NORDUnet A/S
 * All rights reserved.
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
#include <string.h>
#include <strings.h>

#include <hal.h>

#define check(op)                                               \
    do {                                                        \
        hal_error_t err = (op);                                 \
        if (err) {                                              \
            printf("%s: %s\n", #op, hal_error_string(err));     \
            return err;                                         \
        }                                                       \
    } while (0)

int main(int argc, char *argv[])
{
    hal_client_handle_t client = {0};
    hal_user_t user = HAL_USER_WHEEL;

    if (argc < 3) {
        printf("usage: %s user pin\n", argv[0]);
        return 1;
    }

    if (strcasecmp(argv[1], "wheel") == 0)
        user = HAL_USER_WHEEL;
    else if (strcasecmp(argv[1], "so") == 0)
        user = HAL_USER_SO;
    else if (strcasecmp(argv[1], "user") == 0)
        user = HAL_USER_NORMAL;
    else {
        printf("user name must be one of 'wheel', 'so', or 'user'\n");
        return 1;
    }

    check(hal_rpc_client_init());

    check(hal_rpc_login(client, user, argv[2], strlen(argv[2])));
    check(hal_rpc_is_logged_in(client, user));
    check(hal_rpc_logout(client));

    check(hal_rpc_client_close());
    return 0;
}
