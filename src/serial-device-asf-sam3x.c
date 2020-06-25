/* libsmp
 * Copyright (C) 2018 Actronika SAS
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

/********************************************************************/
/* Includes                                                         */
/********************************************************************/
#include <asf.h>
#include <string.h>
#include "serial-device.h"
#include "config.h"
#include "libsmp-private.h"

/********************************************************************/
/* Defines                                                          */
/********************************************************************/
/* All bits in the interrupt mask */
#define MASK_ALL_INTERRUPTS             (0xffffffffUL)

/********************************************************************/
/* Local types definition                                           */
/********************************************************************/
typedef enum
{
    SERIAL_DEVICE_USB = 0,
    SERIAL_DEVICE_USART0,
    SERIAL_DEVICE_USART1,
    SERIAL_DEVICE_USART2,
    SERIAL_DEVICE_USART3,
    NB_SERIAL_DEVICES,

} serial_devices_index_t;

typedef enum
{
    DEVICE_TYPE_USART,
    DEVICE_TYPE_USB

} device_type_t;

typedef struct
{
    const char * device_name;
    device_type_t device_type;
    Usart *p_usart;     /* Only for USART device configuration, NULL otherwise */
    IRQn_Type IRQ_num;  /* Only for USART device configuration, 0 otherwise */

} device_cfg_t;

typedef struct
{
    uint8_t cbuf[SMP_SERIAL_DEVICE_RX_BUFFER_SIZE];
    uint8_t rindex;
    uint8_t windex;

} rx_buffer_t;

/********************************************************************/
/* Local constants                                                  */
/********************************************************************/
const device_cfg_t devices_cfg[NB_SERIAL_DEVICES] = {
    { "USB",    DEVICE_TYPE_USB,   NULL,   0           },   /* SERIAL_DEVICE_USB */
    { "USART0", DEVICE_TYPE_USART, USART0, USART0_IRQn },   /* SERIAL_DEVICE_USART0 */
    { "USART1", DEVICE_TYPE_USART, USART1, USART1_IRQn },   /* SERIAL_DEVICE_USART1 */
    { "USART2", DEVICE_TYPE_USART, USART2, USART2_IRQn },   /* SERIAL_DEVICE_USART2 */
    { "USART3", DEVICE_TYPE_USART, USART3, USART3_IRQn }    /* SERIAL_DEVICE_USART3 */
};

/********************************************************************/
/* Local variables                                                  */
/********************************************************************/
/* Reception buffer for all serial devices (use serial_devices_index_t as index) */
rx_buffer_t g_rx_buffers[NB_SERIAL_DEVICES];

/* Rx event semaphore to wake up waiting task (use serial_devices_index_t as index) */
xSemaphoreHandle rx_event_semaphores[NB_SERIAL_DEVICES];

/* Flag used to authorize USB transfer when USB host is ready */
bool g_flag_autorize_cdc_transfert;

/********************************************************************/
/* Private functions prototypes                                     */
/********************************************************************/
static const device_cfg_t *get_device_from_fd(int fd);

static void usart_serial_start(const device_cfg_t *device, usart_serial_options_t usart_options);
static void usart_serial_stop(const device_cfg_t *device);
static void USART_Handler(serial_devices_index_t index, signed portBASE_TYPE *xHigherPriorityTaskWoken);

/********************************************************************/
/* Private functions                                                */
/********************************************************************/
/**
 * Returns a pointer to the requested device configuration structure.
 *
 * @return Pointer to device configuration structure, NULL if device not found
 */
static const device_cfg_t *get_device_from_fd(int fd)
{
    if(NB_SERIAL_DEVICES <= fd)
        return NULL;

    return &devices_cfg[fd];
}

/**
 * Start the USART serial device
 *
 * @param[in] Pointer to a USART device
 */
static void usart_serial_start(const device_cfg_t *device, usart_serial_options_t usart_options)
{
   /* Initialize USART and enable interrupts */
    usart_serial_init(device->p_usart, &usart_options);
    usart_enable_interrupt(device->p_usart, US_IER_RXRDY);
    NVIC_EnableIRQ(device->IRQ_num);
}

/**
 * Stop the USART serial device
 *
 * @param[in] Pointer to a USART device
 */
static void usart_serial_stop(const device_cfg_t *device)
{
    usart_disable_interrupt(device->p_usart, MASK_ALL_INTERRUPTS);
    NVIC_DisableIRQ(device->IRQ_num);
    usart_reset(device->p_usart);
}

/**
 * USART0 rx interrupt handler
 */
