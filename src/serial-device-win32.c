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
#include <Windows.h>

#include "libsmp-private.h"

static int win_error_to_errno(DWORD error)
{
    switch (error) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return ENOENT;
        case ERROR_ACCESS_DENIED:
            return EPERM;
        case ERROR_INVALID_HANDLE:
            return EBADF;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return ENOMEM;
        case ERROR_INVALID_ACCESS:
            return EACCES;
        case ERROR_NOT_SUPPORTED:
            return ENOSYS;
        case ERROR_DEV_NOT_EXIST:
            return ENODEV;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            return EEXIST;
        case ERROR_INVALID_PARAMETER:
            return EINVAL;
        case ERROR_BUSY:
            return EBUSY;
        default:
            return EFAULT;
    }
}

static int get_last_error_as_errno()
{
    return win_error_to_errno(GetLastError());
}

/* SerialDevice API */
int smp_serial_device_open(SmpSerialDevice *device, const char *path)
{
    HANDLE handle;
    DCB dcb = { 0, };
    COMMTIMEOUTS timeouts;
    BOOL bret;
    int ret;

    handle = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return -get_last_error_as_errno();

    dcb.DCBlength = sizeof(DCB);

    bret = GetCommState(handle, &dcb);
    if (!bret) {
        ret = get_last_error_as_errno();
        CloseHandle(handle);
        return -ret;
    }

    dcb.BaudRate = CBR_115200;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fOutX = dcb.fInX = FALSE;

    bret = SetCommState(handle, &dcb);
    if (!bret) {
        ret = get_last_error_as_errno();
        CloseHandle(handle);
        return -ret;
    }

    /* Set com timeouts so we will never block on read */
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    bret = SetCommTimeouts(handle, &timeouts);
    if (!bret) {
        ret = get_last_error_as_errno();
        CloseHandle(handle);
        return -ret;
    }

    device->handle = handle;
    return 0;
}

void smp_serial_device_close(SmpSerialDevice *device)
{
    CloseHandle(device->handle);
}

intptr_t smp_serial_device_get_fd(SmpSerialDevice *device)
{
    return (intptr_t) device->handle;
}

int smp_serial_device_set_config(SmpSerialDevice *device,
        SmpSerialFrameBaudrate baudrate, SmpSerialFrameParity parity,
        int flow_control)
{
    DCB dcb = { 0, };
    BOOL bret;

    dcb.DCBlength = sizeof(DCB);

    bret = GetCommState(device->handle, &dcb);
    if (!bret)
        return -get_last_error_as_errno();

    switch (baudrate) {
        case SMP_SERIAL_FRAME_BAUDRATE_1200:
            dcb.BaudRate = CBR_1200;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_2400:
            dcb.BaudRate = CBR_2400;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_4800:
            dcb.BaudRate = CBR_4800;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_9600:
            dcb.BaudRate = CBR_9600;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_19200:
            dcb.BaudRate = CBR_19200;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_38400:
            dcb.BaudRate = CBR_38400;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_57600:
            dcb.BaudRate = CBR_57600;
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_115200:
        default:
            dcb.BaudRate = CBR_115200;
            break;
    }

    switch (parity) {
        case SMP_SERIAL_FRAME_PARITY_EVEN:
            dcb.fParity = TRUE;
            dcb.Parity = EVENPARITY;
            break;
        case SMP_SERIAL_FRAME_PARITY_ODD:
            dcb.fParity = TRUE;
            dcb.Parity = ODDPARITY;
            break;
        case SMP_SERIAL_FRAME_PARITY_NONE:
        default:
            dcb.fParity = FALSE;
            dcb.Parity = NOPARITY;
            break;
    }

    if (flow_control)
        dcb.fOutX = dcb.fInX = TRUE;
    else
        dcb.fOutX = dcb.fInX = FALSE;

    bret = SetCommState(device->handle, &dcb);
    if (!bret)
        return -get_last_error_as_errno();

    return 0;
}

ssize_t smp_serial_device_write(SmpSerialDevice *device, const void *buf,
        size_t size)
{
    DWORD wbytes;
    BOOL bret;

    bret = WriteFile(device->handle, buf, size, &wbytes, NULL);
    if (!bret)
        return -get_last_error_as_errno();

    return wbytes;
}

ssize_t smp_serial_device_read(SmpSerialDevice *device, void *buf, size_t size)
{
    DWORD rbytes;
    BOOL bret;

    bret = ReadFile(device->handle, buf, size, &rbytes, NULL);
    if (!bret)
        return get_last_error_as_errno();


    return rbytes;
}
