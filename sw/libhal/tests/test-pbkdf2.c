/*
 * test-pbkdf2.c
 * -------------
 * Test program for PBKDF2.
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

#include <hal.h>

/* PBKDF2 HMAC-SHA-1 test cases from RFC 6070. */

/* 'password' */
static const uint8_t pbkdf2_tc_1_password[] = { /* 8 bytes */
  0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64
};

/* 'salt' */
static const uint8_t pbkdf2_tc_1_salt[] = { /* 4 bytes */
  0x73, 0x61, 0x6c, 0x74
};

static const unsigned pbkdf2_tc_1_count = 1;

static const uint8_t pbkdf2_tc_1_DK[] = { /* 20 bytes */
  0x0c, 0x60, 0xc8, 0x0f, 0x96, 0x1f, 0x0e, 0x71, 0xf3, 0xa9, 0xb5, 0x24,
  0xaf, 0x60, 0x12, 0x06, 0x2f, 0xe0, 0x37, 0xa6
};

/* 'password' */
static const uint8_t pbkdf2_tc_2_password[] = { /* 8 bytes */
  0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64
};

/* 'salt' */
static const uint8_t pbkdf2_tc_2_salt[] = { /* 4 bytes */
  0x73, 0x61, 0x6c, 0x74
};

static const unsigned pbkdf2_tc_2_count = 2;

static const uint8_t pbkdf2_tc_2_DK[] = { /* 20 bytes */
  0xea, 0x6c, 0x01, 0x4d, 0xc7, 0x2d, 0x6f, 0x8c, 0xcd, 0x1e, 0xd9, 0x2a,
  0xce, 0x1d, 0x41, 0xf0, 0xd8, 0xde, 0x89, 0x57
};

/* 'password' */
static const uint8_t pbkdf2_tc_3_password[] = { /* 8 bytes */
  0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64
};

/* 'salt' */
static const uint8_t pbkdf2_tc_3_salt[] = { /* 4 bytes */
  0x73, 0x61, 0x6c, 0x74
};

static const unsigned pbkdf2_tc_3_count = 4096;

static const uint8_t pbkdf2_tc_3_DK[] = { /* 20 bytes */
  0x4b, 0x00, 0x79, 0x01, 0xb7, 0x65, 0x48, 0x9a, 0xbe, 0xad, 0x49, 0xd9,
  0x26, 0xf7, 0x21, 0xd0, 0x65, 0xa4, 0x29, 0xc1
};

/* 'password' */
static const uint8_t pbkdf2_tc_4_password[] = { /* 8 bytes */
  0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64
};

/* 'salt' */
static const uint8_t pbkdf2_tc_4_salt[] = { /* 4 bytes */
  0x73, 0x61, 0x6c, 0x74
};

static const unsigned pbkdf2_tc_4_count = 16777216;

static const uint8_t pbkdf2_tc_4_DK[] = { /* 20 bytes */
  0xee, 0xfe, 0x3d, 0x61, 0xcd, 0x4d, 0xa4, 0xe4, 0xe9, 0x94, 0x5b, 0x3d,
  0x6b, 0xa2, 0x15, 0x8c, 0x26, 0x34, 0xe9, 0x84
};

/* 'passwordPASSWORDpassword' */
static const uint8_t pbkdf2_tc_5_password[] = { /* 24 bytes */
  0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64, 0x50, 0x41, 0x53, 0x53,
  0x57, 0x4f, 0x52, 0x44, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6f, 0x72, 0x64
};

/* 'saltSALTsaltSALTsaltSALTsaltSALTsalt' */
static const uint8_t pbkdf2_tc_5_salt[] = { /* 36 bytes */
  0x73, 0x61, 0x6c, 0x74, 0x53, 0x41, 0x4c, 0x54, 0x73, 0x61, 0x6c, 0x74,
  0x53, 0x41, 0x4c, 0x54, 0x73, 0x61, 0x6c, 0x74, 0x53, 0x41, 0x4c, 0x54,
  0x73, 0x61, 0x6c, 0x74, 0x53, 0x41, 0x4c, 0x54, 0x73, 0x61, 0x6c, 0x74
};

static const unsigned pbkdf2_tc_5_count = 4096;

static const uint8_t pbkdf2_tc_5_DK[] = { /* 25 bytes */
  0x3d, 0x2e, 0xec, 0x4f, 0xe4, 0x1c, 0x84, 0x9b, 0x80, 0xc8, 0xd8, 0x36,
  0x62, 0xc0, 0xe4, 0x4a, 0x8b, 0x29, 0x1a, 0x96, 0x4c, 0xf2, 0xf0, 0x70, 0x38
};

/* 'pass\x00word' */
static const uint8_t pbkdf2_tc_6_password[] = { /* 9 bytes */
  0x70, 0x61, 0x73, 0x73, 0x00, 0x77, 0x6f, 0x72, 0x64
};

/* 'sa\x00lt' */
static const uint8_t pbkdf2_tc_6_salt[] = { /* 5 bytes */
  0x73, 0x61, 0x00, 0x6c, 0x74
};

static const unsigned pbkdf2_tc_6_count = 4096;

static const uint8_t pbkdf2_tc_6_DK[] = { /* 16 bytes */
  0x56, 0xfa, 0x6a, 0xa7, 0x55, 0x48, 0x09, 0x9d, 0xcc, 0x37, 0xd7, 0xf0,
  0x34, 0x25, 0xe0, 0xc3
};

static void print_hex(const uint8_t * const val, const size_t len)
{
  size_t i;
  for (i = 0; i < len; i++)
    printf(" %02x", val[i]);
}

static int _test_pbkdf2(hal_core_t *core,
                        const uint8_t * const pwd,  const size_t pwd_len,
                        const uint8_t * const salt, const size_t salt_len,
                        const uint8_t * const dk,   const size_t dk_len,
                        const unsigned count, const char * const label)
{
  printf("Starting PBKDF2 test case %s\n", label);

  uint8_t result[dk_len];

  hal_error_t err = hal_pbkdf2(core, hal_hash_sha1, pwd, pwd_len, salt, salt_len,
                               result, dk_len, count);

  if (err != HAL_OK) {
    printf("hal_pbkdf2() failed: %s\n", hal_error_string(err));
    return 0;
  }

  printf("Comparing result with known value\n");

  if (memcmp(dk, result, dk_len)) {
    printf("MISMATCH\nExpected:");
    print_hex(dk, dk_len);
    printf("\nGot:     ");
    print_hex(result, dk_len);
    printf("\n");
    return 0;
  }

  else {
    printf("OK\n");
    return 1;
  }
}

#define test_pbkdf2(_n_) \
  _test_pbkdf2(core,                                                            \
               pbkdf2_tc_##_n_##_password, sizeof(pbkdf2_tc_##_n_##_password),  \
               pbkdf2_tc_##_n_##_salt,     sizeof(pbkdf2_tc_##_n_##_salt),      \
               pbkdf2_tc_##_n_##_DK,       sizeof(pbkdf2_tc_##_n_##_DK),        \
               pbkdf2_tc_##_n_##_count,    #_n_)

int main(void)
{
  hal_core_t *core = hal_core_find(SHA1_NAME, NULL);
  int ok = 1;

  if (core == NULL)
    printf("SHA-1 core not present, not testing PBKDF2\n");

  else {
    ok &= test_pbkdf2(1);
    ok &= test_pbkdf2(2);
    ok &= test_pbkdf2(3);
    ok &= test_pbkdf2(4);
    ok &= test_pbkdf2(5);
    ok &= test_pbkdf2(6);
  }

    return !ok;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
