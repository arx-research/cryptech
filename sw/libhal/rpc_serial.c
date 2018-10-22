/*
 * rpc_serial.c
 * ------------
 * Remote procedure call transport over serial line with SLIP framing.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "hal.h"
#include "hal_internal.h"
#include "slip_internal.h"

/*
 * Not thrilled about having OS-specific conditionals, but as such things
 * go, this seems relatively safe: gcc and clang both define it on Mac OS X,
 * and anything *not* on Mac OS X which defines it is begging for trouble.
 */

#ifndef HAL_RPC_SERIAL_USE_MACOSX_IOCTL
#define HAL_RPC_SERIAL_USE_MACOSX_IOCTL	(defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__))
#endif

#if HAL_RPC_SERIAL_USE_MACOSX_IOCTL
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#endif

static int fd = -1;

hal_error_t hal_serial_init(const char * const device, const uint32_t speed)
{
    struct termios tty;
    speed_t termios_speed;

    /*
     * Apparently Linux is too cool to need an atomic mechanism for
     * locking an existing file, so we can't uses O_EXLOCK.  Sigh.
     */

    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1)
	return perror(device), HAL_ERROR_RPC_TRANSPORT;

    if (flock(fd, LOCK_EX) < 0)
        return perror(device), HAL_ERROR_RPC_TRANSPORT;

    if (tcgetattr (fd, &tty) != 0)
	return perror("tcgetattr"), HAL_ERROR_RPC_TRANSPORT;

#if !HAL_RPC_SERIAL_USE_MACOSX_IOCTL

    switch (speed) {
    case 115200:
        termios_speed = B115200;
	break;
    case 921600:
        termios_speed = B921600;
	break;
    default:
        hal_log(HAL_LOG_ERROR, "invalid line speed %lu\n", (unsigned long) speed);
	return HAL_ERROR_RPC_TRANSPORT;
    }

    cfsetospeed (&tty, termios_speed);
    cfsetispeed (&tty, termios_speed);

#endif

    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= (CS8 | CLOCAL | CREAD);

    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
	return perror("tcsetattr"), HAL_ERROR_RPC_TRANSPORT;

#if HAL_RPC_SERIAL_USE_MACOSX_IOCTL

    termios_speed = speed;

    if (ioctl(fd, IOSSIOSPEED, &speed) < 0)
        return perror("ioctl(IOSSIOSPEED)"), HAL_ERROR_RPC_TRANSPORT;

#endif

    return HAL_OK;
}

hal_error_t hal_serial_close(void)
{
    int ret = close(fd);
    fd = -1;
    if (ret != 0)
        return perror("close"), HAL_ERROR_RPC_TRANSPORT;
    return HAL_OK;
}

hal_error_t hal_serial_send_char(const uint8_t c)
{
    if (write(fd, &c, 1) != 1)
	return perror("write"), HAL_ERROR_RPC_TRANSPORT;
    return HAL_OK;
}

hal_error_t hal_serial_recv_char(uint8_t * const c)
{
    if (read(fd, c, 1) != 1)
	return perror("read"), HAL_ERROR_RPC_TRANSPORT;
    return HAL_OK;
}

/* Access routine for the file descriptor, so daemon can poll on it.
 */
int hal_serial_get_fd(void)
{
    return fd;
}
