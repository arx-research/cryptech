/*
 * test-aes-key-wrap.c
 * -------------------
 * Test code for AES Key Wrap.
 *
 * Authors: Rob Austein
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
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <hal.h>

#ifndef TC_BUFSIZE
#define TC_BUFSIZE      4096
#endif

/*
 * Test cases from RFC 5649 all use a 192-bit key, which our AES
 * implementation doesn't support, so had to write our own.
 */

static const uint8_t Q[] = { /* Plaintext, 81 bytes */
  0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x21, 0x20, 0x20, 0x4d, 0x79, 0x20, 0x6e,
  0x61, 0x6d, 0x65, 0x20, 0x69, 0x73, 0x20, 0x49, 0x6e, 0x69, 0x67, 0x6f,
  0x20, 0x4d, 0x6f, 0x6e, 0x74, 0x6f, 0x79, 0x61, 0x2e, 0x20, 0x20, 0x59,
  0x6f, 0x75, 0x20, 0x62, 0x72, 0x6f, 0x6b, 0x65, 0x20, 0x6d, 0x79, 0x20,
  0x41, 0x45, 0x53, 0x20, 0x6b, 0x65, 0x79, 0x20, 0x77, 0x72, 0x61, 0x70,
  0x70, 0x65, 0x72, 0x2e, 0x20, 0x20, 0x50, 0x72, 0x65, 0x70, 0x61, 0x72,
  0x65, 0x20, 0x74, 0x6f, 0x20, 0x64, 0x69, 0x65, 0x2e
};

static const uint8_t K_128[] = { /* 128-bit KEK, 16 bytes */
  0xbc, 0x2a, 0xd8, 0x90, 0xd8, 0x91, 0x10, 0x65, 0xf0, 0x42, 0x10, 0x1b,
  0x4a, 0x6b, 0xaf, 0x99
};

static const uint8_t K_256[] = { /* 256-bit KEK, 32 bytes */
  0xe3, 0x97, 0x52, 0x81, 0x2b, 0x7e, 0xc2, 0xa4, 0x6a, 0xac, 0x50, 0x18,
  0x0d, 0x10, 0xc6, 0x85, 0x2c, 0xcf, 0x86, 0x0a, 0xa9, 0x4f, 0x69, 0xab,
  0x16, 0xa6, 0x4f, 0x3e, 0x96, 0xa0, 0xbd, 0x9e
};

static const uint8_t C_128[] = { /* Plaintext wrapped by 128-bit KEK, 96 bytes */
  0xb0, 0x10, 0x91, 0x7b, 0xe7, 0x67, 0x9c, 0x10, 0x16, 0x64, 0xe7, 0x73,
  0xd2, 0x68, 0xba, 0xed, 0x8c, 0x50, 0x49, 0x80, 0x16, 0x2f, 0x4e, 0x97,
  0xe8, 0x45, 0x5c, 0x2f, 0x2b, 0x7a, 0x88, 0x0e, 0xd8, 0xef, 0xaa, 0x40,
  0xb0, 0x2e, 0xb4, 0x50, 0xe7, 0x60, 0xf7, 0xbb, 0xed, 0x56, 0x79, 0x16,
  0x65, 0xb7, 0x13, 0x9b, 0x4c, 0x66, 0x86, 0x5f, 0x4d, 0x53, 0x2d, 0xcd,
  0x83, 0x41, 0x01, 0x35, 0x0d, 0x06, 0x39, 0x4e, 0x9e, 0xfe, 0x68, 0xc5,
  0x2f, 0x37, 0x33, 0x99, 0xbb, 0x88, 0xf7, 0x76, 0x1e, 0x82, 0x48, 0xd6,
  0xa2, 0xf3, 0x9b, 0x92, 0x01, 0x65, 0xcb, 0x48, 0x36, 0xf5, 0x42, 0xd3
};

