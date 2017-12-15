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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "libsmp-private.h"

int serial_device_open(const char *device)
{
    int fd;

    fd = open(device, O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return -errno;

#ifdef HAVE_TERMIOS_H
    if (isatty(fd)) {
        struct termios term;
        int ret;

        ret = tcgetattr(fd, &term);
        if (ret < 0) {
            close(fd);
            return -errno;
        }

        cfsetispeed(&term, B115200);
        cfsetospeed(&term, B115200);
        cfmakeraw(&term);

        ret = tcsetattr(fd, TCSANOW, &term);
        if (ret < 0) {
            close(fd);
            return -errno;
        }
    }
#endif

    return fd;
}

void serial_device_close(int fd)
{
    close(fd);
}

int serial_device_set_config(int fd, SmpSerialFrameBaudrate baudrate,
        SmpSerialFrameParity parity, int flow_control)
{
    int ret = -ENOSYS;

#ifdef HAVE_TERMIOS_H
    if (isatty(fd)) {
        struct termios term;
        speed_t speed;

        ret = tcgetattr(fd, &term);
        if (ret < 0)
            return -errno;

        switch (baudrate) {
            case SMP_SERIAL_FRAME_BAUDRATE_1200:
                speed = B1200;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_2400:
                speed = B2400;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_4800:
                speed = B4800;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_9600:
                speed = B9600;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_19200:
                speed = B19200;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_38400:
                speed = B38400;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_57600:
                speed = B57600;
                break;
            case SMP_SERIAL_FRAME_BAUDRATE_115200:
            default:
                speed = B115200;
                break;
        }
        cfsetispeed(&term, speed);
        cfsetospeed(&term, speed);

        switch (parity) {
            case SMP_SERIAL_FRAME_PARITY_ODD:
                term.c_cflag |= PARENB;
                term.c_cflag |= PARODD;
                break;
            case SMP_SERIAL_FRAME_PARITY_EVEN:
                term.c_cflag |= PARENB;
                term.c_cflag &= ~PARODD;
                break;
            case SMP_SERIAL_FRAME_PARITY_NONE:
            default:
                term.c_cflag &= ~PARENB;
                break;
        }

        if (flow_control)
            term.c_iflag |= IXON;
        else
            term.c_iflag &= ~IXON;

        ret = tcsetattr(fd, TCSANOW, &term);
        if (ret < 0)
            return -errno;

        ret = 0;
    }
#endif

    return ret;
}

ssize_t serial_device_write(int fd, const void *buf, size_t size)
{
    return write(fd, buf, size);
}

ssize_t serial_device_read(int fd, void *buf, size_t size)
{
    return read(fd, buf, size);
}
