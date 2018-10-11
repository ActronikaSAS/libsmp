/* libsmp
 * Copyright (C) 2018 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 *             Sylvain Gaultier <sylvain.gaultier@actronika.com>
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

#include "serial-device.h"

#include "Arduino.h"
#include "HardwareSerial.h"
#include "libsmp-private.h"

/* Arduino APIs are not coherent between hardware so add a custom interface */
class ArduinoSerialCompatIface
{
public:
    virtual void begin(uint32_t baudrate) = 0;
    virtual void begin(uint32_t baudrate, uint8_t config) = 0;
    virtual void end(void) = 0;

    virtual int available(void) = 0;
    virtual int read(void) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;

    virtual operator bool() = 0;
};

template<class T, typename ConfigType = uint8_t>
class ArduinoSerialCompat : public ArduinoSerialCompatIface
{
public:
    ArduinoSerialCompat(T *ptr) : m_ptr(ptr) {}

    void begin(uint32_t baudrate) { m_ptr->begin(baudrate); }

    /* use a trampoline to cast config to the right type */
    void begin(uint32_t baudrate, uint8_t config) {
        beginInternal(baudrate, static_cast<ConfigType>(config));
    }

    void end(void) { m_ptr->end(); }

    int available(void) { return m_ptr->available(); }
    int read(void) { return m_ptr->read(); }
    size_t write(const uint8_t *buffer, size_t size) {
        return m_ptr->write(buffer, size);
    }

    operator bool() { return (static_cast<bool>(*m_ptr)); }

private:
    inline void beginInternal(uint32_t baudrate, ConfigType config) {
        m_ptr->begin(baudrate, config);
    }

    ArduinoSerialCompat();
    T *m_ptr;
};

typedef struct
{
    const char *name;
    ArduinoSerialCompatIface *dev;
} DeviceMap;

#if defined(ARDUINO_SAM_DUE)    /* Arduino SAM Due */
static ArduinoSerialCompat<UARTClass, UARTClass::UARTModes> serial_compat(&Serial);
static ArduinoSerialCompat<UARTClass, UARTClass::UARTModes> serial1_compat(&Serial1);
static ArduinoSerialCompat<UARTClass, UARTClass::UARTModes> serial2_compat(&Serial2);
static ArduinoSerialCompat<UARTClass, UARTClass::UARTModes> serial3_compat(&Serial3);
static ArduinoSerialCompat<Serial_> serial_usb_compat(&SerialUSB);

static DeviceMap devmap[] = {
    { "serial0", &serial_compat },
    { "serial1", &serial1_compat },
    { "serial2", &serial2_compat },
    { "serial3", &serial3_compat },
    { "serialUSB", &serial_usb_compat },
};
#elif defined(CORE_TEENSY) && defined(__MK66FX1M0__)    /* Teensy 3.6 */
/* Serial lacks begin(baudrate, mode) so define it ourself */
class TeensyUSBSerial : public usb_serial_class
{
public:
    void begin(long baudrate) { usb_serial_class::begin(baudrate); }
    void begin(long baudrate, uint8_t config) {
        /* baudrate and config are not relevent in USB */
        begin(baudrate);
    }
};
static TeensyUSBSerial teensy_usb_serial;

static ArduinoSerialCompat<TeensyUSBSerial> serial_compat(&teensy_usb_serial);
static ArduinoSerialCompat<HardwareSerial> serial1_compat(&Serial1);
static ArduinoSerialCompat<HardwareSerial> serial2_compat(&Serial2);
static ArduinoSerialCompat<HardwareSerial> serial3_compat(&Serial3);

static DeviceMap devmap[] = {
    { "serial0", &serial_compat },
    { "serial1", &serial1_compat },
    { "serial2", &serial2_compat },
    { "serial3", &serial3_compat },
};
#else    /* Other Arduino boards */
/* for other arduino board, rely on HAVE_HWSERIALxx defines */
#ifdef HAVE_HWSERIAL0
static ArduinoSerialCompat<HardwareSerial> serial_compat(&Serial);
#endif
#ifdef HAVE_HWSERIAL1
static ArduinoSerialCompat<HardwareSerial> serial1_compat(&Serial1);
#endif
#ifdef HAVE_HWSERIAL2
static ArduinoSerialCompat<HardwareSerial> serial2_compat(&Serial2);
#endif
#ifdef HAVE_HWSERIAL3
static ArduinoSerialCompat<HardwareSerial> serial3_compat(&Serial3);
#endif

static DeviceMap devmap[] = {
#ifdef HAVE_HWSERIAL0
    { "serial0", &serial_compat },
#endif
#ifdef HAVE_HWSERIAL1
    { "serial1", &serial1_compat },
#endif
#ifdef HAVE_HWSERIAL2
    { "serial2", &serial2_compat },
#endif
#ifdef HAVE_HWSERIAL3
    { "serial3", &serial3_compat },
#endif
};
#endif

