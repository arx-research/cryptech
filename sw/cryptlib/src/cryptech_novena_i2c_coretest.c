/* 
 * cryptech_novena_i2c_coretest.c
 * ------------------------------
 *
 * This is an early prototype Hardware Adaption Layer (HAL) for using
 * Cryptlib with the Cryptech project's FGPA cores over an I2C bus on
 * the Novena PVT1 development board using the "coretest" byte stream
 * protocol.  This is compatible with the core/novena FPGA build.
 *
 * The communication channel used here is not suitable for production
 * use, this is just a prototype.
 * 
 * Authors: Joachim Strömbergson, Paul Selkirk, Rob Austein
 * Copyright (c) 2014, SUNET
 * 
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided that the following 
 * conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The HAL framework is taken from the Cryptlib hw_dummy.c template,
 * and is Copyright 1998-2009 by Peter Gutmann.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#if defined( INC_ALL )
  #include "crypt.h"
  #include "context.h"
  #include "hardware.h"
#else
  #include "crypt.h"
  #include "context/context.h"
  #include "device/hardware.h"
#endif /* Compiler-specific includes */

/*
 * I2C_SLAVE comes from /usr/include/linux/i2c-dev.h, but if we
 * include that we won't be able to compile this except on Linux.  It
 * won't *run* anywhere but on Linux, but it's useful to be able to do
 * compilation tests on other platforms, eg, with Clang, so for now we
 * take the small risk that this one magic constant might change.
 */

#define I2C_SLAVE       0x0703


#ifdef USE_HARDWARE

/*
 * I2C-related parameters, copied from hash_tester.c
 */

/* I2C configuration */
#define I2C_DEV   "/dev/i2c-2"
#define I2C_ADDR  0x0f

/* command codes */
#define SOC       0x55
#define EOC       0xaa
#define READ_CMD  0x10
#define WRITE_CMD 0x11
#define RESET_CMD 0x01

/* response codes */
#define SOR       0xaa
#define EOR       0x55
#define READ_OK   0x7f
#define WRITE_OK  0x7e
#define RESET_OK  0x7d
#define UNKNOWN   0xfe
#define ERROR     0xfd

/* addresses and codes common to all hash cores */
#define ADDR_NAME0              0x00
#define ADDR_NAME1              0x01
#define ADDR_VERSION            0x02
#define ADDR_CTRL               0x08
#define CTRL_INIT_CMD           1
#define CTRL_NEXT_CMD           2
#define ADDR_STATUS             0x09
#define STATUS_READY_BIT        1
#define STATUS_VALID_BIT        2

/*
 * Addresses and codes for the specific hash cores.
 * Lengths here are in bytes (not bits, not 32-bit words).
 */

#define SHA1_ADDR_PREFIX        0x10
#define SHA1_ADDR_BLOCK         0x10
#define SHA1_BLOCK_LEN          bitsToBytes(512)
#define	SHA1_LENGTH_LEN		bitsToBytes(64)
#define SHA1_ADDR_DIGEST        0x20
#define SHA1_DIGEST_LEN         bitsToBytes(160)

#define SHA256_ADDR_PREFIX      0x20
#define SHA256_ADDR_BLOCK       0x10
#define SHA256_BLOCK_LEN        bitsToBytes(512)
#define	SHA256_LENGTH_LEN	bitsToBytes(64)
#define SHA256_ADDR_DIGEST      0x20
#define SHA256_DIGEST_LEN       bitsToBytes(256)

#define SHA512_ADDR_PREFIX      0x30
#define SHA512_CTRL_MODE_LOW    2
#define SHA512_CTRL_MODE_HIGH   3
#define SHA512_ADDR_BLOCK       0x10
#define SHA512_BLOCK_LEN        bitsToBytes(1024)
#define	SHA512_LENGTH_LEN	bitsToBytes(128)
#define SHA512_ADDR_DIGEST      0x40
#define SHA384_DIGEST_LEN       bitsToBytes(384)
#define SHA512_DIGEST_LEN       bitsToBytes(512)
#define MODE_SHA_512_224        (0 << SHA512_CTRL_MODE_LOW)
#define MODE_SHA_512_256        (1 << SHA512_CTRL_MODE_LOW)
#define MODE_SHA_384            (2 << SHA512_CTRL_MODE_LOW)
#define MODE_SHA_512            (3 << SHA512_CTRL_MODE_LOW)

/* Longest digest block we support at the moment */
#define MAX_BLOCK_LEN           SHA512_BLOCK_LEN

