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

/* Dead-simple fully-cooperative tasker. There are no priorities; tasks
 * are run in a strictly round-robin fashion. There is no preemption;
 * tasks explicitly yield control. Tasks are created at system init time,
 * and are expected to run an infinite loop; tasks do not return, nor are
 * tasks deleted.
 */

#include "stm-init.h"
#include "task.h"

/* Task Control Block. The structure is private, in case we want to change
 * it later without having to change the API. In any case, external code
 * shouldn't poke its fingers in the internal details.
 */
struct task_cb {
    struct task_cb *next;
    task_state_t state;

    char *name;
    funcp_t func;
    void *cookie;

    void *stack_base;
    size_t stack_len;
    void *stack_ptr;
};

/* Number of tasks. Default is number of RPC dispatch tasks, plus CLI task. */
#ifndef MAX_TASK
#ifdef NUM_RPC_TASK
#define MAX_TASK (NUM_RPC_TASK + 2)
#else
#define MAX_TASK 6
#endif
#endif

static tcb_t tcbs[MAX_TASK];
static size_t num_task = 0;

/* We have a circular list of tasks. New tasks are added at the tail, and
 * tail->next is the head.
 */
static tcb_t *tail = NULL;

/* Currently running task */
static tcb_t *cur_task = NULL;

#define STACK_GUARD_WORD 0x55AA5A5A

#ifdef DO_TASK_METRICS
static uint32_t tick_start = 0;
static uint32_t tick_idle  = 0;
static uint32_t tick_max   = 0;
static uint32_t nyield     = 0;
#endif

static uint32_t tick_prev  = 0;
#ifndef TASK_YIELD_THRESHOLD
#define TASK_YIELD_THRESHOLD 100
#endif

/* Add a task.
 */
tcb_t *task_add(char *name, funcp_t func, void *cookie, void *stack, size_t stack_len)
{
    if (num_task >= MAX_TASK)
        return NULL;

    if (name == NULL || func == NULL || stack == NULL)
        return NULL;

    tcb_t *t = &tcbs[num_task++];
    t->state = TASK_INIT;

    t->name = name;
    t->func = func;
    t->cookie = cookie;

    t->stack_base = stack;
    t->stack_len = stack_len;
    t->stack_ptr = stack + stack_len;

    for (uint32_t *p = (uint32_t *)t->stack_base; p < (uint32_t *)t->stack_ptr; ++p)
        *p = STACK_GUARD_WORD;

    if (tail == NULL) {
        /* Empty list; initialize it to this task. */
        t->next = t;
    }
    else {
        /* Otherwise insert at the end of the list. */
        t->next = tail->next;
        tail->next = t;
    }
    tail = t;

    return t;
}

/* Reinitalize the current task.
 * NOTE: This will destroy any state in the running task.
 * DO NOT CALL THIS UNLESS YOU ARE REALLY SURE THAT'S WHAT YOU WANT TO DO.
 */
void task_mod(char *name, funcp_t func, void *cookie)
{
    tcb_t *t = cur_task;
    t->name = name;
    t->func = func;
    t->cookie = cookie;
    t->state = TASK_INIT;
    t->stack_ptr = t->stack_base + t->stack_len;
    for (uint32_t *p = (uint32_t *)t->stack_base; p < (uint32_t *)t->stack_ptr; ++p)
        *p = STACK_GUARD_WORD;
    __set_MSP((uint32_t)cur_task->stack_ptr);
    task_yield();
}

/* Set the idle hook function pointer.
 *
 * This function is called repeatedly when the system is idle (there are
 * no runnable tasks).
 *
 * The idle function should NOT call task_delay or HAL_Delay, because that
 * will cause fatal recursion. We could add a recursion guard to
 * task_yield, but we're not currently using the idle hook, and I'm
 * thinking about removing it entirely.
 */
static void default_idle_hook(void) { }
static funcp_t idle_hook = default_idle_hook;
void task_set_idle_hook(funcp_t func)
{
    idle_hook = (func == NULL) ? default_idle_hook : func;
}

/* Find the next runnable task.
 */
static tcb_t *next_task(void)
{
    tcb_t *t;

    /* If the tasker isn't running yet, return the first task. */
    if (cur_task == NULL)
        return (tail == NULL) ? NULL : tail->next;

    // XXX critical section?

    /* find the next runnable task */
    for (t = cur_task->next; t != cur_task; t = t->next) {
        if (t->state != TASK_WAITING)
            return t;
    }

    /* searched all the way back to cur_task - is it runnable? */
    return (cur_task->state == TASK_WAITING) ? NULL : cur_task;
}

/* Check for stack overruns.
 */
static void check_stack(tcb_t *t)
{
    if (t->stack_ptr < t->stack_base ||
        t->stack_ptr >= t->stack_base + t->stack_len ||
        *(uint32_t *)t->stack_base != STACK_GUARD_WORD)
        Error_Handler();
}

/* Yield control to the next runnable task.
 */
