/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include <stdint.h>
#include "libsmp.h"
#include "libsmp-private.h"

int serial_device_open(const char *device);
void serial_device_close(int fd);
int serial_device_set_config(int fd, SmpSerialFrameBaudrate baudrate,
        int parity, int flow_control);
ssize_t serial_device_write(int fd, const void *buf, size_t size);
ssize_t serial_device_read(int fd, void *buf, size_t size);

#endif
