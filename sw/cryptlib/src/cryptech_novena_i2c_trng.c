/* 
 * cryptech_novena_i2c_trng.c
 * --------------------------
 *
 * This is an early prototype Hardware Adaption Layer (HAL) for using
 * Cryptlib with the Cryptech project's FGPA cores over an I2C bus on
 * the Novena PVT1 development board using the "coretest" byte stream
 * protocol.  This is compatible with the test/novena_trng FPGA build.
 *
 * Well, except that apparently cryptlib doesn't like it when we just
 * provide a TRNG, so try also adding all the code for the hash cores.
 *
 * The communication channel used here is not suitable for production
 * use, this is just a prototype.
 * 
 * Authors: Joachim Strömbergson, Paul Selkirk, Rob Austein
 * Copyright (c) 2014-2015, SUNET
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
#include <stdint.h>

#if defined( INC_ALL )
  #include "crypt.h"
  #include "context.h"
  #include "hardware.h"
#else
  #include "crypt.h"
  #include "context/context.h"
  #include "device/hardware.h"
#endif /* Compiler-specific includes */

#include <cryptech.h>

#ifdef USE_HARDWARE

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

int debug = 0;

/****************************************************************************
 *                                                                          *
 *                               Hash utilities                             *
 *                                                                          *
 ****************************************************************************/

/*
 * Send one block to a core.
 */

static int hash_write_block(const off_t offset,
                            const uint8_t ctrl_mode,
                            const hash_state_t *state)
{
  uint8_t ctrl_cmd[4] = { 0 };
  off_t base = offset & ~(0xff);

  assert(state != NULL && state->block_length % 4 == 0);

  if (tc_write(offset, state->block, state->block_length) != 0)
    return CRYPT_ERROR_FAILED;

  ctrl_cmd[3] = (state->block_count == 0 ? CTRL_INIT_CMD : CTRL_NEXT_CMD) | ctrl_mode;

  if (debug)
    fprintf(stderr, "[ %s ]\n", state->block_count == 0 ? "init" : "next");

  return
    tc_write(base + ADDR_CTRL, ctrl_cmd, 4) ||
    tc_wait_ready(base + ADDR_STATUS);
}

/*
 * Read hash result from core.
 */

static int hash_read_digest(const off_t offset,
                            unsigned char *digest,
                            const size_t digest_length)
{
  assert(digest_length % 4 == 0);

  /* Technically, we should poll the status register for the "valid" bit, but
   * hash_write_block() has already polled for the "ready" bit, and we know
   * that the sha cores always set valid one clock cycle before ready.
   */

  return tc_read(offset, digest, digest_length);
}

/****************************************************************************
 *                                                                          *
 *                               Random Numbers                             *
 *                                                                          *
 ****************************************************************************/

/*
 * First attempt at reading random data from the Novena.
 *
 * In theory, we should wait for TRNG_VALID before reading random
 * data, but as long as this is running over I2C we're going to be so
 * slow that there's no point, and checking would just make us slower.
 *
 * If the TRNG isn't installed we need to return failure to our
 * caller.  At least with the current I2C coretest interface, coretest
 * signals this (deliberately?) by returning all zeros.
 */

#define WAIT_FOR_TRNG_VALID     0

static int readRandom(void *buffer, const int length)
{
  unsigned char temp[4], *buf = buffer;
  int i, last;

  if (debug)
    fprintf(stderr, "[ Requesting %d bytes of random data ]\n", length);

  assert(isWritePtr(buffer, length));

  REQUIRES_B(length >= 1 && length < MAX_INTLENGTH);

  for (i = 0; i < length; i += 4) {

#if WAIT_FOR_TRNG_VALID
    if (tc_wait_ready(CSPRNG_ADDR_STATUS) != 0) {
      fprintf(stderr, "[ tc_wait_valid(CSPRNG_ADDR_STATUS) failed ]\n");
      return CRYPT_ERROR_FAILED;
    }
#endif  /* WAIT_FOR_TRNG_VALID */

    last = (length - i) < 4;
    if (tc_read(CSPRNG_ADDR_RANDOM, (last ? temp : (buf + i)), 4) != 0) {
      fprintf(stderr, "[ tc_read(CSPRNG_ADDR_RANDOM) failed ]\n");
      return CRYPT_ERROR_FAILED;
    }
    if (last) {
      for (; i < length; i++)
        buf[i] = temp[i & i];
    }
  }

  for (i = 0, buf = buffer; i < length; i++, buf++)
    if (*buf != 0)
      return CRYPT_OK;

  fprintf(stderr, "[ \"Random\" data all zeros, guess TRNG is not installed ]\n");
  return CRYPT_ERROR_FAILED;
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
                  const size_t block_length,
                  const off_t addr_block,
                  const size_t digest_length,
                  const off_t addr_digest,
                  const unsigned char ctrl_mode,
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
      if (hash_write_block(addr_block, ctrl_mode, state) != 0)
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
      if (hash_write_block(addr_block, ctrl_mode, state) != 0)
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
    if (hash_write_block(addr_block, ctrl_mode, state) != 0)
      return CRYPT_ERROR_FAILED;
    state->block_count++;

    /* All data pushed to core, now we just need to read back the result */

    assert(digest_length <= sizeof(contextInfoPtr->ctxHash->hash));
    if (hash_read_digest(addr_digest, contextInfoPtr->ctxHash->hash, digest_length) != 0)
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
                SHA1_BLOCK_LEN, SHA1_ADDR_BLOCK,
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
                  SHA256_BLOCK_LEN, SHA256_ADDR_BLOCK,
                  SHA256_DIGEST_LEN, SHA256_ADDR_DIGEST, 0, SHA256_LENGTH_LEN);

  case bitsToBytes(384):
    return doHash(contextInfoPtr, buffer, length,
                  SHA512_BLOCK_LEN, SHA512_ADDR_BLOCK,
                  SHA384_DIGEST_LEN, SHA512_ADDR_DIGEST, MODE_SHA_384,
                  SHA512_LENGTH_LEN);

  case bitsToBytes(512):
    return doHash(contextInfoPtr, buffer, length,
                  SHA512_BLOCK_LEN, SHA512_ADDR_BLOCK,
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
 * Get random data from the hardware.
 *
 * So, we provide this function because the Cryptlib HAL API seems to
 * require it, but as far as I can tell nothing ever calls it.  Hmm.
 * See src/cryptech_random.c for how I'm using this to feed Cryptlib's
 * CSPRNG.  Bypassing the CSPRNG would be, well, not hard exactly, but
 * would require somewhat drastic surgery, so I'm leaving that for
 * another day.
 */

int hwGetRandom(void *buffer, const int length)
{
  if (debug)
    fprintf(stderr, "[ Requested %d bytes of random data]\n", length);

  assert(isWritePtr(buffer, length));

  REQUIRES(length >= 1 && length < MAX_INTLENGTH);

  return readRandom(buffer, length);
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
  if (debug)
    fprintf(stderr, "[ Initializing cryptech hardware ]\n");

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
