/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"

#include "serial-device.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "libsmp-private.h"

static SmpError errno_to_smp_error(int err)
{
    switch (err) {
        case EINVAL:
            return SMP_ERROR_INVALID_PARAM;
        case EBADMSG:
            return SMP_ERROR_BAD_MESSAGE;
        case E2BIG:
            return SMP_ERROR_TOO_BIG;
        case ENOMEM:
            return SMP_ERROR_NO_MEM;
        case ENOENT:
            return SMP_ERROR_NO_DEVICE;
        case ETIMEDOUT:
            return SMP_ERROR_TIMEDOUT;
        case EBADF:
            return SMP_ERROR_BAD_FD;
        case ENOSYS:
            return SMP_ERROR_NOT_SUPPORTED;
        case EBUSY:
            return SMP_ERROR_BUSY;
        case EPERM:
            return SMP_ERROR_PERM;
        case EAGAIN:
            return SMP_ERROR_WOULD_BLOCK;
        case EIO:
            return SMP_ERROR_IO;
        default:
            return SMP_ERROR_OTHER;
    }
}

int smp_serial_device_open(SmpSerialDevice *device, const char *path)
{
    int fd;

    fd = open(path, O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return errno_to_smp_error(errno);

#ifdef HAVE_TERMIOS_H
    if (isatty(fd)) {
        struct termios term;
        int ret;

        ret = tcgetattr(fd, &term);
        if (ret < 0) {
            close(fd);
            return errno_to_smp_error(errno);
        }

        cfsetispeed(&term, B115200);
        cfsetospeed(&term, B115200);
        cfmakeraw(&term);

        ret = tcsetattr(fd, TCSANOW, &term);
        if (ret < 0) {
            close(fd);
            return errno_to_smp_error(errno);
        }
    }
#endif

    device->fd = fd;

    return fd;
}

void smp_serial_device_close(SmpSerialDevice *device)
{
    close(device->fd);
}

intptr_t smp_serial_device_get_fd(SmpSerialDevice *device)
{
    return (device->fd < 0) ? SMP_ERROR_BAD_FD : device->fd;
}

int smp_serial_device_set_config(SmpSerialDevice *device,
        SmpSerialBaudrate baudrate, SmpSerialParity parity,
        int flow_control)
{
    int ret = SMP_ERROR_NOT_SUPPORTED;

#ifdef HAVE_TERMIOS_H
    if (isatty(device->fd)) {
        struct termios term;
        speed_t speed;

        ret = tcgetattr(device->fd, &term);
        if (ret < 0)
            return errno_to_smp_error(errno);

        switch (baudrate) {
            case SMP_SERIAL_BAUDRATE_1200:
                speed = B1200;
                break;
            case SMP_SERIAL_BAUDRATE_2400:
                speed = B2400;
                break;
            case SMP_SERIAL_BAUDRATE_4800:
                speed = B4800;
                break;
            case SMP_SERIAL_BAUDRATE_9600:
                speed = B9600;
                break;
            case SMP_SERIAL_BAUDRATE_19200:
                speed = B19200;
                break;
            case SMP_SERIAL_BAUDRATE_38400:
                speed = B38400;
                break;
            case SMP_SERIAL_BAUDRATE_57600:
                speed = B57600;
                break;
            case SMP_SERIAL_BAUDRATE_115200:
            default:
                speed = B115200;
                break;
        }
        cfsetispeed(&term, speed);
        cfsetospeed(&term, speed);

        switch (parity) {
            case SMP_SERIAL_PARITY_ODD:
                term.c_cflag |= PARENB;
                term.c_cflag |= PARODD;
                break;
            case SMP_SERIAL_PARITY_EVEN:
                term.c_cflag |= PARENB;
                term.c_cflag &= ~PARODD;
                break;
            case SMP_SERIAL_PARITY_NONE:
            default:
                term.c_cflag &= ~PARENB;
                break;
        }

        if (flow_control) {
            term.c_iflag |= IXON;
            term.c_iflag |= IXOFF;
        } else {
            term.c_iflag &= ~IXON;
            term.c_iflag &= ~IXOFF;
        }

        ret = tcsetattr(device->fd, TCSANOW, &term);
        if (ret < 0)
            return errno_to_smp_error(errno);

        ret = 0;
    }
#endif

    return ret;
}

ssize_t smp_serial_device_write(SmpSerialDevice *device, const void *buf,
        size_t size)
{
    ssize_t ret;

    ret = write(device->fd, buf, size);

    return (ret < 0) ? errno_to_smp_error(errno) : ret;
}

ssize_t smp_serial_device_read(SmpSerialDevice *device, void *buf, size_t size)
{
    ssize_t ret;

    ret = read(device->fd, buf, size);

    return (ret < 0) ? errno_to_smp_error(errno) : ret;
}

int smp_serial_device_wait(SmpSerialDevice *device, int timeout_ms)
{
#ifdef HAVE_POLL_H
    struct pollfd pfd;
    int ret;

    pfd.fd = device->fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0)
        return errno_to_smp_error(errno);
    else if (ret == 0)
        return SMP_ERROR_TIMEDOUT;
    else
        return 0;
#else
    return SMP_ERROR_NOT_SUPPORTED;
#endif
}
