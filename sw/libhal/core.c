/*
 * core.c
 * ------
 * This module contains code to probe the FPGA for its installed cores.
 *
 * Author: Paul Selkirk, Rob Austein
 * Copyright (c) 2015-2017, NORDUnet A/S All rights reserved.
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

#include "hal.h"
#include "hal_internal.h"

/*
 * POSIX function whose declaration gets lost somewhere in the twisty
 * corridors of glibc's "Feature Test Macro" system.
 */

extern size_t strnlen(const char *, size_t);

/*
 * Structure of our internal database is private, in case we want to
 * change representation (array, tree, list of lists, whatever) at
 * some later date without having to change the public API.
 */

struct hal_core {
  hal_core_info_t info;
  int busy;
  hal_core_lru_t lru;
  struct hal_core *next;
};

#if defined(HAL_STATIC_CORE_STATE_BLOCKS) && HAL_STATIC_CORE_STATE_BLOCKS > 0
static hal_core_t core_table[HAL_STATIC_CORE_STATE_BLOCKS];
#else
#error HAL_STATIC_CORE_STATE_BLOCKS must be defined as a positive integer
#endif

/*
 * Check whether a core's name matches a particular string.  This is a
 * bit nasty due to non-null-terminated fixed-length names.
 */

static inline int name_matches(const hal_core_t *const core, const char * const name)
{
  return (core != NULL && name != NULL && *name != '\0' &&
          strncmp(name, core->info.name, strnlen(name, sizeof(core->info.name))) == 0);
}

/*
 * Probe the FPGA and build our internal database.
 *
 * At the moment this knows far more than it should about pecularities
 * of certain cores.  In theory at least some of this will be fixed
 * soon on the Verilog side.  Adding a core-length word to the core
 * header sure would make this simpler.
 */

#define CORE_MIN                0
#define	CORE_MAX                0x10000
#define	CORE_SIZE               0x100

static hal_core_t *head = NULL;
static hal_core_lru_t lru = 0;

static inline hal_core_t *probe_cores(void)
{
  /* Extra space to leave after particular cores.  Yummy. */
  static const struct { const char *name; hal_addr_t extra; } gaps[] = {
    { "csprng",  11 * CORE_SIZE }, /* empty slots after csprng */
    { "modexps6", 3 * CORE_SIZE }, /* ModexpS6 uses four slots */
    { "modexpa7", 7 * CORE_SIZE }, /* ModexpA7 uses eight slots */
  };

  if (offsetof(hal_core_t, info) != 0)
    return NULL;                /* Paranoia, see hal.h */

  if (head != NULL)
    return head;

  hal_core_t **tail = &head;
  hal_error_t err = HAL_OK;
  hal_addr_t addr;
  int n;

  for (addr = CORE_MIN,    n = 0;
       addr < CORE_MAX  && n < HAL_STATIC_CORE_STATE_BLOCKS;
       addr += CORE_SIZE,  n++) {

    hal_core_t *core = &core_table[n];
    memset(core, 0, sizeof(*core));
    core->info.base = addr;

    if ((err = hal_io_read(core, ADDR_NAME0,   (uint8_t *) core->info.name,    8)) != HAL_OK ||
        (err = hal_io_read(core, ADDR_VERSION, (uint8_t *) core->info.version, 4)) != HAL_OK)
      goto fail;

    if (core->info.name[0] == 0x00 || core->info.name[0] == 0xff)
      continue;

    for (size_t i = 0; i < sizeof(gaps)/sizeof(*gaps); i++) {
      if (name_matches(core, gaps[i].name)) {
        addr += gaps[i].extra;
        break;
      }
    }

    *tail = core;
    tail = &core->next;
  }

  return head;

 fail:
  memset(core_table, 0, sizeof(core_table));
  return NULL;
}

void hal_core_reset_table(void)
{
  head = NULL;
  memset(core_table, 0, sizeof(core_table));
}

hal_core_t * hal_core_iterate(hal_core_t *core)
{
  return core == NULL ? probe_cores() : core->next;
}