/* Hash state */
typedef struct {
  unsigned long long msg_length_high;   /* Total data hashed in this message */
  unsigned long long msg_length_low;    /* (128 bits in SHA-512 cases) */
  size_t block_length;                  /* Block length for this algorithm */
  unsigned char block[MAX_BLOCK_LEN];   /* Block we're accumulating */
  size_t block_used;                    /* How much of the block we've used */
  unsigned block_count;                 /* Blocks sent */
} hash_state_t;

static int i2cfd = -1;
static int debug = 0;

/*
 * I2C low-level code
 */

static int i2c_open(void)
{
  if (i2cfd >= 0)
    return 1;

  i2cfd = open(I2C_DEV, O_RDWR);

  if (i2cfd < 0) {
    perror("Unable to open " I2C_DEV);
    i2cfd = -1;
    return 0;
  }

  if (ioctl(i2cfd, I2C_SLAVE, I2C_ADDR) < 0) {
    perror("Unable to set i2c slave device");
    return 0;
  }

  if (debug)
    fprintf(stderr, "[ Opened %s, fd %d ]\n", I2C_DEV, i2cfd);

  return 1;
}

static int i2c_write_bytes(const unsigned char *buf, const size_t len)
{
  if (debug) {
    int i;
    fprintf(stderr, "write [");
    for (i = 0; i < len; ++i)
      fprintf(stderr, " %02x", buf[i]);
    fprintf(stderr, " ]\n");
  }

  if (!i2c_open())
    return 0;

  if (write(i2cfd, buf, len) != len) {
    perror("i2c write failed");
    return 0;
  }

  return 1;
}

static int i2c_read_byte(unsigned char *b)
{
  /*
   * read() on the i2c device only returns one byte at a time,
   * and we need to parse the response one byte at a time anyway.
   */

  if (!i2c_open())
    return 0;

  if (read(i2cfd, b, 1) != 1) {
    perror("i2c read failed");
    return 0;
  }

  return 1;
}

static int i2c_send_write_cmd(const unsigned char addr0, const unsigned char addr1, const unsigned char data[])
{
  unsigned char buf[9];

  buf[0] = SOC;
  buf[1] = WRITE_CMD;
  buf[2] = addr0;
  buf[3] = addr1;
  buf[4] = data[0];
  buf[5] = data[1];
  buf[6] = data[2];
  buf[7] = data[3];
  buf[8] = EOC;

  return i2c_write_bytes(buf, sizeof(buf));
}

static int i2c_send_read_cmd(const unsigned char addr0, const unsigned char addr1)
{
  unsigned char buf[5];

  buf[0] = SOC;
  buf[1] = READ_CMD;
  buf[2] = addr0;
  buf[3] = addr1;
  buf[4] = EOC;

  return i2c_write_bytes(buf, sizeof(buf));
}

static int i2c_get_resp(unsigned char *buf, const size_t length)
{
  int i, len = length;

  for (i = 0; i < len; ++i) {
    assert(len <= length);      /* Paranoia */

    if (!i2c_read_byte(&buf[i]))
      return 0;

    switch (i) {                /* Special handling for certain positions in response */

    case 0:
      if (buf[i] == SOR)        /* Start of record (we hope) */
        continue;
      fprintf(stderr, "Lost sync: expected 0x%02x (SOR), got 0x%02x\n", SOR, buf[0]);
      return 0;

    case 1:                     /* Response code */
      switch (buf[i]) {
      case READ_OK:
        len = 9;
        continue;
      case WRITE_OK:
        len = 5;
        continue;
      case RESET_OK:
        len = 3;
        continue;
      case ERROR:
      case UNKNOWN:
        len = 4;
        continue;
      default:
        fprintf(stderr, "Lost sync: unknown response code 0x%02x\n", buf[i]);
        return 0;
      }
    }
  }

  if (debug) {
    fprintf(stderr, "read  [");
    for (i = 0; i < len; ++i)
      fprintf(stderr, " %02x", buf[i]);
    fprintf(stderr, " ]\n");
  }

  return 1;
}

static int i2c_check_expected(const unsigned char buf[], const int i, const unsigned char expected)
{
  if (buf[i] == expected)
    return 1;
  fprintf(stderr, "Response byte %d: expected 0x%02x, got 0x%02x\n", i, expected, buf[i]);
  return 0;
}

