/* 
 * cryptech_random.c
 * -----------------
 *
 * This is a shim to connect the Cryptech TRNG to Cryptlib's CSPRNG.
 *
 * Prototype HAL code for the Cryptech environment already provides
 * the RNG code required by the HAL, but it doesn't look like Cryptlib
 * itself ever calls that, it only seems to use the system device's
 * random function.  So this shim just uses the code we already wrote
 * to meet the HAL API requirement to feed Cryptlib's CSPRNG.
 *
 * Whether it makes sense to use what we hope is already a good TRNG
 * just to provide entropy for a CSPRNG is a question for another day.
 *
 * Author: Rob Austein
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
 */

#if defined( INC_ALL )
  #include "crypt.h"
  #include "context.h"
  #include "hardware.h"
  #include "random.h"
#else
  #include "crypt.h"
  #include "context/context.h"
  #include "device/hardware.h"
  #include "random/random.h"
#endif

#define FAST_BUFSIZE    bitsToBytes(64)
#define SLOW_BUFSIZE    (5 * FAST_BUFSIZE)

void fastPoll(void)
{
  MESSAGE_DATA msgData;
  unsigned char buffer[FAST_BUFSIZE];

  if (hwGetRandom(buffer, sizeof(buffer)) == CRYPT_OK) {
	setMessageData(&msgData, buffer, sizeof(buffer));
    krnlSendMessage(SYSTEM_OBJECT_HANDLE, IMESSAGE_SETATTRIBUTE_S, &msgData, CRYPT_IATTRIBUTE_ENTROPY);
    zeroise(buffer, sizeof(buffer));
  }
}

void slowPoll(void)
{
  MESSAGE_DATA msgData;
  unsigned char buffer[SLOW_BUFSIZE];
  int quality = 100;

  if (hwGetRandom(buffer, sizeof(buffer)) == CRYPT_OK) {
	setMessageData(&msgData, buffer, sizeof(buffer));
    krnlSendMessage(SYSTEM_OBJECT_HANDLE, IMESSAGE_SETATTRIBUTE_S, &msgData, CRYPT_IATTRIBUTE_ENTROPY);
    zeroise(buffer, sizeof(buffer));
    krnlSendMessage(SYSTEM_OBJECT_HANDLE, IMESSAGE_SETATTRIBUTE, &quality, CRYPT_IATTRIBUTE_ENTROPY_QUALITY);
  }
}

/*
 * Might patch these out in random/random.h eventually, but no-op
 * stubs will do for testing.
 */

void initRandomPolling(void)
{
  return;
}

void endRandomPolling(void)
{
  return;
}

CHECK_RETVAL \
int waitforRandomCompletion(const BOOLEAN force)
{
  return CRYPT_OK;
}

CHECK_RETVAL_BOOL \
BOOLEAN checkForked(void)
{
  return FALSE;
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
