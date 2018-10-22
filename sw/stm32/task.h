/*
 * task.c
 * ----------------
 * Simple cooperative tasking system.
 *
 * Copyright (c) 2017, NORDUnet A/S All rights reserved.
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

#ifndef _TASK_H_
#define _TASK_H_

#include <sys/types.h>
#include <stdint.h>

typedef enum task_state {
    TASK_INIT,
    TASK_WAITING,
    TASK_READY
} task_state_t;

typedef struct task_cb tcb_t;

typedef struct { unsigned locked; } task_mutex_t;

typedef void (*funcp_t)(void);

extern tcb_t *task_add(char *name, funcp_t func, void *cookie, void *stack, size_t stack_len);
extern void task_mod(char *name, funcp_t func, void *cookie);

extern void task_set_idle_hook(funcp_t func);

extern void task_yield(void);
extern void task_yield_maybe(void);
extern void task_sleep(void);
extern void task_wake(tcb_t *t);

extern tcb_t *task_get_tcb(void);
extern char *task_get_name(tcb_t *t);
extern funcp_t task_get_func(tcb_t *t);
extern void *task_get_cookie(tcb_t *t);
extern task_state_t task_get_state(tcb_t *t);
extern void *task_get_stack(tcb_t *t);
extern size_t task_get_stack_highwater(tcb_t *t);

extern tcb_t *task_iterate(tcb_t *t);

extern void task_delay(uint32_t delay);

extern void task_mutex_lock(task_mutex_t *mutex);
extern void task_mutex_unlock(task_mutex_t *mutex);

#ifdef DO_TASK_METRICS
#include <sys/time.h>

struct task_metrics {
    struct timeval avg, max;
};

void task_get_metrics(struct task_metrics *tm);
void task_reset_metrics(void);
#endif

#endif /* _TASK_H_ */
