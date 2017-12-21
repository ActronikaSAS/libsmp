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

#ifndef SERIAL_DEVICE_H
#define SERIAL_DEVICE_H

#include <stdint.h>
#include "libsmp.h"
#include "libsmp-private.h"

int smp_serial_device_open(const char *device);
void smp_serial_device_close(int fd);
int smp_serial_device_set_config(int fd, SmpSerialFrameBaudrate baudrate,
        SmpSerialFrameParity parity, int flow_control);
ssize_t smp_serial_device_write(int fd, const void *buf, size_t size);
ssize_t smp_serial_device_read(int fd, void *buf, size_t size);

#endif
