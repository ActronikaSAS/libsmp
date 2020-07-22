/* libsmp
 * Copyright (C) 2020 Actronika SAS
 *     Author: Hugo Bouchard <hugo.bouchard@actronika.com>
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
#include "libsmp-private.h"

void smp_serial_device_init(SmpSerialDevice *device)
{
    device->fd = -1;
}

int smp_serial_device_open(SmpSerialDevice *device, const char *path)
{
    device->fd = 1;

    return device->fd;
}

void smp_serial_device_close(SmpSerialDevice *device)
{
    device->fd = -1;
}

intptr_t smp_serial_device_get_fd(SmpSerialDevice *device)
{
    return (device->fd < 0) ? SMP_ERROR_BAD_FD : device->fd;
}

int smp_serial_device_set_config(SmpSerialDevice *device,
        SmpSerialBaudrate baudrate, SmpSerialParity parity,
        int flow_control)
{
    return SMP_ERROR_NOT_SUPPORTED;
}

ssize_t smp_serial_device_write(SmpSerialDevice *device, const void *buf,
        size_t size)
{
    return 0;
}

ssize_t smp_serial_device_read(SmpSerialDevice *device, void *buf, size_t size)
{
    return 0;
}

int smp_serial_device_wait(SmpSerialDevice *device, int timeout_ms)
{
    return SMP_ERROR_NOT_SUPPORTED;
}