static int i2c_write(const unsigned char addr0, const unsigned char addr1, const unsigned char data[])
{
  unsigned char buf[5];

  if (!i2c_send_write_cmd(addr0, addr1, data) ||
      !i2c_get_resp(buf, sizeof(buf))         ||
      !i2c_check_expected(buf, 0, SOR)        ||
      !i2c_check_expected(buf, 1, WRITE_OK)   ||
      !i2c_check_expected(buf, 2, addr0)      ||
      !i2c_check_expected(buf, 3, addr1)      ||
      !i2c_check_expected(buf, 4, EOR))
    return 0;

  return 1;
}

static int i2c_read(const unsigned char addr0, const unsigned char addr1, unsigned char data[])
{
  unsigned char buf[9];

  if (!i2c_send_read_cmd(addr0, addr1)     ||
      !i2c_get_resp(buf, sizeof(buf))      ||
      !i2c_check_expected(buf, 0, SOR)     ||
      !i2c_check_expected(buf, 1, READ_OK) ||
      !i2c_check_expected(buf, 2, addr0)   ||
      !i2c_check_expected(buf, 3, addr1)   ||
      !i2c_check_expected(buf, 8, EOR))
    return 0;

  data[0] = buf[4];
  data[1] = buf[5];
  data[2] = buf[6];
  data[3] = buf[7];
  return 1;
}

static int i2c_ctrl(const unsigned char addr0, const unsigned char ctrl_cmd)
{
  unsigned char data[4];
  memset(data, 0, sizeof(data));
  data[3] = ctrl_cmd;
  return i2c_write(addr0, ADDR_CTRL, data);
}

static int i2c_wait(const unsigned char addr0, const unsigned char status)
{
  unsigned char buf[9];

  do {
    if (!i2c_send_read_cmd(addr0, ADDR_STATUS))
      return 0;
    if (!i2c_get_resp(buf, sizeof(buf)))
      return 0;
    if (buf[1] != READ_OK)
      return 0;
  } while ((buf[7] & status) != status);

  if (debug)
    fprintf(stderr, "[ Done waiting ]\n");

  return 1;
}

static int i2c_wait_ready(const unsigned char addr0)
{
  if (debug)
    fprintf(stderr, "[ Waiting for ready ]\n");
  return i2c_wait(addr0, STATUS_READY_BIT);
}

static int i2c_wait_valid(const unsigned char addr0)
{
  if (debug)
    fprintf(stderr, "[ Waiting for valid ]\n");
  return i2c_wait(addr0, STATUS_VALID_BIT);
}

/*
 * Send one block to a core.
 */

static int hash_write_block(const unsigned char addr_prefix,
                            const unsigned char addr_block,
                            const unsigned char ctrl_mode,
                            const hash_state_t *state)
{
  unsigned char ctrl_cmd;
  int i;

  assert(state != NULL && state->block_length % 4 == 0);

  for (i = 0; i + 3 < state->block_length; i += 4)
    if (!i2c_write(addr_prefix, addr_block + i/4, state->block + i))
      return 0;

  ctrl_cmd = state->block_count == 0 ? CTRL_INIT_CMD : CTRL_NEXT_CMD;

  if (debug)
    fprintf(stderr, "[ %s ]\n", state->block_count == 0 ? "init" : "next");

  return i2c_ctrl(addr_prefix, ctrl_cmd|ctrl_mode) && i2c_wait_ready(addr_prefix);
}

/*
 * Read hash result from core.
 */

static int hash_read_digest(const unsigned char addr_prefix, const unsigned char addr_digest,
                            unsigned char *digest, const size_t digest_length)
{
  int i;

  assert(digest_length % 4 == 0);

  if (!i2c_wait_valid(addr_prefix))
    return 0;

  for (i = 0; i + 3 < digest_length; i += 4)
    if (!i2c_read(addr_prefix, addr_digest + i/4, digest + i))
      return 0;

  return 1;
}

/****************************************************************************
 *                                                                          *
 *                               Random Numbers                             *
 *                                                                          *
 ****************************************************************************/

/*
 * We have a TRNG core, but I don't think it's hooked up to I2C yet, so
 * for the moment we use the toy generator from hw_dummy.c.
 */