void task_yield(void)
{
    tcb_t *next;

    /* If there are no defined tasks, exit immediately so we don't get
     * caught in the idle loop.
     */
    if (tail == NULL)
	return;

#ifdef DO_TASK_METRICS
    uint32_t tick0 = HAL_GetTick();
#endif

    /* Find the next runnable task. Loop if every task is waiting. */
    while (1) {
        next = next_task();
        if (next == NULL)
            idle_hook();
        else
            break;
    }

    /* If we decide we don't need the idle hook, the preceding loop could
     * devolve to something like this:
     *
     * do {
     *     next = next_task();
     * } while (next == NULL);
     */

#ifdef DO_TASK_METRICS
    uint32_t tick = HAL_GetTick();
    tick_idle += (tick - tick0);
    if (tick_start == 0)
        tick_start = tick;
    if (tick_prev != 0) {
        uint32_t duration = tick0 - tick_prev;
        if (duration > tick_max)
            tick_max = duration;
    }
    tick_prev = tick;
    ++nyield;
#else
    tick_prev = HAL_GetTick();
#endif

    /* If there are no other runnable tasks (and cur_task is runnable),
     * we don't need to context-switch.
     */
    if (next == cur_task && cur_task->state != TASK_INIT)
        return;

    /* Save current context, if there is one. */
    if (cur_task != NULL && cur_task->state != TASK_INIT) {
        __asm("push {r0-r12, lr}");
        cur_task->stack_ptr = (void *)__get_MSP();

        /* Check for stack overruns. */
        check_stack(cur_task);
    }

    cur_task = next;

    /* If task is in init state, call its entry point. */
    if (cur_task->state == TASK_INIT) {
        __set_MSP((uint32_t)cur_task->stack_ptr);
        cur_task->state = TASK_READY;
        cur_task->func();
        /*NOTREACHED*/
    }

    /* Otherwise, restore the task's context. */
    else {
        __set_MSP((uint32_t)cur_task->stack_ptr);
        __asm("pop {r0-r12, lr}");
        return;
    }
}

/* Yield if it's been "too long" since the last yield.
 */
void task_yield_maybe(void)
{
    if (HAL_GetTick() - tick_prev >= TASK_YIELD_THRESHOLD)
        task_yield();
}

/* Put the current task to sleep (make it non-runnable).
 */
void task_sleep(void)
{
    if (cur_task != NULL)
        cur_task->state = TASK_WAITING;

    task_yield();
}

/* Wake a task (make it runnable).
 */
void task_wake(tcb_t *t)
{
    if (t != NULL)
        t->state = TASK_READY;
}

/* Accessor functions */

tcb_t *task_get_tcb(void)
{
    return cur_task;
}

char *task_get_name(tcb_t *t)
{
    if (t == NULL)
        t = cur_task;

    return t->name;
}

funcp_t task_get_func(tcb_t *t)
{
    if (t == NULL)
        t = cur_task;

    return t->func;
}

void *task_get_cookie(tcb_t *t)
{
    if (t == NULL)
        t = cur_task;

    return t->cookie;
}

task_state_t task_get_state(tcb_t *t)
{
    if (t == NULL)
        t = cur_task;

    return t->state;
}

void *task_get_stack(tcb_t *t)
{
    if (t == NULL)
        t = cur_task;

    return t->stack_ptr;
}

/* stupid linear search for first non guard word */
size_t task_get_stack_highwater(tcb_t *t)
{
    if (t == NULL)
        t = cur_task;

    const uint32_t * const b = (uint32_t *)t->stack_base;

    for (size_t i = 0; i < t->stack_len/4; ++i) {
        if (b[i] != STACK_GUARD_WORD) {
            return (t->stack_len - (i * 4));
        }
    }

    return 0;
}    

/* Iterate through tasks.
 *
 * Returns the next task control block, or NULL at the end of the list.
 */
tcb_t *task_iterate(tcb_t *t)
{
    if (t == NULL)
        return (tail == NULL) ? NULL : tail->next;

    if (t == tail)
        return NULL;

    return t->next;
}

/* Delay a number of 1ms ticks.
 */
void task_delay(uint32_t delay)
{
    uint32_t tickstart = HAL_GetTick();

    while ((HAL_GetTick() - tickstart) < delay)
	task_yield();
}
void HAL_Delay(uint32_t delay) __attribute__((alias("task_delay")));

/* Simple mutex-like locks. A real mutex would require the unlocker to be
 * the current owner, but then we have to define and return errors, when
 * all we want at the moment is simple mutual exclusion.
 */
void task_mutex_lock(task_mutex_t *mutex)
{
    while (mutex->locked)
	task_yield();
    mutex->locked = 1;
}

void task_mutex_unlock(task_mutex_t *mutex)
{
    if (mutex != NULL)
	mutex->locked = 0;
}

#ifdef DO_TASK_METRICS
void task_get_metrics(struct task_metrics *tm)
{
    if (tm != NULL) {
        tm->avg.tv_sec  = 0;
        tm->avg.tv_usec = (HAL_GetTick() - tick_start - tick_idle) * 1000 / nyield;
        if (tm->avg.tv_usec > 1000000) {
            tm->avg.tv_sec  = tm->avg.tv_usec / 1000000;
            tm->avg.tv_usec = tm->avg.tv_usec % 1000000;
        }
        tm->max.tv_sec  = tick_max / 1000;
        tm->max.tv_usec = (tick_max % 1000) * 1000;
    }
}

void task_reset_metrics(void)
{
    tick_start = HAL_GetTick();
    tick_prev = tick_idle = tick_max = nyield = 0;
}
#endif
