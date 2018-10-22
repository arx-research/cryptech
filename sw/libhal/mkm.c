/*
 * mkm.c
 * -----
 * Master Key Memory functions.
 *
 * Copyright (c) 2016, NORDUnet A/S All rights reserved.
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

/*
 * Code to load the Master key (Key Encryption Key) from either the volatile MKM
 * (by asking the FPGA to provide it, using the mkmif) or from the last sector in
 * the keystore flash.
 *
 * Storing the master key in flash is a pretty Bad Idea, but since the Alpha board
 * doesn't have a battery mounted (only pin headers for attaching one), it might
 * help in non-production use where one doesn't have tamper protection anyways.
 *
 * For production use on the Alpha, one option is to have the Master Key on paper
 * and enter it into volatile RAM after each power on.
 *
 * In both volatile memory and flash, the data is stored as a 32 bit status to
 * know if the memory is initialized or not, followed by 32 bytes (256 bits) of
 * Master Key.
 */

#define HAL_OK CMIS_HAL_OK
#include "stm-init.h"
#include "stm-keystore.h"
#undef HAL_OK

#define HAL_OK LIBHAL_OK
#include "hal.h"
#include "hal_internal.h"
#undef HAL_OK

#include <string.h>


static int volatile_init = 0;
static hal_core_t *core = NULL;

#define MKM_VOLATILE_STATUS_ADDRESS     0
#define MKM_VOLATILE_SCLK_DIV           0x20
#define MKM_FLASH_STATUS_ADDRESS        (KEYSTORE_SECTOR_SIZE * (KEYSTORE_NUM_SECTORS - 1))

/*
 * Match uninitialized flash for the "not set" value.
 * Leave some bits at 1 for the "set" value to allow
 * for adding more values later, if needed.
 */
#define MKM_STATUS_NOT_SET              0xffffffff
#define MKM_STATUS_SET                  0x0000ffff
#define MKM_STATUS_ERASED               0x00000000


static hal_error_t hal_mkm_volatile_init(void)
{
  if (volatile_init)
    return LIBHAL_OK;

  hal_error_t err;
  uint32_t status;

  if ((core = hal_core_find(MKMIF_NAME, NULL)) == NULL)
    return HAL_ERROR_CORE_NOT_FOUND;

  if ((err = hal_mkmif_set_clockspeed(core, MKM_VOLATILE_SCLK_DIV)) != LIBHAL_OK ||
      (err = hal_mkmif_init(core)) != LIBHAL_OK ||
      (err = hal_mkmif_read_word(core, MKM_VOLATILE_STATUS_ADDRESS, &status)) != LIBHAL_OK)
    return err;

  if (status != MKM_STATUS_SET && status != MKM_STATUS_NOT_SET) {
    /*
     * XXX Something is a bit fishy here. If we just write the status word, it reads back wrong sometimes,
     * while if we write the full buf too it is consistently right afterwards.
     */
    uint8_t buf[KEK_LENGTH] = {0};
    if ((err = hal_mkmif_write(core, MKM_VOLATILE_STATUS_ADDRESS + 4, buf, sizeof(buf))) != LIBHAL_OK ||
	(err = hal_mkmif_write_word(core, MKM_VOLATILE_STATUS_ADDRESS, MKM_STATUS_NOT_SET)) != LIBHAL_OK)
      return err;
  }

  volatile_init = 1;
  return LIBHAL_OK;
}