hal_core_t *hal_core_find(const char *name, hal_core_t *core)
{
  for (core = hal_core_iterate(core); core != NULL; core = core->next)
    if (name_matches(core, name))
      return core;
  return NULL;
}

/*
 * If caller specifies a non-NULL core value, we fail unless that core
 * is available and has the right name and lru values.
 *
 * If caller specifies a NULL core value, we take any free core with
 * the right name.
 *
 * Modification of lru field is handled by the jacket routines, to
 * avoid premature updates.
 */

static hal_error_t hal_core_alloc_no_wait(const char *name, hal_core_t **pcore, hal_core_lru_t *pomace)
{
  if (name == NULL || pcore == NULL || (*pcore != NULL && pomace == NULL))
    return HAL_ERROR_BAD_ARGUMENTS;

  hal_error_t err = HAL_ERROR_CORE_NOT_FOUND;
  hal_core_t *core = *pcore;
  hal_core_t *best = NULL;
  uint32_t age = 0;

  hal_critical_section_start();

  /*
   * User wants to reuse previous core, grab that core or or bust.
   * Never return CORE_BUSY in this case, because busy implies
   * somebody else has touched it.  Checking the name in this case
   * isn't strictly necessary, but it's cheap insurance.
   */
  if (core != NULL) {
    if (core->busy || core->lru != *pomace)
      err = HAL_ERROR_CORE_REASSIGNED;
    else if (!name_matches(core, name))
      err = HAL_ERROR_CORE_NOT_FOUND;
    else {
      core->busy = 1;
      err = HAL_OK;
    }
  }

  /*
   * User just wants a core with the right name, search for least recently used matching core.
   */
  else {
    for (core = hal_core_find(name, NULL); core != NULL; core = hal_core_find(name, core)) {
      if (core->busy && err == HAL_ERROR_CORE_NOT_FOUND) {
        err = HAL_ERROR_CORE_BUSY;
      }
      else if (!core->busy && (lru - core->lru) >= age) {
        best = core;
        age = (lru - core->lru);
      }
    }
    if (best != NULL) {
      *pcore = best;
      best->busy = 1;
      err = HAL_OK;
    }
  }

  hal_critical_section_end();

  return err;
}

hal_error_t hal_core_alloc(const char *name, hal_core_t **pcore, hal_core_lru_t *pomace)
{
  hal_error_t err;

  while ((err = hal_core_alloc_no_wait(name, pcore, pomace)) == HAL_ERROR_CORE_BUSY)
    hal_task_yield();

  if (err != HAL_OK)
    return err;

  (*pcore)->lru = ++lru;

  if (pomace != NULL)
    *pomace = (*pcore)->lru;

  return HAL_OK;
}

hal_error_t hal_core_alloc2(const char *name1, hal_core_t **pcore1, hal_core_lru_t *pomace1,
                            const char *name2, hal_core_t **pcore2, hal_core_lru_t *pomace2)
{
  const int clear = pcore1 != NULL && *pcore1 == NULL;

  for (;;) {

    hal_error_t err = hal_core_alloc_no_wait(name1, pcore1, pomace1);

    switch (err) {

    case HAL_OK:
      break;

    case HAL_ERROR_CORE_BUSY:
      hal_task_yield();
      continue;

    default:
      return err;
    }

    if ((err = hal_core_alloc_no_wait(name2, pcore2, pomace2)) == HAL_OK)
      break;

    hal_core_free(*pcore1);     /* hal_core_free does a yield, so we don't need to do another one */

    if (clear)                  /* put *pcore1 back as we found it */
      *pcore1 = NULL;

    if (err != HAL_ERROR_CORE_BUSY)
      return err;
  }

  (*pcore1)->lru = ++lru;
  (*pcore2)->lru = ++lru;

  if (pomace1 != NULL)
    *pomace1 = (*pcore1)->lru;

  if (pomace2 != NULL)
    *pomace2 = (*pcore2)->lru;

  return HAL_OK;
}

void hal_core_free(hal_core_t *core)
{
  if (core != NULL) {
    hal_critical_section_start();
    core->busy = 0;
    hal_critical_section_end();
    hal_task_yield();
  }
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
