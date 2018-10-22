/*
 * Implementation of RFC 5649 variant of AES Key Wrap, using Cryptlib
 * to supply the AES ECB encryption and decryption functions.
 *
 * Note that there are two different block sizes involved here: the
 * key wrap algorithm deals entirely with 64-bit blocks, while AES
 * itself deals with 128-bit blocks.  In practice, this is not as
 * confusing as it sounds, because we combine two 64-bit blocks to
 * create one 128-bit block just prior to performing an AES operation,
 * then split the result back to 64-bit blocks immediately afterwards.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <cryptlib.h>

#include "aes_keywrap.h"

aes_key_wrap_status_t aes_key_wrap(const CRYPT_CONTEXT K,
                                   const unsigned char * const Q,
                                   const size_t m,
                                   unsigned char *C,
                                   size_t *C_len)
{
  unsigned char aes_block[16];
  unsigned long n;
  long i, j;

  assert(AES_KEY_WRAP_CIPHERTEXT_SIZE(m) % 8 == 0);

  if (Q == NULL || C == NULL || C_len == NULL || *C_len < AES_KEY_WRAP_CIPHERTEXT_SIZE(m))
    return AES_KEY_WRAP_BAD_ARGUMENTS;

  *C_len = AES_KEY_WRAP_CIPHERTEXT_SIZE(m);

  memmove(C + 8, Q, m);
  if (m % 8 != 0)
    memset(C + 8 + m, 0, 8 -  (m % 8));
  C[0] = 0xA6;
  C[1] = 0x59;
  C[2] = 0x59;
  C[3] = 0xA6;
  C[4] = (m >> 24) & 0xFF;
  C[5] = (m >> 16) & 0xFF;
  C[6] = (m >>  8) & 0xFF;
  C[7] = (m >>  0) & 0xFF;

  n = (AES_KEY_WRAP_CIPHERTEXT_SIZE(m) / 8) - 1;

  if (n == 1) {
    if (cryptEncrypt(K, C, 16) != CRYPT_OK)
      return AES_KEY_WRAP_ENCRYPTION_FAILED;
  }

  else {
    for (j = 0; j <= 5; j++) {
      for (i = 1; i <= n; i++) {
        unsigned long t = n * j + i;
        memcpy(aes_block + 0, C,         8);
        memcpy(aes_block + 8, C + i * 8, 8);
        if (cryptEncrypt(K, aes_block, sizeof(aes_block)) != CRYPT_OK)
          return AES_KEY_WRAP_ENCRYPTION_FAILED;
        memcpy(C,         aes_block + 0, 8);
        memcpy(C + i * 8, aes_block + 8, 8);
        C[7] ^= t & 0xFF; t >>= 8;
        C[6] ^= t & 0xFF; t >>= 8;
        C[5] ^= t & 0xFF; t >>= 8;
        C[4] ^= t & 0xFF;
      }
    }
  }

  return AES_KEY_WRAP_OK;
}

aes_key_wrap_status_t aes_key_unwrap(const CRYPT_CONTEXT K,
                                     const unsigned char * const C,
                                     const size_t C_len,
                                     unsigned char *Q,
                                     size_t *Q_len)
{
  unsigned char aes_block[16];
  unsigned long n;
  long i, j;
  size_t m;

  if (C == NULL || Q == NULL || C_len % 8 != 0 || C_len < 16 || Q_len == NULL || *Q_len < C_len)
    return AES_KEY_WRAP_BAD_ARGUMENTS;

  n = (C_len / 8) - 1;

  if (Q != C)
    memmove(Q, C, C_len);

  if (n == 1) {
    if (cryptDecrypt(K, Q, 16) != CRYPT_OK)
      return AES_KEY_WRAP_DECRYPTION_FAILED;
  }

  else {
    for (j = 5; j >= 0; j--) {
      for (i = n; i >= 1; i--) {
        unsigned long t = n * j + i;
        Q[7] ^= t & 0xFF; t >>= 8;
        Q[6] ^= t & 0xFF; t >>= 8;
        Q[5] ^= t & 0xFF; t >>= 8;
        Q[4] ^= t & 0xFF;
        memcpy(aes_block + 0, Q,         8);
        memcpy(aes_block + 8, Q + i * 8, 8);
        if (cryptDecrypt(K, aes_block, sizeof(aes_block)) != CRYPT_OK)
          return AES_KEY_WRAP_DECRYPTION_FAILED;
        memcpy(Q,         aes_block + 0, 8);
        memcpy(Q + i * 8, aes_block + 8, 8);
      }
    }
  }

  if (Q[0] != 0xA6 || Q[1] != 0x59 || Q[2] != 0x59 || Q[3] != 0xA6)
    return AES_KEY_WRAP_BAD_MAGIC;

  m = (((((Q[4] << 8) + Q[5]) << 8) + Q[6]) << 8) + Q[7];

  if (m <= 8 * (n - 1) || m > 8 * n)
    return AES_KEY_WRAP_BAD_LENGTH;

  if (m % 8 != 0)
    for (i = m + 8; i < 8 * (n + 1); i++)
      if (Q[i] != 0x00)
        return AES_KEY_WRAP_BAD_PADDING;

  *Q_len = m;

  memmove(Q, Q + 8, m);

  return AES_KEY_WRAP_OK;
}

const char *aes_key_wrap_error_string(const aes_key_wrap_status_t code)
{
  switch (code) {
  case AES_KEY_WRAP_OK:                 return "OK";
  case AES_KEY_WRAP_BAD_ARGUMENTS:      return "Bad argument";
  case AES_KEY_WRAP_ENCRYPTION_FAILED:  return "Encryption failed";
  case AES_KEY_WRAP_DECRYPTION_FAILED:  return "Decryption failed";
  case AES_KEY_WRAP_BAD_MAGIC:          return "Bad AIV magic number";
  case AES_KEY_WRAP_BAD_LENGTH:         return "Encoded length out of range";
  case AES_KEY_WRAP_BAD_PADDING:        return "Nonzero padding detected";
  default:                              return NULL;
  }
}


#ifdef AES_KEY_WRAP_SELF_TEST

/*
 * Test cases from RFC 5649.
 */