static void dummyGenRandom(void *buffer, const int length)
{
  HASHFUNCTION_ATOMIC hashFunctionAtomic;
  BYTE hashBuffer[CRYPT_MAX_HASHSIZE], *bufPtr = buffer;
  static int counter = 0;
  int hashSize, i;

  assert(isWritePtr(buffer, length));

  REQUIRES_V(length >= 1 && length < MAX_INTLENGTH);

  /*
   * Fill the buffer with random-ish data.  This gets a bit tricky
   * because we need to fool the entropy tests so we can't just fill
   * it with a fixed (or even semi-random) pattern but have to set up
   * a somewhat kludgy PRNG.
   */
  getHashAtomicParameters(CRYPT_ALGO_SHA1, 0, &hashFunctionAtomic, &hashSize);
  memset(hashBuffer, counter, hashSize);
  counter++;
  for (i = 0; i < length; i++) {
    if (i % hashSize == 0)
      hashFunctionAtomic(hashBuffer, CRYPT_MAX_HASHSIZE, hashBuffer, hashSize);
    bufPtr[i] = hashBuffer[i % hashSize];
  }
}

/****************************************************************************
 *                                                                           *
 *                   Hash/MAC Capability Interface Routines                  *
 *                                                                           *
 ****************************************************************************/

/*
 * Return context subtype-specific information.  All supported hash
 * algorithms currently use the same state object, so they can all use
 * this method.
 */

static int hashGetInfo(const CAPABILITY_INFO_TYPE type,
                      CONTEXT_INFO *contextInfoPtr, 
                      void *data, const int length)
{
  switch (type) {
  case CAPABILITY_INFO_STATESIZE:
    /*
     * Tell cryptlib how much hash-state storage we want allocated.
     */
    *(int *) data = sizeof(hash_state_t);
    return CRYPT_OK;

  default:
    return getDefaultInfo(type, contextInfoPtr, data, length);
  }
}

/*
 * Hash data.  All supported hash algorithms use similar block
 * manipulations and padding algorithms, so all can use this method
 * with a few parameters which we handle via closures below.
 */

static int doHash(CONTEXT_INFO *contextInfoPtr, const unsigned char *buffer, int length,
                  const size_t block_length, const unsigned char addr_prefix, const unsigned char addr_block,
                  const size_t digest_length, const unsigned char addr_digest, const unsigned char ctrl_mode,
                  const size_t length_length)
{
  hash_state_t *state = NULL;
  size_t n;
  int i;

  assert(isWritePtr(contextInfoPtr, sizeof(CONTEXT_INFO)));
  assert(length == 0 || isWritePtr(buffer, length));

  state = (hash_state_t *) contextInfoPtr->ctxHash->hashInfo;

  /*
   * If the hash state was reset to allow another round of hashing,
   * reinitialise things.
   */

  if (!(contextInfoPtr->flags & CONTEXT_FLAG_HASH_INITED)) {
    memset(state, 0, sizeof(*state));
    state->block_length = block_length;
  }

  /* May want an assertion here that state->block_length is correct */

  if (length > 0) {             /* More data to hash */

    const unsigned char *p = buffer;

    while ((n = state->block_length - state->block_used) <= length) {
      /*
       * We have enough data for another complete block.
       */
      if (debug)
        fprintf(stderr, "[ Full block, length %lu, used %lu, n %lu, msg_length %llu ]\n",
                (unsigned long) length, (unsigned long) state->block_used, (unsigned long) n, state->msg_length_low);
      memcpy(state->block + state->block_used, p, n);
      if ((state->msg_length_low += n) < n)
        state->msg_length_high++;
      state->block_used = 0;
      length -= n;
      p += n;
      if (!hash_write_block(addr_prefix, addr_block, ctrl_mode, state))
        return CRYPT_ERROR_FAILED;
      state->block_count++;
    }

    if (length > 0) {
      /*
       * Data left over, but not enough for a full block, stash it.
       */
      if (debug)
        fprintf(stderr, "[ Partial block, length %lu, used %lu, n %lu, msg_length %llu ]\n",
                (unsigned long) length, (unsigned long) state->block_used, (unsigned long) n, state->msg_length_low);
      assert(length < n);
      memcpy(state->block + state->block_used, p, length);
      if ((state->msg_length_low += length) < length)
        state->msg_length_high++;
      state->block_used += length;
    }
  }

  else {           /* Done: add padding, then pull result from chip */

    unsigned long long bit_length_low  = (state->msg_length_low  << 3);
    unsigned long long bit_length_high = (state->msg_length_high << 3) | (state->msg_length_low >> 61);
    unsigned char *p;

    /* Initial pad byte */
    assert(state->block_used < state->block_length);
    state->block[state->block_used++] = 0x80;

    /* If not enough room for bit count, zero and push current block */
    if ((n = state->block_length - state->block_used) < length_length) {
      if (debug)
        fprintf(stderr, "[ Overflow block, length %lu, used %lu, n %lu, msg_length %llu ]\n",
                (unsigned long) length, (unsigned long) state->block_used, (unsigned long) n, state->msg_length_low);
      if (n > 0)
        memset(state->block + state->block_used, 0, n);
      if (!hash_write_block(addr_prefix, addr_block, ctrl_mode, state))
        return CRYPT_ERROR_FAILED;
      state->block_count++;
      state->block_used = 0;
    }

    /* Pad final block */
    n = state->block_length - state->block_used;
    assert(n >= length_length);
    if (n > 0)
      memset(state->block + state->block_used, 0, n);
    if (debug)
      fprintf(stderr, "[ Final block, length %lu, used %lu, n %lu, msg_length %llu ]\n",
              (unsigned long) length, (unsigned long) state->block_used, (unsigned long) n, state->msg_length_low);
    p = state->block + state->block_length;
    for (i = 0; (bit_length_low || bit_length_high) && i < length_length; i++) {
      *--p = (unsigned char) (bit_length_low & 0xFF);
      bit_length_low >>= 8;
      if (bit_length_high) {
        bit_length_low |= ((bit_length_high & 0xFF) << 56);
        bit_length_high >>= 8;
      }
    }

    /* Push final block */
    if (!hash_write_block(addr_prefix, addr_block, ctrl_mode, state))
      return CRYPT_ERROR_FAILED;
    state->block_count++;

    /* All data pushed to core, now we just need to read back the result */

    assert(digest_length <= sizeof(contextInfoPtr->ctxHash->hash));
    if (!hash_read_digest(addr_prefix, addr_digest, contextInfoPtr->ctxHash->hash, digest_length))
      return CRYPT_ERROR_FAILED;
  }

  return CRYPT_OK;
}

