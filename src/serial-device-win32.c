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

#include <Windows.h>

#include "libsmp-private.h"

static int win_error_to_smp_error(DWORD error)
{
    switch (error) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return SMP_ERROR_NOT_FOUND;
        case ERROR_ACCESS_DENIED:
        case ERROR_INVALID_ACCESS:
            return SMP_ERROR_PERM;
        case ERROR_INVALID_HANDLE:
            return SMP_ERROR_BAD_FD;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return SMP_ERROR_NO_MEM;
        case ERROR_NOT_SUPPORTED:
            return SMP_ERROR_NOT_SUPPORTED;
        case ERROR_DEV_NOT_EXIST:
            return SMP_ERROR_NO_DEVICE;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
            return SMP_ERROR_EXIST;
        case ERROR_INVALID_PARAMETER:
            return SMP_ERROR_INVALID_PARAM;
        case ERROR_BUSY:
            return SMP_ERROR_BUSY;
        default:
            return SMP_ERROR_OTHER;
    }
}

static int get_last_error_as_smp_error()
{
    return win_error_to_smp_error(GetLastError());
}

/* SerialDevice API */
void smp_serial_device_init(SmpSerialDevice *device)
{
    device->handle = INVALID_HANDLE_VALUE;
}

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
        return get_last_error_as_smp_error();

    dcb.DCBlength = sizeof(DCB);

    bret = GetCommState(handle, &dcb);
    if (!bret) {
        ret = get_last_error_as_smp_error();
        CloseHandle(handle);
        return ret;
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
        ret = get_last_error_as_smp_error();
        CloseHandle(handle);
        return ret;
    }

    /* Set com timeouts so we will never block on read */
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    bret = SetCommTimeouts(handle, &timeouts);
    if (!bret) {
        ret = get_last_error_as_smp_error();
        CloseHandle(handle);
        return ret;
    }

    device->handle = handle;
    return 0;
}

void smp_serial_device_close(SmpSerialDevice *device)
{
    CloseHandle(device->handle);
    device->handle = INVALID_HANDLE_VALUE;
}

intptr_t smp_serial_device_get_fd(SmpSerialDevice *device)
{
    return (intptr_t) device->handle;
}

int smp_serial_device_set_config(SmpSerialDevice *device,
        SmpSerialBaudrate baudrate, SmpSerialParity parity, int flow_control)
{
    DCB dcb = { 0, };
    BOOL bret;

    dcb.DCBlength = sizeof(DCB);

    bret = GetCommState(device->handle, &dcb);
    if (!bret)
        return get_last_error_as_smp_error();

    switch (baudrate) {
        case SMP_SERIAL_BAUDRATE_1200:
            dcb.BaudRate = CBR_1200;
            break;
        case SMP_SERIAL_BAUDRATE_2400:
            dcb.BaudRate = CBR_2400;
            break;
        case SMP_SERIAL_BAUDRATE_4800:
            dcb.BaudRate = CBR_4800;
            break;
        case SMP_SERIAL_BAUDRATE_9600:
            dcb.BaudRate = CBR_9600;
            break;
        case SMP_SERIAL_BAUDRATE_19200:
            dcb.BaudRate = CBR_19200;
            break;
        case SMP_SERIAL_BAUDRATE_38400:
            dcb.BaudRate = CBR_38400;
            break;
        case SMP_SERIAL_BAUDRATE_57600:
            dcb.BaudRate = CBR_57600;
            break;
        case SMP_SERIAL_BAUDRATE_115200:
            dcb.BaudRate = CBR_115200;
            break;
        case SMP_SERIAL_BAUDRATE_230400:
            dcb.BaudRate = 230400;
            break;
        case SMP_SERIAL_BAUDRATE_460800:
            dcb.BaudRate = 460800;
            break;
        case SMP_SERIAL_BAUDRATE_921600:
            dcb.BaudRate = 921600;
            break;
        case SMP_SERIAL_BAUDRATE_1000000:
            dcb.BaudRate = 1000000;
            break;
        case SMP_SERIAL_BAUDRATE_2000000:
            dcb.BaudRate = 2000000;
            break;
        case SMP_SERIAL_BAUDRATE_4000000:
            dcb.BaudRate = 4000000;
            break;
        default:
            return SMP_ERROR_INVALID_PARAM;
    }

    switch (parity) {
        case SMP_SERIAL_PARITY_EVEN:
            dcb.fParity = TRUE;
            dcb.Parity = EVENPARITY;
            break;
        case SMP_SERIAL_PARITY_ODD:
            dcb.fParity = TRUE;
            dcb.Parity = ODDPARITY;
            break;
        case SMP_SERIAL_PARITY_NONE:
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
        return get_last_error_as_smp_error();

    return 0;
}

ssize_t smp_serial_device_write(SmpSerialDevice *device, const void *buf,
        size_t size)
{
    DWORD wbytes;
    BOOL bret;

    if (size > MAXDWORD)
        return SMP_ERROR_OVERFLOW;

    bret = WriteFile(device->handle, buf, (DWORD) size, &wbytes, NULL);
    if (!bret) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            /* This likely indicate that the port has been disconnected/removed
             */
            return SMP_ERROR_PIPE;
        }
        return get_last_error_as_smp_error();
    }

    return wbytes;
}

ssize_t smp_serial_device_read(SmpSerialDevice *device, void *buf, size_t size)
{
    DWORD rbytes;
    BOOL bret;

    if (size > MAXDWORD)
        return SMP_ERROR_OVERFLOW;

    bret = ReadFile(device->handle, buf, (DWORD) size, &rbytes, NULL);
    if (!bret) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            /* This likely indicate that the port has been disconnected/removed
             */
            return SMP_ERROR_PIPE;

        }
        return get_last_error_as_smp_error();
    }

    return rbytes;
}

int smp_serial_device_wait(SmpSerialDevice *device, int timeout_ms)
{
    int ret;

    if (timeout_ms < 0)
        timeout_ms = INFINITE;

    ret = WaitForSingleObject(device->handle, timeout_ms);
    switch (ret) {
        case WAIT_OBJECT_0:
            return 0;
        case WAIT_TIMEOUT:
            return SMP_ERROR_TIMEDOUT;
        case WAIT_FAILED:
            if (GetLastError() == ERROR_GEN_FAILURE) {
                /* this is the error code which is returned when device has
                 * been removed */
                return SMP_ERROR_PIPE;
            }
            return SMP_ERROR_OTHER;
        case WAIT_ABANDONED:
        default:
            return SMP_ERROR_OTHER;
    }
}