static int get_device_fd_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < SMP_N_ELEMENTS(devmap); i++) {
        if (strcmp(name, devmap[i].name) == 0)
            return i;
    }

    return SMP_ERROR_NO_DEVICE;
}

static ArduinoSerialCompatIface *get_device_from_fd(int fd)
{
    if ((size_t)fd >= SMP_N_ELEMENTS(devmap))
        return NULL;

    return devmap[fd].dev;
}

/* SerialDevice API */
int smp_serial_device_open(SmpSerialDevice *sdev, const char *path)
{
    ArduinoSerialCompatIface *serial;
    int fd;

    fd = get_device_fd_by_name(path);
    if (fd < 0)
        return fd;

    serial = get_device_from_fd(fd);
    serial->begin(115200);

    /* wait for device to be opened */
    while (!(*serial));

    sdev->fd = fd;
    return 0;
}

void smp_serial_device_close(SmpSerialDevice *sdev)
{
    ArduinoSerialCompatIface *serial;

    serial = get_device_from_fd(sdev->fd);
    if (serial == NULL)
        return;

    serial->end();
    sdev->fd = -1;
}

intptr_t smp_serial_device_get_fd(SmpSerialDevice *sdev)
{
    return (sdev->fd < 0) ? SMP_ERROR_BAD_FD : sdev->fd;
}

int smp_serial_device_set_config(SmpSerialDevice *sdev,
        SmpSerialBaudrate baudrate, SmpSerialParity parity, int flow_control)
{
    ArduinoSerialCompatIface *serial;
    long br;
    long mode;

    serial = get_device_from_fd(sdev->fd);
    if (serial == NULL)
        return SMP_ERROR_NO_DEVICE;

    if (flow_control != 0) {
        /* Not supported */
        return SMP_ERROR_INVALID_PARAM;
    }

    switch (baudrate) {
        case SMP_SERIAL_BAUDRATE_1200:
            br = 1200;
            break;
        case SMP_SERIAL_BAUDRATE_2400:
            br = 2400;
            break;
        case SMP_SERIAL_BAUDRATE_4800:
            br = 4800;
            break;
        case SMP_SERIAL_BAUDRATE_9600:
            br = 9600;
            break;
        case SMP_SERIAL_BAUDRATE_19200:
            br = 19200;
            break;
        case SMP_SERIAL_BAUDRATE_38400:
            br = 38400;
            break;
        case SMP_SERIAL_BAUDRATE_57600:
            br = 57600;
            break;
        case SMP_SERIAL_BAUDRATE_115200:
            br = 115200;
            break;
        default:
            return SMP_ERROR_INVALID_PARAM;
    }

    switch (parity) {
        case SMP_SERIAL_PARITY_NONE:
            mode = SERIAL_8N1;
            break;
        case SMP_SERIAL_PARITY_ODD:
            mode = SERIAL_8O1;
            break;
        case SMP_SERIAL_PARITY_EVEN:
            mode = SERIAL_8E1;
            break;
        default:
            return SMP_ERROR_INVALID_PARAM;
    }

    serial->end();
    serial->begin(br, mode);

    /* wait for device to be opened */
    while (!(*serial));
    return 0;
}

ssize_t smp_serial_device_write(SmpSerialDevice *sdev, const void *buf,
        size_t size)
{
    ArduinoSerialCompatIface *serial;

    serial = get_device_from_fd(sdev->fd);
    if (serial == NULL)
        return SMP_ERROR_NO_DEVICE;

    return serial->write((const uint8_t *) buf, size);
}

ssize_t smp_serial_device_read(SmpSerialDevice *sdev, void *buf, size_t size)
{
    ArduinoSerialCompatIface *serial;
    size_t i;
    int incomming_byte;

    serial = get_device_from_fd(sdev->fd);
    if (serial == NULL)
        return SMP_ERROR_NO_DEVICE;

    for (i = 0; i < size; i++) {
        if (serial->available() == 0) {
            break;
        }

        incomming_byte = serial->read();
        if (incomming_byte < 0) {
            break;
        }

        ((uint8_t *) buf)[i] = (uint8_t) incomming_byte;
    }
    return i;
}

int smp_serial_device_wait(SmpSerialDevice *device, int timeout_ms)
{
    ArduinoSerialCompatIface *serial;

    serial = get_device_from_fd(device->fd);
    if (serial == NULL)
        return SMP_ERROR_NO_DEVICE;

    /* do we already have some data ? */
    if (serial->available() > 0)
        return 0;

    if (timeout_ms < 0) {
        /* never timeout */
        while (serial->available() <= 0);

        return 0;
    } else {
        unsigned long start_time = millis();
        unsigned long current_time;
        start_time = millis();

        for (current_time = millis();
                (current_time - start_time) < (unsigned long) timeout_ms;
                current_time = millis()) {
            if (serial->available() > 0)
                return 0;
        }

        /* make a last try */
        if (serial->available() > 0)
            return 0;

        return SMP_ERROR_TIMEDOUT;
    }
}