/* Perform a self-test */

static int sha1SelfTest(void)
{
  /*
   * If we think of a self-test, insert it here.
   */

  return CRYPT_OK;
}

/* Hash data */

static int sha1Hash(CONTEXT_INFO *contextInfoPtr, unsigned char *buffer, int length)
{
  return doHash(contextInfoPtr, buffer, length,
                SHA1_BLOCK_LEN, SHA1_ADDR_PREFIX, SHA1_ADDR_BLOCK,
                SHA1_DIGEST_LEN, SHA1_ADDR_DIGEST, 0, SHA1_LENGTH_LEN);
}

/* Perform a self-test */

static int sha2SelfTest(void)
{
  /*
   * If we think of a self-test, insert it here.
   */

  return CRYPT_OK;
}

/* Hash data */

static int sha2Hash(CONTEXT_INFO *contextInfoPtr, unsigned char *buffer, int length)
{
  assert(contextInfoPtr != NULL && contextInfoPtr->capabilityInfo != NULL);

  switch (contextInfoPtr->capabilityInfo->blockSize) {

  case bitsToBytes(256):
    return doHash(contextInfoPtr, buffer, length,
                  SHA256_BLOCK_LEN, SHA256_ADDR_PREFIX, SHA256_ADDR_BLOCK,
                  SHA256_DIGEST_LEN, SHA256_ADDR_DIGEST, 0, SHA256_LENGTH_LEN);

  case bitsToBytes(384):
    return doHash(contextInfoPtr, buffer, length,
                  SHA512_BLOCK_LEN, SHA512_ADDR_PREFIX, SHA512_ADDR_BLOCK,
                  SHA384_DIGEST_LEN, SHA512_ADDR_DIGEST, MODE_SHA_384,
                  SHA512_LENGTH_LEN);

  case bitsToBytes(512):
    return doHash(contextInfoPtr, buffer, length,
                  SHA512_BLOCK_LEN, SHA512_ADDR_PREFIX, SHA512_ADDR_BLOCK,
                  SHA512_DIGEST_LEN, SHA512_ADDR_DIGEST, MODE_SHA_512,
                  SHA512_LENGTH_LEN);

  default:
    return CRYPT_ERROR_FAILED;
  }
}

/* Parameter initialization, to handle SHA-2 algorithms other than SHA-256 */