typedef struct {
  const char *K;                /* Key-encryption-key */
  const char *Q;                /* Plaintext */
  const char *C;                /* Ciphertext */
} test_case_t;

static const test_case_t test_case[] = {

  { "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",                       /* K */
    "c37b7e6492584340 bed1220780894115 5068f738",                               /* Q */
    "138bdeaa9b8fa7fc 61f97742e72248ee 5ae6ae5360d1ae6a 5f54f373fa543b6a"},     /* C */

  { "5840df6e29b02af1 ab493b705bf16ea1 ae8338f4dcc176a8",                       /* K */
    "466f7250617369",                                                           /* Q */
    "afbeb0f07dfbf541 9200f2ccb50bb24f" }                                       /* C */

};

static int parse_hex(const char *hex, unsigned char *bin, size_t *len, const size_t max)
{
  static const char whitespace[] = " \t\r\n";
  size_t i;

  assert(hex != NULL && bin != NULL && len != NULL);

  hex += strspn(hex, whitespace);

  for (i = 0; *hex != '\0' && i < max; i++, hex += 2 + strspn(hex + 2, whitespace))
    if (sscanf(hex, "%2hhx", &bin[i]) != 1)
      return 0;

  *len = i;

  return *hex == '\0';
}

static const char *format_hex(const unsigned char *bin, const size_t len, char *hex, const size_t max)
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

#ifndef TC_BUFSIZE
#define TC_BUFSIZE      4096
#endif

static int run_test(const test_case_t * const tc)
{
  unsigned char K[TC_BUFSIZE], Q[TC_BUFSIZE], C[TC_BUFSIZE], q[TC_BUFSIZE], c[TC_BUFSIZE];
  size_t K_len, Q_len, C_len, q_len = sizeof(q), c_len = sizeof(c);
  char h1[TC_BUFSIZE * 3],  h2[TC_BUFSIZE * 3];
  aes_key_wrap_status_t ret;
  CRYPT_CONTEXT ctx;
  int ok = 1;

  assert(tc != NULL);

  if (!parse_hex(tc->K, K, &K_len, sizeof(K)))
    return printf("couldn't parse KEK %s\n", tc->K), 0;

  if (!parse_hex(tc->Q, Q, &Q_len, sizeof(Q)))
    return printf("couldn't parse plaintext %s\n", tc->Q), 0;

  if (!parse_hex(tc->C, C, &C_len, sizeof(C)))
    return printf("couldn't parse ciphertext %s\n", tc->C), 0;

  if (cryptCreateContext(&ctx, CRYPT_UNUSED, CRYPT_ALGO_AES) != CRYPT_OK)
    return printf("couldn't create context\n"), 0;

  if (cryptSetAttribute(ctx, CRYPT_CTXINFO_MODE, CRYPT_MODE_ECB) != CRYPT_OK ||
      cryptSetAttributeString(ctx, CRYPT_CTXINFO_KEY, K, K_len)  != CRYPT_OK)
    ok = printf("couldn't initialize KEK\n"), 0;

  if (ok) {

    if ((ret = aes_key_wrap(ctx, Q, Q_len, c, &c_len)) != AES_KEY_WRAP_OK)
      ok = printf("couldn't wrap %s: %s\n", tc->Q, aes_key_wrap_error_string(ret)), 0;

    if ((ret = aes_key_unwrap(ctx, C, C_len, q, &q_len)) != AES_KEY_WRAP_OK)
      ok = printf("couldn't unwrap %s: %s\n", tc->C, aes_key_wrap_error_string(ret)), 0;

    if (C_len != c_len || memcmp(C, c, C_len) != 0)
      ok = printf("ciphertext mismatch:\n  Want: %s\n  Got:  %s\n",
                  format_hex(C, C_len, h1, sizeof(h1)),
                  format_hex(c, c_len, h2, sizeof(h2))), 0;

    if (Q_len != q_len || memcmp(Q, q, Q_len) != 0)
      ok = printf("plaintext mismatch:\n  Want: %s\n  Got:  %s\n",
                  format_hex(Q, Q_len, h1, sizeof(h1)),
                  format_hex(q, q_len, h2, sizeof(h2))), 0;

  }

  cryptDestroyContext(ctx);
  return ok;
}

int main (int argc, char *argv[])
{
  int i;

  if (cryptInit() != CRYPT_OK) 
    return printf("Couldn't initialize Cryptlib\n"), 1;

  for (i = 0; i < sizeof(test_case)/sizeof(*test_case); i++) {
    printf("Running test case #%d...", i);
    if (run_test(&test_case[i]))
      printf("OK\n");
  }

  if (cryptEnd() != CRYPT_OK)
    return printf("Cryptlib unhappy on shutdown\n"), 1;

  return 0;
}

#endif

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