static const uint8_t C_256[] = { /* Plaintext wrapped by 256-bit KEK, 96 bytes */
  0x08, 0x00, 0xbc, 0x1b, 0x35, 0xe4, 0x2a, 0x69, 0x3f, 0x43, 0x07, 0x54,
  0x31, 0xba, 0xb6, 0x89, 0x7c, 0x64, 0x9f, 0x03, 0x84, 0xc4, 0x4a, 0x71,
  0xdb, 0xcb, 0xae, 0x55, 0x30, 0xdf, 0xb0, 0x2b, 0xc3, 0x91, 0x5d, 0x07,
  0xa9, 0x24, 0xdb, 0xe7, 0xbe, 0x4d, 0x0d, 0x62, 0xd4, 0xf8, 0xb1, 0x94,
  0xf1, 0xb9, 0x22, 0xb5, 0x94, 0xab, 0x7e, 0x0b, 0x15, 0x6a, 0xd9, 0x5f,
  0x6c, 0x20, 0xb7, 0x7e, 0x13, 0x19, 0xfa, 0xc4, 0x70, 0xec, 0x0d, 0xbd,
  0xf7, 0x01, 0xc6, 0xb3, 0x9a, 0x19, 0xaf, 0xf2, 0x47, 0x68, 0xea, 0x7e,
  0x97, 0x7e, 0x52, 0x2e, 0xd4, 0x03, 0x31, 0xcb, 0x22, 0xb6, 0xfe, 0xf5
};

static const char *format_hex(const uint8_t *bin, const size_t len, char *hex, const size_t max)
{
  size_t i;

  assert(bin != NULL && hex != NULL && len * 3 < max);

  if (len == 0)
    return "";

  for (i = 0; i < len; i++)
    sprintf(hex + 3 * i, "%02x:", bin[i]);

  hex[len * 3 - 1] = '\0';
  return hex;
}

static int run_test(hal_core_t *core,
                    const uint8_t * const K, const size_t K_len,
                    const uint8_t * const C, const size_t C_len)
{
  const size_t Q_len = sizeof(Q);
  uint8_t q[TC_BUFSIZE], c[TC_BUFSIZE];
  size_t q_len = sizeof(q), c_len = sizeof(c);
  char h1[TC_BUFSIZE * 3 + 1],  h2[TC_BUFSIZE * 3 + 1];
  hal_error_t err;
  int ok1 = 1, ok2 = 1;

  /*
   * Wrap and compare results.
   */

  printf("Wrapping with %lu-bit KEK...\n", (unsigned long) K_len * 8);
  if ((err = hal_aes_keywrap(core, K, K_len, Q, Q_len, c, &c_len)) != HAL_OK) {
    printf("Couldn't wrap with %lu-bit KEK: %s\n",
           (unsigned long) K_len * 8, hal_error_string(err));
    ok1 = 0;
  }
  else if (C_len != c_len || memcmp(C, c, C_len) != 0) {
    printf("Ciphertext mismatch:\n  Want: %s\n  Got:  %s\n",
           format_hex(C, C_len, h1, sizeof(h1)),
           format_hex(c, c_len, h2, sizeof(h2)));
    ok1 = 0;
  }
  else {
    printf("OK\n");
  }

  /*
   * Unwrap and compare results.
   */

  printf("Unwrapping with %lu-bit KEK...\n", (unsigned long) K_len * 8);
  if ((err = hal_aes_keyunwrap(core, K, K_len, C, C_len, q, &q_len)) != HAL_OK) {
    printf("Couldn't unwrap with %lu-bit KEK: %s\n",
           (unsigned long) K_len * 8, hal_error_string(err));
    ok2 = 0;
  }
  else if (Q_len != q_len || memcmp(Q, q, Q_len) != 0) {
    printf("Plaintext mismatch:\n  Want: %s\n  Got:  %s\n",
           format_hex(Q, Q_len, h1, sizeof(h1)),
           format_hex(q, q_len, h2, sizeof(h2)));
    ok2 = 0;
  }
  else {
    printf("OK\n");
  }

  return ok1 && ok2;
}

int main(void)
{
  int failures = 0;

  printf("Testing whether AES core reports present...");

  hal_core_t *core = hal_core_find(AES_CORE_NAME, NULL);

  if (core == NULL) {
    printf("no, skipping keywrap tests\n");
  }

  else {
    printf("yes\n");
    if (!run_test(core, K_128, sizeof(K_128), C_128, sizeof(C_128)))
      failures++;
    if (!run_test(core, K_256, sizeof(K_256), C_256, sizeof(C_256)))
      failures++;
  }

  return failures;
}

/*
 * "Any programmer who fails to comply with the standard naming, formatting,
 *  or commenting conventions should be shot.  If it so happens that it is
 *  inconvenient to shoot him, then he is to be politely requested to recode
 *  his program in adherence to the above standard."
 *                      -- Michael Spier, Digital Equipment Corporation
 *
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