void USART0_Handler(void)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    USART_Handler(SERIAL_DEVICE_USART0, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken was set to true you we should yield */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/**
 * USART1 rx interrupt handler
 */
void USART1_Handler(void)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    USART_Handler(SERIAL_DEVICE_USART1, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken was set to true you we should yield */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/**
 * USART2 rx interrupt handler
 */
void USART2_Handler(void)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    USART_Handler(SERIAL_DEVICE_USART2, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken was set to true you we should yield */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/**
 * USART3 rx interrupt handler
 */
void USART3_Handler(void)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    USART_Handler(SERIAL_DEVICE_USART3, &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken was set to true you we should yield */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/**
 * Common USART rx interrupt handler
 */
static void USART_Handler(serial_devices_index_t index, signed portBASE_TYPE *xHigherPriorityTaskWoken)
{
    Usart *p_usart;
    rx_buffer_t *rx_buffer;
    uint32_t value;
    uint32_t ret;

    p_usart = devices_cfg[index].p_usart;
    rx_buffer = &g_rx_buffers[index];

    /* Get received byte */
    ret = usart_read(p_usart, &value);

    /* If a byte could be read */
    if (0U == ret) {
        /* Store it into rx buffer */
        rx_buffer->cbuf[rx_buffer->windex] = (uint8_t)value;

        /* Increment write index and roll over if at the end (circular buffer) */
        rx_buffer->windex++;
        if (rx_buffer->windex >= SMP_SERIAL_DEVICE_RX_BUFFER_SIZE)
            rx_buffer->windex = 0;

        /* Wakeup Rx task */
        xSemaphoreGiveFromISR(rx_event_semaphores[index], xHigherPriorityTaskWoken);
    }
}

/**
 * After the device enumeration (detecting and identifying USB devices), the USB host starts the
 * device configuration. When the USB CDC interface from the device is accepted by the host,
 * the USB host enables this interface and this callback function is called and return true.
 * Thus, when this event is received, the data transfer on CDC interface are authorized.
 */
bool usb_callback_cdc_enable(void)
{
    g_flag_autorize_cdc_transfert = true;
    return true;
}

void usb_callback_cdc_disable(void)
{
    g_flag_autorize_cdc_transfert = false;
}

void usb_rx_notify_callback(void)
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* Wakeup Rx task */
    xSemaphoreGiveFromISR(rx_event_semaphores[SERIAL_DEVICE_USB], &xHigherPriorityTaskWoken);

    /* If xHigherPriorityTaskWoken was set to true you we should yield */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/********************************************************************/
/* Public functions                                                 */
/********************************************************************/

/**
 * Initializes the serial device
 *
 * @param[in] Pointer to serial device structure
 */
void smp_serial_device_init(SmpSerialDevice *sdev)
{
    sdev->fd = -1;
}

/**
 * Opens the serial device
 *
 * @param[in] Pointer to serial device structure
 * @param[in] Path to microcontroller device to attach to serial device structure
 *
 * @return Index of device in device configuration structure to be used as fd
 */
int smp_serial_device_open(SmpSerialDevice *sdev, const char *path)
{
    int ret = SMP_ERROR_NO_DEVICE;
    serial_devices_index_t i;
    usart_serial_options_t default_usart_options;

    /* Find which microcontroller device to open */
    for (i = 0U; i < NB_SERIAL_DEVICES; i++) {
        if (strcmp(path, devices_cfg[i].device_name) == 0) {
            /* Create Rx event semaphore to wake up waiting task (must be created before corresponding interrupt is enabled) */
            vSemaphoreCreateBinary(rx_event_semaphores[i]);

            /* Initialize corresponding serial device */
            if (DEVICE_TYPE_USB == devices_cfg[i].device_type) {
                udc_start();
            }
            else {
                /* Set default USART options */
                default_usart_options.baudrate = 115200;
                default_usart_options.charlength = US_MR_CHRL_8_BIT;
                default_usart_options.paritytype = US_MR_PAR_NO;
                default_usart_options.stopbits = US_MR_NBSTOP_1_BIT;

                /* Start USART */
                usart_serial_start(&devices_cfg[i], default_usart_options);
            }

            /* Set index of device in devices_cfg table to be the file descriptor */
            sdev->fd = i;
            ret = i;

            break;
        }
    }

    return ret;
}

/**
 * Closes the serial device
 *
 * @param[in] Pointer to serial device structure
 */
void smp_serial_device_close(SmpSerialDevice *sdev)
{
    const device_cfg_t *device;

    device = get_device_from_fd(sdev->fd);
    if (NULL != device) {
        if (DEVICE_TYPE_USB == device->device_type) {
            udc_stop();
        }
        else {
            usart_serial_stop(device);
        }
    }
}

/**
 * Get the file descriptor of the serial device structure
 *
 * @param[in] Pointer to serial device structure
 */
intptr_t smp_serial_device_get_fd(SmpSerialDevice *sdev)
{
    return (sdev->fd < 0) ? SMP_ERROR_BAD_FD : sdev->fd;
}

/**
 * Configure the serial port with the specified parameters
 *
 * @param[in] Pointer to serial device structure
 * @param[in] Baudrate
 * @param[in] Parity
 * @param[in] Flow control (not supported so ignored)
 */
int smp_serial_device_set_config(SmpSerialDevice *sdev, SmpSerialBaudrate baudrate, SmpSerialParity parity, int flow_control)
{
    const device_cfg_t *device;
    usart_serial_options_t usart_options;

    device = get_device_from_fd(sdev->fd);
    if (NULL == device)
        return SMP_ERROR_BAD_FD;

    /* Not applicable for USB port */
    if (DEVICE_TYPE_USB == device->device_type)
        return 0;

    /* First stop serial port */
    usart_serial_stop(device);

    /* Set all other configuration parameters not specified in function parameters */
    usart_options.charlength  = US_MR_CHRL_8_BIT;
    usart_options.stopbits    = US_MR_NBSTOP_1_BIT;

    /* Set baudrate */
    switch (baudrate) {
        case SMP_SERIAL_BAUDRATE_1200:
            usart_options.baudrate = (uint32_t)1200;
            break;
        case SMP_SERIAL_BAUDRATE_2400:
            usart_options.baudrate = (uint32_t)2400;
            break;
        case SMP_SERIAL_BAUDRATE_4800:
            usart_options.baudrate = (uint32_t)4800;
            break;
        case SMP_SERIAL_BAUDRATE_9600:
            usart_options.baudrate = (uint32_t)9600;
            break;
        case SMP_SERIAL_BAUDRATE_19200:
            usart_options.baudrate = (uint32_t)19200;
            break;
        case SMP_SERIAL_BAUDRATE_38400:
            usart_options.baudrate = (uint32_t)38400;
            break;
        case SMP_SERIAL_BAUDRATE_57600:
            usart_options.baudrate = (uint32_t)57600;
            break;
        case SMP_SERIAL_BAUDRATE_115200:
            usart_options.baudrate = (uint32_t)115200;
            break;
        case SMP_SERIAL_BAUDRATE_125000:
            usart_options.baudrate = (uint32_t)125000;
            break;
        default:
            usart_options.baudrate = (uint32_t)115200;
            break;
    }

    /* Set parity */
    switch (parity) {
        case SMP_SERIAL_PARITY_ODD:
            usart_options.paritytype = US_MR_PAR_ODD;
            break;
        case SMP_SERIAL_PARITY_EVEN:
            usart_options.paritytype = US_MR_PAR_EVEN;
            break;
        case SMP_SERIAL_PARITY_NONE:
        default:
            usart_options.paritytype = US_MR_PAR_NO;
            break;
    }

    /* Start USART */
    usart_serial_start(device, usart_options);

    return 0;
}

ssize_t smp_serial_device_write(SmpSerialDevice *sdev, const void *buf, size_t size)
{
    const device_cfg_t *device;
    ssize_t ret = 0;
    iram_size_t remaining_bytes;

    device = get_device_from_fd(sdev->fd);
    if (NULL == device)
        return SMP_ERROR_BAD_FD;

    /* Call proper function according to serial device type */
    if (DEVICE_TYPE_USB == device->device_type) {
        /* If a USB host is connected */
        if(true == g_flag_autorize_cdc_transfert) {
            remaining_bytes = udi_cdc_write_buf(buf, size);
            if ((iram_size_t)0 != remaining_bytes)
                ret = (ssize_t)(size - remaining_bytes);
            else
                ret = size;
        }
    }
    else {
        if (STATUS_OK == usart_serial_write_packet(device->p_usart, buf, size))
            ret = size;
    }

    return ret;
}

ssize_t smp_serial_device_read(SmpSerialDevice *sdev, void *buf, size_t size)
{
    const device_cfg_t *device;
    rx_buffer_t *rx_buffer;
    int value;
    size_t i;

    device = get_device_from_fd(sdev->fd);
    if (NULL == device)
        return SMP_ERROR_BAD_FD;

    if (DEVICE_TYPE_USB == device->device_type) {
        for (i = 0; i < size; i++) {
            /* Check if no more data to read as the udi_cdc_getc will wait if there is no byte to read */
            if (!udi_cdc_is_rx_ready())
                break;

            /* Read and store rx byte into buffer */
            value = udi_cdc_getc();
            ((uint8_t *)buf)[i] = (uint8_t)value;
        }
    }
    else {
        rx_buffer = &g_rx_buffers[sdev->fd];

        for (i = 0; i < size; i++) {
            /* If no more data to read */
            if (rx_buffer->rindex == rx_buffer->windex)
                break;

            /* Store rx byte into buffer */
            ((uint8_t *)buf)[i] = rx_buffer->cbuf[rx_buffer->rindex];

            /* Increment read index and roll over if at the end (circular buffer) */
            rx_buffer->rindex++;
            if (rx_buffer->rindex >= SMP_SERIAL_DEVICE_RX_BUFFER_SIZE)
                rx_buffer->rindex = 0;
        }
    }

    return i;
}

int smp_serial_device_wait(SmpSerialDevice *sdev, int timeout_ms)
{
    portTickType timeout;
    signed portBASE_TYPE ret;

    if (NB_SERIAL_DEVICES <= sdev->fd)
        return SMP_ERROR_BAD_FD;

    /* Convert timeout from ms to OS ticks */
    if(0 > timeout_ms)
        timeout = portMAX_DELAY;
    else
        timeout = (portTickType)timeout_ms / portTICK_RATE_MS;

    ret = xSemaphoreTake(rx_event_semaphores[sdev->fd], timeout);
    if (errQUEUE_EMPTY == ret)
        return SMP_ERROR_TIMEDOUT;

    return 0;
}
