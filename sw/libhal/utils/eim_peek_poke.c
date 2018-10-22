/*
 * eim_peek_poke.c
 * ---------------
 * Read or write a 32-bit word via the EIM bus.  This is mostly
 * intended for use as part of the FPGA initialization sequence.
 *
 * Authors: Paul Selkirk, Rob Austein
 * Copyright (c) 2015, NORDUnet A/S
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "novena-eim.h"

#define string_match(...) \
  _string_match(__VA_ARGS__, NULL)

static int _string_match(const char *s1, ...)
{
  const char *s2;

  if (s1 == NULL)
    return 0;

  va_list ap;
  va_start(ap, s1);
  for (s2 = va_arg(ap, const char *); s2 != NULL; s2 = va_arg(ap, const char *))
    if (!strcmp(s1, s2))
      break;
  va_end(ap);

  return s2 != NULL;
}

static int parse_value(const char *s, uint32_t *value)
{
  if (s == NULL || value == NULL)
    return 0;

  char *e;
  *value = strtoul(s, &e, 0);

  return *s != '\0' && *e == '\0';
}

static int parse_offset(const char *s, off_t *offset)
{
  uint32_t value;

  if (offset == NULL || !parse_value(s, &value))
    return 0;

  *offset = (off_t) value;
  return 1;
}

static void usage(const int code, const char *jane)
{
  FILE *f = code ? stderr : stdout;
  fprintf(f, "usage: %s { --read  offset | --write offset value }\n", jane);
  exit(code);
}

int main(int argc, char *argv[])
{
  off_t offset = 0;
  uint32_t value;

  if (argc == 1 || string_match(argv[1], "-?", "-h", "--help"))
    usage(EXIT_SUCCESS, argv[0]);

  if (eim_setup() != 0) {
    fprintf(stderr, "EIM setup failed\n");
    return EXIT_FAILURE;
  }

  if (string_match(argv[1], "r", "-r", "--read", "--peek")) {
    if (argc != 3 || !parse_offset(argv[2], &offset))
      usage(EXIT_FAILURE, argv[0]);
    eim_read_32(offset, &value);
    printf("%08x\n", value);
  }

  else if (string_match(argv[1], "w", "-w", "--write", "--poke")) {
    if (argc != 4 || !parse_offset(argv[2], &offset) || !parse_value(argv[3], &value))
      usage(EXIT_FAILURE, argv[0]);
    eim_write_32(offset, &value);
    exit(EXIT_SUCCESS);
  }

  else {
    usage(EXIT_FAILURE, argv[0]);
  }

  return EXIT_SUCCESS;
}