hal_error_t hal_mkm_volatile_read(uint8_t *buf, const size_t len)
{
  hal_error_t err;
  uint32_t status;

  if (len && len != KEK_LENGTH)
    return HAL_ERROR_MASTERKEY_BAD_LENGTH;

  if ((err = hal_mkm_volatile_init()) != LIBHAL_OK ||
      (err = hal_mkmif_read_word(core, MKM_VOLATILE_STATUS_ADDRESS, &status)) != LIBHAL_OK)
    return err;

  if (buf != NULL && len) {
    /*
     * Don't return the random bytes in the RAM memory in case it isn't initialized.
     * Or maybe we should fill the buffer with proper random data in that case... hmm.
     */
    if (status != MKM_STATUS_SET)
      memset(buf, 0x0, len);
    else if ((err = hal_mkmif_read(core, MKM_VOLATILE_STATUS_ADDRESS + 4, buf, len)) != LIBHAL_OK)
      return err;
  }

  if (status == MKM_STATUS_SET)
    return LIBHAL_OK;

  if (status == MKM_STATUS_NOT_SET)
    return HAL_ERROR_MASTERKEY_NOT_SET;

  return HAL_ERROR_MASTERKEY_FAIL;
}

hal_error_t hal_mkm_volatile_write(const uint8_t * const buf, const size_t len)
{
  hal_error_t err;

  if (len != KEK_LENGTH)
    return HAL_ERROR_MASTERKEY_BAD_LENGTH;

  if (buf == NULL)
    return HAL_ERROR_MASTERKEY_FAIL;

  if ((err = hal_mkm_volatile_init()) != LIBHAL_OK ||
      (err = hal_mkmif_write(core, MKM_VOLATILE_STATUS_ADDRESS + 4, buf, len)) != LIBHAL_OK ||
      (err = hal_mkmif_write_word(core, MKM_VOLATILE_STATUS_ADDRESS, MKM_STATUS_SET)) != LIBHAL_OK)
    return err;

  return LIBHAL_OK;
}

hal_error_t hal_mkm_volatile_erase(const size_t len)
{
  uint8_t buf[KEK_LENGTH] = {0};
  hal_error_t err;

  if (len != KEK_LENGTH)
    return HAL_ERROR_MASTERKEY_BAD_LENGTH;

  if ((err = hal_mkm_volatile_init()) != LIBHAL_OK ||
      (err = hal_mkmif_write(core, MKM_VOLATILE_STATUS_ADDRESS + 4, buf, sizeof(buf))) != LIBHAL_OK ||
      (err = hal_mkmif_write_word(core, MKM_VOLATILE_STATUS_ADDRESS, MKM_STATUS_NOT_SET)) != LIBHAL_OK)
    return err;

  return LIBHAL_OK;
}

/*
 * hal_mkm_flash_*() functions moved to ks_flash.c, to keep all the code that
 * knows intimate details of the keystore flash layout in one place.
 */

hal_error_t hal_mkm_get_kek(uint8_t *kek,
			    size_t *kek_len,
			    const size_t kek_max)
{
  if (kek == NULL || kek_len == NULL || kek_max < bitsToBytes(128))
    return HAL_ERROR_BAD_ARGUMENTS;

  const size_t len = ((kek_max < bitsToBytes(192)) ? bitsToBytes(128) :
                      (kek_max < bitsToBytes(256)) ? bitsToBytes(192) :
                      bitsToBytes(256));

  hal_error_t err = hal_mkm_volatile_read(kek, len);

  if (err == LIBHAL_OK) {
    *kek_len = len;
    return LIBHAL_OK;
  }

#if HAL_MKM_FLASH_BACKUP_KLUDGE

  /*
   * It turns out that, in every case where this function is called,
   * we already hold the keystore lock, so attempting to grab it again
   * would deadlock.  This almost never happens when the volatile MKM
   * is set, but there's a race condition that might drop us here if
   * hal_mkm_volatile_read() returns HAL_ERROR_CORE_BUSY.  Whee!
   */

  if (hal_mkm_flash_read_no_lock(kek, len) == LIBHAL_OK) {
    *kek_len = len;
    return LIBHAL_OK;
  }

#endif

  /*
   * Both keystores returned an error, probably HAL_ERROR_MASTERKEY_NOT_SET.
   * I could try to be clever and compare the errors, but really the volatile
   * keystore is the important one (you shouldn't store the master key in
   * flash), so return that error.
   */
  return err;
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
