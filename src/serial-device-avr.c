/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "serial-device.h"
#include "libsmp-private.h"

typedef struct
{
    volatile uint8_t *dr;
    volatile uint8_t *csr_a;
    volatile uint8_t *csr_b;
    volatile uint8_t *csr_c;
    volatile uint8_t *br_l;
    volatile uint8_t *br_h;
} UARTDeviceRegisters;

typedef struct
{
    char *name;

    UARTDeviceRegisters regs;

    uint8_t cbuf[SMP_SERIAL_DEVICE_AVR_CBUF_SIZE];
    uint8_t rindex;
    uint8_t windex;
} UARTDevice;

static void rx_interrupt_handler(UARTDevice *dev);

/* macros helper to define UARTDevice and interrupt handler */
#define DEFINE_UART_DEVICE(sname, UDR, UCSRA, UCSRB, UCSRC, UBRRL, UBRRH) \
{ \
    .name = sname, \
    .regs = { \
        .dr = &UDR, \
        .csr_a = &UCSRA, \
        .csr_b = &UCSRB, \
        .csr_c = &UCSRC, \
        .br_l = &UBRRL, \
        .br_h = &UBRRH, \
    }, \
}

#define DEFINE_RX_ISR(device, vector) \
ISR(vector) \
{ \
    rx_interrupt_handler(device); \
}

/* UART devices definitions according to used avr
 * Assume all registers have the same layout across uart module */

/* Atmega 328p */
#if defined(__AVR_ATmega328P__)
static UARTDevice avr_uart_devices[] = {
#if defined(AVR_ENABLE_SERIAL0)
    DEFINE_UART_DEVICE("serial0", UDR0, UCSR0A, UCSR0B, UCSR0C, UBRR0L, UBRR0H),
#endif
};

#if defined(AVR_ENABLE_SERIAL0)
DEFINE_RX_ISR(&avr_uart_devices[0], USART_RX_vect);
#endif

#elif defined(__AVR_ATmega2560__)
static UARTDevice avr_uart_devices[] = {
#if defined(AVR_ENABLE_SERIAL0)
    DEFINE_UART_DEVICE("serial0", UDR0, UCSR0A, UCSR0B, UCSR0C, UBRR0L, UBRR0H),
#endif
#if defined(AVR_ENABLE_SERIAL1)
    DEFINE_UART_DEVICE("serial1", UDR1, UCSR1A, UCSR1B, UCSR1C, UBRR1L, UBRR1H),
#endif
#if defined(AVR_ENABLE_SERIAL2)
    DEFINE_UART_DEVICE("serial2", UDR2, UCSR2A, UCSR2B, UCSR2C, UBRR2L, UBRR2H),
#endif
#if defined(AVR_ENABLE_SERIAL3)
    DEFINE_UART_DEVICE("serial3", UDR3, UCSR3A, UCSR3B, UCSR3C, UBRR3L, UBRR3H),
#endif
};

#if defined(AVR_ENABLE_SERIAL0)
DEFINE_RX_ISR(&avr_uart_devices[0], USART0_RX_vect);
#endif
#if defined(AVR_ENABLE_SERIAL1)
DEFINE_RX_ISR(&avr_uart_devices[1], USART1_RX_vect);
#endif
#if defined(AVR_ENABLE_SERIAL2)
DEFINE_RX_ISR(&avr_uart_devices[2], USART2_RX_vect);
#endif
#if defined(AVR_ENABLE_SERIAL3)
DEFINE_RX_ISR(&avr_uart_devices[3], USART3_RX_vect);
#endif

#endif

static uint8_t avr_uart_n_devices = SMP_N_ELEMENTS(avr_uart_devices);

/* Some functions now... */
static UARTDevice *get_device_from_fd(int fd)
{
    if (fd >= avr_uart_n_devices)
        return NULL;

    return &avr_uart_devices[fd];
}

static void rx_interrupt_handler(UARTDevice *dev)
{
    dev->cbuf[dev->windex] = *(dev->regs.dr);
    dev->windex++;
    if (dev->windex >= SMP_SERIAL_DEVICE_AVR_CBUF_SIZE)
        dev->windex = 0;
}