static int sha2InitParams(INOUT CONTEXT_INFO *contextInfoPtr, 
                          IN_ENUM(KEYPARAM) const KEYPARAM_TYPE paramType,
                          IN_OPT const void *data, 
                          IN_INT const int dataLength)
{
  static const CAPABILITY_INFO capabilityInfoSHA384 = {
    CRYPT_ALGO_SHA2, bitsToBytes( 384 ), "SHA-384", 7,
    bitsToBytes( 0 ), bitsToBytes( 0 ), bitsToBytes( 0 ),
    sha2SelfTest, hashGetInfo, NULL, NULL, NULL, NULL, sha2Hash, sha2Hash
  };

  static const CAPABILITY_INFO capabilityInfoSHA512 = {
    CRYPT_ALGO_SHA2, bitsToBytes( 512 ), "SHA-512", 7,
    bitsToBytes( 0 ), bitsToBytes( 0 ), bitsToBytes( 0 ),
    sha2SelfTest, hashGetInfo, NULL, NULL, NULL, NULL, sha2Hash, sha2Hash
  };

  assert(isWritePtr(contextInfoPtr, sizeof(CONTEXT_INFO)));
  REQUIRES(contextInfoPtr->type == CONTEXT_HASH);
  REQUIRES(paramType > KEYPARAM_NONE && paramType < KEYPARAM_LAST);

  if (paramType == KEYPARAM_BLOCKSIZE) {
    switch (dataLength) {
    case bitsToBytes(256):
      return CRYPT_OK;
    case bitsToBytes(384):
      contextInfoPtr->capabilityInfo = &capabilityInfoSHA384;
      return CRYPT_OK;
    case bitsToBytes(512):
      contextInfoPtr->capabilityInfo = &capabilityInfoSHA512;
      return CRYPT_OK;
    default:
      return CRYPT_ARGERROR_NUM1;
    }
  }

  return initGenericParams(contextInfoPtr, paramType, data, dataLength);
}

/****************************************************************************
 *                                                                          *
 *                           Hardware External Interface                    *
 *                                                                          *
 ****************************************************************************/

/* The capability information for this device */

static const CAPABILITY_INFO capabilities[] = {

  { CRYPT_ALGO_SHA1, bitsToBytes( 160 ), "SHA-1", 5,
    bitsToBytes( 0 ), bitsToBytes( 0 ), bitsToBytes( 0 ),
    sha1SelfTest, hashGetInfo, NULL, NULL, NULL, NULL, sha1Hash, sha1Hash },

  { CRYPT_ALGO_SHA2, bitsToBytes( 256 ), "SHA-2", 5,
    bitsToBytes( 0 ), bitsToBytes( 0 ), bitsToBytes( 0 ),
    sha2SelfTest, hashGetInfo, NULL, sha2InitParams, NULL, NULL, sha2Hash, sha2Hash },

  { CRYPT_ALGO_NONE }, { CRYPT_ALGO_NONE }
};

/* Return the hardware capabilities list */

int hwGetCapabilities(const CAPABILITY_INFO **capabilityInfo, int *noCapabilities)
{
  assert(isReadPtr(capabilityInfo, sizeof(CAPABILITY_INFO *)));
  assert(isWritePtr(noCapabilities, sizeof(int)));

  *capabilityInfo = capabilities;
  *noCapabilities = FAILSAFE_ARRAYSIZE(capabilities, CAPABILITY_INFO);

  return CRYPT_OK;
}

/*
 * Get random data from the hardware.  We have a TRNG core, but I
 * don't think we hae I2C code for it yet, so leave this as a dummy
 * for the moment.
 */

int hwGetRandom(void *buffer, const int length)
{
  assert(isWritePtr(buffer, length));

  REQUIRES(length >= 1 && length < MAX_INTLENGTH);

  /* Fill the buffer with random-ish data */
  dummyGenRandom(buffer, length);

  return CRYPT_OK;
}

/*
 * These "personality" methods are trivial stubs, as we do not yet
 * have any cores which do encyrption or signature.  When we do, these
 * methods will need to be rewritten, and whoever does that rewriting
 * will definitely want to look at the detailed comments and template
 * code in device/hw_dummy.c.
 */

/* Look up an item held in the hardware */

int hwLookupItem(const void *keyID, const int keyIDlength, int *keyHandle)
{
  assert(keyHandle != NULL);
  *keyHandle = CRYPT_ERROR;
  return CRYPT_ERROR_NOTFOUND;
}

/* Delete an item held in the hardware */

int hwDeleteItem(const int keyHandle)
{
  return CRYPT_OK;
}

/* Initialise/zeroise the hardware */

int hwInitialise(void)
{
  return CRYPT_OK;
}

#endif /* USE_HARDWARE */

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
