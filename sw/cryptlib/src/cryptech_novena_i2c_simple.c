/* 
 * cryptech_novena_i2c_simple.c
 * ----------------------------
 *
 * This is an early prototype Hardware Adaption Layer (HAL) for using
 * Cryptlib with the Cryptech project's FGPA cores over an I2C bus on
 * the Novena PVT1 development board using a simple stream-based
 * protocol in which each core is represented as a separate I2C device.
 * This is compatible with the core/novena_i2c_simple FPGA build.
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
 * I2C configuration.  Note that, unlike the i2c_coretest HAL, each
 * hash core has its own I2C address.  The SHA-512 core still has mode
 * bits to select which of its four hash algorithms we want, but since
 * they're stuffed into the low bits of the I2C address, they look
 * like separate devices to us, so we treat them that way.
 */

#define I2C_DEV                 "/dev/i2c-2"
#define I2C_SHA1_ADDR           0x1e
#define I2C_SHA256_ADDR         0x1f
#define I2C_SHA384_ADDR         0x22
#define I2C_SHA512_ADDR         0x23

/*
 * Length parameters for the various hashes.
 */

#define SHA1_BLOCK_LEN          bitsToBytes(512)
#define	SHA1_LENGTH_LEN		bitsToBytes(64)
#define SHA1_DIGEST_LEN         bitsToBytes(160)

#define SHA256_BLOCK_LEN        bitsToBytes(512)
#define	SHA256_LENGTH_LEN	bitsToBytes(64)
#define SHA256_DIGEST_LEN       bitsToBytes(256)

#define	SHA384_BLOCK_LEN	SHA512_BLOCK_LEN
#define	SHA384_LENGTH_LEN	SHA512_LENGTH_LEN
#define SHA384_DIGEST_LEN       bitsToBytes(384)

#define SHA512_BLOCK_LEN        bitsToBytes(1024)
#define	SHA512_LENGTH_LEN	bitsToBytes(128)
#define SHA512_DIGEST_LEN       bitsToBytes(512)

#define MAX_BLOCK_LEN           SHA512_BLOCK_LEN

/* Hash state */
typedef struct {
  unsigned long long msg_length_high;   /* Total data hashed in this message */
  unsigned long long msg_length_low;    /* (128 bits in SHA-512 cases) */
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

  if (debug)
    fprintf(stderr, "[ Opened %s, fd %d ]\n", I2C_DEV, i2cfd);

  return 1;
}

static int i2c_addr(const int addr)
{
  if (!addr)
    return 1;

  if (ioctl(i2cfd, I2C_SLAVE, addr) < 0) {
    perror("Unable to set slave address on I2C " I2C_DEV);
    return 0;
  }

  if (debug)
    fprintf(stderr, "[ Selected I2C slave 0x%x ]\n", (unsigned) addr);

  return 1;
}

static int i2c_write(const int addr, const unsigned char *buf, const size_t len)
{
  if (debug) {
    int i;
    fprintf(stderr, "write [");
    for (i = 0; i < len; ++i)
      fprintf(stderr, " %02x", buf[i]);
    fprintf(stderr, " ]\n");
  }

  if (!i2c_open() || !i2c_addr(addr))
    return 0;

  if (write(i2cfd, buf, len) != len) {
    perror("i2c write failed");
    return 0;
  }

  return 1;
}

/*
 * read() on i2c device returns one byte at a time.
 */

static int i2c_read(unsigned char *buf, const size_t len)
{
  size_t i;

  assert(i2cfd >= 0);

  for (i = 0; i < len; i++) {
    if (read(i2cfd, buf + i, 1) != 1) {
      perror("i2c read failed");
      return 0;
    }
  }

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

static int doHash(CONTEXT_INFO *contextInfoPtr,
                  const unsigned char *buffer,
                  int length,
                  const int addr,
                  const size_t block_length,
                  const size_t digest_length,
                  const size_t length_length)
{
  hash_state_t *state = NULL;

  assert(isWritePtr(contextInfoPtr, sizeof(CONTEXT_INFO)));
  assert(length == 0 || isWritePtr(buffer, length));

  state = (hash_state_t *) contextInfoPtr->ctxHash->hashInfo;

  /*
   * If the hash state was reset to allow another round of hashing,
   * reinitialise things.
   */

  if (!(contextInfoPtr->flags & CONTEXT_FLAG_HASH_INITED))
    memset(state, 0, sizeof(*state));

  if (length > 0) {             /* More data to hash */

    if (!i2c_write(addr, buffer, length))
      return CRYPT_ERROR_FAILED;

    if ((state->msg_length_low += length) < length)
      state->msg_length_high++;

  }

  else {           /* Done: add padding, then pull result from chip */

    unsigned long long bit_length_low  = (state->msg_length_low  << 3);
    unsigned long long bit_length_high = (state->msg_length_high << 3) | (state->msg_length_low >> 61);
    unsigned char block[MAX_BLOCK_LEN];
    unsigned char *p;
    size_t n;
    int i;

    /* Prepare padding buffer */
    memset(block, 0, sizeof(block));
    block[0] = 0x80;

    /* How much room is left in the current block */
    n = block_length - ((state->msg_length_low) & (block_length - 1));

    /* If there's not enough room for length count and initial padding byte, push an extra block  */
    if (n < length_length + 1) {
      if (debug)
        fprintf(stderr, "[ Overflow block, n %lu, msg_length %llu ]\n", n, state->msg_length_low);
      if (!i2c_write(addr, block, n))
        return CRYPT_ERROR_FAILED;
      block[0] = 0;
      n = block_length;
    }

    /* Finish padding with length count and push final block */
    assert(n >= length_length + 1);
    if (debug)
      fprintf(stderr, "[ Final block, n %lu, msg_length %llu ]\n", (unsigned long) n, state->msg_length_low);
    p = block + n;
    for (i = 0; (bit_length_low || bit_length_high) && i < length_length; i++) {
      *--p = (unsigned char) (bit_length_low & 0xFF);
      bit_length_low >>= 8;
      if (bit_length_high) {
        bit_length_low |= ((bit_length_high & 0xFF) << 56);
        bit_length_high >>= 8;
      }
    }
    if (!i2c_write(addr, block, n))
      return CRYPT_ERROR_FAILED;

    /* All data pushed to core, now we just need to read back the result */

    assert(digest_length <= sizeof(contextInfoPtr->ctxHash->hash));
    if (!i2c_read(contextInfoPtr->ctxHash->hash, digest_length))
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
  return doHash(contextInfoPtr, buffer, length, I2C_SHA1_ADDR, SHA1_BLOCK_LEN, SHA1_DIGEST_LEN, SHA1_LENGTH_LEN);
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
    return doHash(contextInfoPtr, buffer, length, I2C_SHA256_ADDR, SHA256_BLOCK_LEN, SHA256_DIGEST_LEN, SHA256_LENGTH_LEN);
  case bitsToBytes(384):
    return doHash(contextInfoPtr, buffer, length, I2C_SHA384_ADDR, SHA384_BLOCK_LEN, SHA384_DIGEST_LEN, SHA384_LENGTH_LEN);
  case bitsToBytes(512):
    return doHash(contextInfoPtr, buffer, length, I2C_SHA512_ADDR, SHA512_BLOCK_LEN, SHA512_DIGEST_LEN, SHA512_LENGTH_LEN);
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
