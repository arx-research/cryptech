/*
 * mgmt-task.c
 * -----------
 * CLI 'task' functions.
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

/*
 * Show the active tasks. This is mostly for debugging, and looks deeply
 * into OS-level structures, but sometimes you just need to know...
 */

#include "mgmt-cli.h"
#include "mgmt-task.h"
#include "task.h"

static char *task_state[] = {
    "INIT",
    "WAITING",
    "READY"
};

extern size_t request_queue_len(void);
extern size_t request_queue_max(void);

static int cmd_task_show(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;
    argv = argv;
    argc = argc;

    cli_print(cli, "name            state           stack high water");
    cli_print(cli, "--------        --------        ----------------");

    for (tcb_t *t = task_iterate(NULL); t != NULL; t = task_iterate(t)) {
        cli_print(cli, "%-15s %-15s %d",
                  task_get_name(t),
                  task_state[task_get_state(t)],
                  task_get_stack_highwater(t));
    }

    cli_print(cli, " ");
    cli_print(cli, "RPC request queue current length: %u", request_queue_len());
    cli_print(cli, "RPC request queue maximum length: %u", request_queue_max());

    extern size_t uart_rx_max;
    cli_print(cli, " ");
    cli_print(cli, "UART receive queue maximum length: %u", uart_rx_max);

    return CLI_OK;
}

#ifdef DO_TASK_METRICS
static int cmd_task_show_metrics(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    command = command;
    argv = argv;
    argc = argc;

    struct task_metrics tm;

    task_get_metrics(&tm);

    cli_print(cli, "avg time between yields: %ld.%06ld sec", tm.avg.tv_sec, tm.avg.tv_usec);
    cli_print(cli, "max time between yields: %ld.%06ld sec", tm.max.tv_sec, tm.max.tv_usec);

    return CLI_OK;
}

static int cmd_task_reset_metrics(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    cli = cli;
    command = command;
    argv = argv;
    argc = argc;

    task_reset_metrics();

    return CLI_OK;
}
#endif

void configure_cli_task(struct cli_def *cli)
{
    struct cli_command *c = cli_register_command(cli, NULL, "task", NULL, 0, 0, NULL);

    /* task show */
#ifdef DO_TASK_METRICS
    struct cli_command *c_show =
#endif
    cli_register_command(cli, c, "show", cmd_task_show, 0, 0, "Show the active tasks");

#ifdef DO_TASK_METRICS
    /* task show metrics */
    cli_register_command(cli, c_show, "metrics", cmd_task_show_metrics, 0, 0, "Show task metrics");

    /* task reset */
    struct cli_command *c_reset = cli_register_command(cli, c, "reset", NULL, 0, 0, NULL);

    /* task reset metrics */
    cli_register_command(cli, c_reset, "metrics", cmd_task_reset_metrics, 0, 0, "Reset task metrics");
#endif
}
