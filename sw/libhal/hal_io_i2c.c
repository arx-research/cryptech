/*
 * hal_io_i2c.c
 * ------------
 * This module contains common code to talk to the FPGA over the I2C bus.
 *
 * Author: Paul Selkirk
 * Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
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
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "hal.h"
#include "hal_internal.h"

#define I2C_dev                 "/dev/i2c-2"
#define I2C_addr                0x0f
#define I2C_SLAVE               0x0703

static int debug = 0;
static int i2cfd = -1;


void hal_io_set_debug(int onoff)
{
  debug = onoff;
}

static void dump(char *label, const uint8_t *buf, size_t len)
{
  if (debug) {
    int i;
    printf("%s [", label);
    for (i = 0; i < len; ++i)
      printf(" %02x", buf[i]);
    printf(" ]\n");
  }
}

static void i2c_close(void)
{
  (void) close(i2cfd);
}

static hal_error_t i2c_open(void)
{
  int fd = -1;

  if (i2cfd >= 0)
    return HAL_OK;

  /* It's dead, Jim, you can stop kicking it now */
  if (i2cfd < -1)
    return HAL_ERROR_IO_SETUP_FAILED;

  fd = open(I2C_dev, O_RDWR);
  if (fd < 0) {
    if (debug)
      perror("Unable to open %s: " I2C_dev);
    goto fail;
  }

  if (ioctl(fd, I2C_SLAVE, I2C_addr) < 0) {
    if (debug)
      perror("Unable to set I2C slave device");
    goto fail;
  }

  if (atexit(i2c_close) < 0) {
    if (debug)
      perror("Unable to set I2C atexit handler");
    goto fail;
  }

  i2cfd = fd;
  return HAL_OK;

 fail:
  if (fd >= 0)
    close(fd);
  i2cfd = -2;
  return HAL_ERROR_IO_SETUP_FAILED;
}

static hal_error_t i2c_write(const uint8_t *buf, size_t len)
{
  hal_error_t err;

  if ((err = i2c_open()) != HAL_OK)
    return err;

  dump("write ", buf, len);

  if (write(i2cfd, buf, len) != len) {
    if (debug)
      perror("i2c write failed");
    return HAL_ERROR_IO_OS_ERROR;
  }

  return HAL_OK;
}

static hal_error_t i2c_read(uint8_t *b)
{
  hal_error_t err;

  if ((err = i2c_open()) != HAL_OK)
    return err;

  /*
   * read() on the i2c device only returns one byte at a time,
   * and hal_io_get_resp() needs to parse the response one byte at a time
   */
  if (read(i2cfd, b, 1) != 1) {
    if (debug)
      perror("i2c read failed");
    return HAL_ERROR_IO_OS_ERROR;
  }

  return 0;
}


/* coretest command codes */
#define SOC       0x55
#define EOC       0xaa
#define READ_CMD  0x10
#define WRITE_CMD 0x11
#define RESET_CMD 0x01

/* coretest response codes */
#define SOR       0xaa
#define EOR       0x55
#define READ_OK   0x7f
#define WRITE_OK  0x7e
#define RESET_OK  0x7d
#define UNKNOWN   0xfe
#define ERROR     0xfd

static hal_error_t hal_io_send_write_cmd(hal_addr_t offset, const uint8_t *data)
{
  uint8_t buf[9] = { SOC, WRITE_CMD, (offset >> 8) & 0xff, offset & 0xff,
                     data[0], data[1], data[2], data[3], EOC };
  return i2c_write(buf, sizeof(buf));
}

static hal_error_t hal_io_send_read_cmd(hal_addr_t offset)
{
  uint8_t buf[5] = { SOC, READ_CMD, (offset >> 8) & 0xff, offset & 0xff, EOC };
  return i2c_write(buf, sizeof(buf));
}

static hal_error_t hal_io_get_resp(uint8_t *buf, size_t len)
{
  hal_error_t err;
  int i;

  for (i = 0; i < len; ++i) {

    if ((err = i2c_read(&buf[i])) != HAL_OK)
      return err;

    if ((i == 0) && (buf[i] != SOR))
      /* We've gotten out of sync, and there's probably nothing we can do */
      return HAL_ERROR_IO_UNEXPECTED;

    if (i == 1) {      /* response code */
      switch (buf[i]) {
      case READ_OK:
        len = 9;
        break;
      case WRITE_OK:
        len = 5;
        break;
      case RESET_OK:
        len = 3;
        break;
      case ERROR:
      case UNKNOWN:
        len = 4;
        break;
      default:
        /* we've gotten out of sync, and there's probably nothing we can do */
        return HAL_ERROR_IO_UNEXPECTED;
      }
    }
  }

  dump("read  ", buf, len);

  return HAL_OK;
}

static hal_error_t hal_io_compare(uint8_t *buf, const uint8_t *expected, size_t len)
{
  size_t i;

  /* start at byte 1 because SOR has already been tested */
  for (i = 1; i < len; ++i) {
    if (buf[i] != expected[i])
      return HAL_ERROR_IO_UNEXPECTED;
  }

  return HAL_OK;
}

static hal_error_t hal_io_get_write_resp(hal_addr_t offset)
{
  uint8_t buf[5];
  uint8_t expected[5] = { SOR, WRITE_OK, (offset >> 8) & 0xff, offset & 0xff, EOR };
  hal_error_t err;

  if ((err = hal_io_get_resp(buf, sizeof(buf))) != HAL_OK)
    return err;

  return hal_io_compare(buf, expected, sizeof(expected));
}

static hal_error_t hal_io_get_read_resp(hal_addr_t offset, uint8_t *data)
{
  uint8_t buf[9];
  uint8_t expected[4] = { SOR, READ_OK, (offset >> 8) & 0xff, offset & 0xff };
  hal_error_t err;

  if ((err = hal_io_get_resp(buf, sizeof(buf))) != HAL_OK ||
      (err = hal_io_compare(buf, expected, 4))  != HAL_OK)
    return err;

  if (buf[8] != EOR)
    return HAL_ERROR_IO_UNEXPECTED;

  data[0] = buf[4];
  data[1] = buf[5];
  data[2] = buf[6];
  data[3] = buf[7];

  return HAL_OK;
}

hal_error_t hal_io_write(const hal_core_t *core, hal_addr_t offset, const uint8_t *buf, size_t len)
{
  hal_error_t err;

  if (core == NULL)
    return HAL_ERROR_CORE_NOT_FOUND;

  offset += hal_core_base(core);

  for (; len > 0; offset++, buf += 4, len -= 4)
    if ((err = hal_io_send_write_cmd(offset, buf)) != HAL_OK ||
        (err = hal_io_get_write_resp(offset))      != HAL_OK)
      return err;

  return HAL_OK;
}

hal_error_t hal_io_read(const hal_core_t *core, hal_addr_t offset, uint8_t *buf, size_t len)
{
  hal_error_t err;

  if (core == NULL)
    return HAL_ERROR_CORE_NOT_FOUND;

  offset += hal_core_base(core);

  for (; len > 0; offset++, buf += 4, len -= 4)
    if ((err = hal_io_send_read_cmd(offset))      != HAL_OK ||
        (err = hal_io_get_read_resp(offset, buf)) != HAL_OK)
      return err;

  return HAL_OK;
}

/*
 * Local variables:
 * indent-tabs-mode: nil
 * End:
 */
