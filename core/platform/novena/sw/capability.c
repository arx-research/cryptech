/*
 * capability.c
 * ------------
 * This module contains code to probe the FPGA for its installed cores.
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
#include <stdint.h>
#include <string.h>

#include "cryptech.h"

static struct core_info *tc_probe_cores(void)
{
    static struct core_info *head = NULL;
    struct core_info *tail = NULL, *node;
    off_t offset;

    if (head != NULL)
	return head;

    /* XXX could use unix linked-list macros */
    for (offset = 0; offset < 0x10000; offset += CORE_SIZE) {
        node = calloc(1, sizeof(struct core_info));
        if (node == NULL) {
            perror("malloc");
            goto fail;
        }

        if (tc_read(offset, (uint8_t *)&node->name[0], 8) ||
            tc_read(offset + 2, (uint8_t *)&node->version[0], 4)) {
            fprintf(stderr, "tc_read(%04x) error\n", (unsigned int)offset);
            free(node);
            goto fail;
        }
        if (node->name[0] == 0) {
            free(node);
            break;
        }

        node->base = offset;

        if (head == NULL) {
            head = tail = node;
        }
        else {
            tail->next = node;
            tail = node;
        }
    }

    return head;

fail:
    while (head) {
        node = head;
        head = node->next;
        free(node);
    }
    return NULL;
}

static struct core_info *tc_core_find(struct core_info *node, char *name)
{
    struct core_info *core;
    size_t len;

    if ((name == NULL) || (*name == '\0'))
	return node;

    len = strlen(name);
    for (core = node->next; core != NULL; core = core->next) {
	if (strncmp(core->name, name, len) == 0)
	    return core;
    }

    return NULL;
}

struct core_info *tc_core_first(char *name)
{
    struct core_info *head;

    head = tc_probe_cores();
    if (head == NULL)
	return NULL;

    return tc_core_find(head, name);
}

struct core_info *tc_core_next(struct core_info *node, char *name)
{
    if (node == NULL) {
	node = tc_core_first(name);
	if (node == NULL)
	    return NULL;
    }

    return tc_core_find(node->next, name);
}

off_t tc_core_base(char *name)
{
    struct core_info *node;

    node = tc_core_first(name);
    if (node == NULL)
	return 0;
    /* 0 is the base address for the "board-regs" core, installed
     * unconditionally at that address. Probing for any other core,
     * and getting 0, should be considered an error.
     */

    return node->base;
}