static void set_baudrate_1200(UARTDevice *device)
{
#undef BAUD
#define BAUD 1200
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_2400(UARTDevice *device)
{
#undef BAUD
#define BAUD 2400
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_4800(UARTDevice *device)
{
#undef BAUD
#define BAUD 4800
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_9600(UARTDevice *device)
{
#undef BAUD
#define BAUD 9600
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_19200(UARTDevice *device)
{
#undef BAUD
#define BAUD 19200
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_38400(UARTDevice *device)
{
#undef BAUD
#define BAUD 38400
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_57600(UARTDevice *device)
{
#undef BAUD
#define BAUD 57600
#include <util/setbaud.h>
    *device->regs.br_h = UBRRH_VALUE;
    *device->regs.br_l = UBRRL_VALUE;
#if USE_2X
    *device->regs.csr_a |= _BV(U2X0);
#else
    *device->regs.csr_a &= ~(_BV(U2X0));
#endif
}

static void set_baudrate_115200(UARTDevice *device)
{
#undef BAUD
#define BAUD 115200
#include <util/setbaud.h>
    *(device->regs.br_h) = UBRRH_VALUE;
    *(device->regs.br_l) = UBRRL_VALUE;
#if USE_2X
    *(device->regs.csr_a) |= _BV(U2X0);
#else
    *(device->regs.csr_a) &= ~(_BV(U2X0));
#endif
}

static void set_baudrate(UARTDevice *device, SmpSerialFrameBaudrate baudrate)
{
    switch (baudrate) {
        case SMP_SERIAL_FRAME_BAUDRATE_1200:
            set_baudrate_1200(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_2400:
            set_baudrate_2400(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_4800:
            set_baudrate_4800(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_9600:
            set_baudrate_9600(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_19200:
            set_baudrate_19200(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_38400:
            set_baudrate_38400(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_57600:
            set_baudrate_57600(device);
            break;
        case SMP_SERIAL_FRAME_BAUDRATE_115200:
            set_baudrate_115200(device);
            break;
        default:
            break;
    }
}

static int init_uart_device(UARTDevice *device)
{
    UARTDeviceRegisters *regs = &device->regs;

    if (bit_is_set(*regs->csr_b, TXEN0) || bit_is_set(*regs->csr_b, RXEN0)) {
        /* TX/RX is already enable, error out */
        return -EBUSY;
    }

    /* configure baudrate */
    set_baudrate(device, SMP_SERIAL_FRAME_BAUDRATE_115200);

    /* configure data width to 8 */
    *regs->csr_c |= _BV(UCSZ00);
    *regs->csr_c |= _BV(UCSZ01);
    *regs->csr_b &= ~(_BV(UCSZ02));

    /* configure parity to none */
    *regs->csr_c &= ~(_BV(UPM00));
    *regs->csr_c &= ~(_BV(UPM01));

    /* configure stop bit */
    *regs->csr_c &= ~(_BV(USBS0));

    /* enable rx interrupt and disable tx */
    *regs->csr_b |= _BV(RXCIE0);
    *regs->csr_b &= ~(_BV(TXCIE0));

    /* enable rx/tx */
    *regs->csr_b |= _BV(RXEN0);
    *regs->csr_b |= _BV(TXEN0);

    return 0;
}

/* SerialDevice API */
int serial_device_open(const char *device)
{
    int ret = -ENOENT;
    uint8_t i;

    for (i = 0; i < avr_uart_n_devices; i++) {
        if (strcmp(device, avr_uart_devices[i].name) == 0) {
            ret = init_uart_device(&avr_uart_devices[i]);
            break;
        }
    }

    return (ret == 0) ? i : ret;
}

void serial_device_close(int fd)
{
    UARTDevice *dev;
    UARTDeviceRegisters *regs;

    dev = get_device_from_fd(fd);
    if (dev == NULL)
        return;

    regs = &dev->regs;

    /* disable uart device */
    *regs->csr_b &= ~(_BV(RXEN0));
    *regs->csr_b &= ~(_BV(TXEN0));
}

int serial_device_set_config(int fd, SmpSerialFrameBaudrate baudrate,
        int parity, int flow_control)
{
    UARTDevice *dev;
    UARTDeviceRegisters *regs;

    dev = get_device_from_fd(fd);
    if (dev == NULL)
        return -ENOENT;

    regs = &dev->regs;

    /* disable uart device */
    *regs->csr_b &= ~(_BV(RXEN0));
    *regs->csr_b &= ~(_BV(TXEN0));

    set_baudrate(dev, baudrate);

    /* FIXME: don't ignore parity and flow control */

    /* enable rx/tx */
    *regs->csr_b |= _BV(RXEN0);
    *regs->csr_b |= _BV(TXEN0);

    return 0;
}

ssize_t serial_device_write(int fd, const void *buf, size_t size)
{
    UARTDevice *device;
    UARTDeviceRegisters *regs;
    size_t i;
    ssize_t ret = 0;

    device = get_device_from_fd(fd);
    if (device == NULL)
        return -ENOENT;

    regs = &device->regs;

    for (i = 0; i < size; i++) {
        /* wait for a ready window */
        loop_until_bit_is_set(*regs->csr_a, UDRE0);

        *regs->dr = ((const uint8_t *) buf)[i];
        ret++;
    }

    return ret;
}

ssize_t serial_device_read(int fd, void *buf, size_t size)
{
    UARTDevice *dev;
    size_t i;

    dev = get_device_from_fd(fd);
    if (dev == NULL)
        return -ENOENT;

    for (i = 0; i < size; i++) {
        if (dev->rindex == dev->windex)
            break;

        ((uint8_t *) buf)[i] = dev->cbuf[dev->rindex];
        dev->rindex++;
        if (dev->rindex >= SMP_SERIAL_DEVICE_AVR_CBUF_SIZE)
            dev->rindex = 0;
    }

    return i;
}
