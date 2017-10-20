/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */
/**
 * @file
 * \defgroup serial_frame SerialFrame
 *
 * Encode and decode a payload to be sent over a serial line with some
 * error detection.
 */

#include "config.h"

#include "libsmp.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "libsmp-private.h"

#define START_BYTE 0x10
#define END_BYTE 0xFF
#define ESC_BYTE 0x1B

static inline int is_magic_byte(uint8_t byte)
{
    return (byte == START_BYTE || byte == END_BYTE || byte == ESC_BYTE);
}

static uint8_t compute_checksum(const uint8_t *buf, size_t size)
{
    uint8_t checksum = 0;
    size_t i;

    for (i = 0; i < size; i++) {
        checksum ^= buf[i];
    }

    return checksum;
}

static int smp_serial_frame_decoder_init(SmpSerialFrameDecoder *decoder,
        const SmpSerialFrameDecoderCallbacks *cbs, void *userdata)
{
    memset(decoder, 0, sizeof(*decoder));

    decoder->state = SMP_SERIAL_FRAME_DECODER_STATE_WAIT_HEADER;
    decoder->cbs = *cbs;
    decoder->userdata = userdata;

    return 0;
}

static void
smp_serial_frame_decoder_process_byte_inframe(SmpSerialFrameDecoder *decoder,
        uint8_t byte)
{
    switch (byte) {
        case START_BYTE:
            /* we are in a frame without end byte, resync on current byte */
            decoder->cbs.error(SMP_SERIAL_FRAME_ERROR_CORRUPTED,
                    decoder->userdata);
            decoder->frame_offset = 0;
            break;
        case ESC_BYTE:
            decoder->state = SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME_ESC;
            break;
        case END_BYTE: {
           size_t framesize;
           uint8_t cs;

           /* framesize is without the CRC */
           framesize = decoder->frame_offset - 1;

           /* frame complete, check crc and call user callback */
           cs = compute_checksum(decoder->frame, framesize);
           if (cs != decoder->frame[decoder->frame_offset - 1]) {
               decoder->cbs.error(SMP_SERIAL_FRAME_ERROR_CORRUPTED,
                       decoder->userdata);
           } else {
               decoder->cbs.new_frame(decoder->frame, framesize,
                       decoder->userdata);
           }

           decoder->state = SMP_SERIAL_FRAME_DECODER_STATE_WAIT_HEADER;
           break;
        }
        default:
            if (decoder->frame_offset >= SMP_SERIAL_FRAME_MAX_FRAME_SIZE) {
                decoder->cbs.error(SMP_SERIAL_FRAME_ERROR_PAYLOAD_TOO_BIG,
                        decoder->userdata);
                break;
            }

            decoder->frame[decoder->frame_offset++] = byte;
            break;
    }
}

static void
smp_serial_frame_decoder_process_byte(SmpSerialFrameDecoder *decoder,
        uint8_t byte)
{
    switch (decoder->state) {
        case SMP_SERIAL_FRAME_DECODER_STATE_WAIT_HEADER:
            if (byte == START_BYTE) {
                decoder->state = SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME;
                decoder->frame_offset = 0;
            }
            break;

        case SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME:
            smp_serial_frame_decoder_process_byte_inframe(decoder, byte);
            break;

        case SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME_ESC:
            if (decoder->frame_offset >= SMP_SERIAL_FRAME_MAX_FRAME_SIZE) {
                decoder->cbs.error(SMP_SERIAL_FRAME_ERROR_PAYLOAD_TOO_BIG,
                        decoder->userdata);
                break;
            }

            decoder->frame[decoder->frame_offset++] = byte;
            decoder->state = SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME;
            break;

        default:
            break;
    }
}

/* dest should be able to contain at least 2 bytes. returns the number of
 * bytes written */
static int smp_serial_frame_write_byte(uint8_t *dest, uint8_t byte)
{
    int offset = 0;

    /* escape special byte */
    if (is_magic_byte(byte))
        dest[offset++] = ESC_BYTE;

    dest[offset++] = byte;

    return offset;
}

static int serial_device_open(const char *device)
{
    int fd;

    fd = open(device, O_RDWR | O_NONBLOCK);
    if (fd < 0)
        return -errno;

#ifdef HAVE_TERMIOS_H
    {
        struct termios term;

        tcgetattr(fd, &term);
        cfsetispeed(&term, B115200);
        cfsetospeed(&term, B115200);
        cfmakeraw(&term);
        tcsetattr(fd, TCSANOW, &term);
    }
#endif

    return fd;
}

#ifdef HAVE_TERMIOS_H
static int set_serial_config(SmpSerialFrameContext *ctx,
        SmpSerialFrameBaudrate baudrate, int parity, int flow_control)
{
    struct termios term;
    speed_t speed;

    tcgetattr(ctx->serial_fd, &term);
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

    if (parity)
        term.c_iflag |= INPCK;
    else
        term.c_iflag &= ~INPCK;

    if (flow_control)
        term.c_iflag |= IXON;
    else
        term.c_iflag &= ~IXON;

    return 0;
}
#else
static int set_serial_config(SmpSerialFrameContext *ctx,
        SmpSerialFrameBaudrate baudrate, int parity, int flow_control)
{
    return -ENOSYS;
}
#endif

/* API */

/**
 * \ingroup serial_frame
 * Initialize a SmpSerialFrameContext using the given serial device and decoder
 * callbacks.
 *
 * @param[in] ctx the SmpSerialFrameContext to initialize
 * @param[in] device path to the serial device to use
 * @param[in] cbs callback to use by the decoder
 * @param[in] userdata a pointer to userdata which will be passed in callbacks.
 *
 * @return 0 on success, a negative errno value otherwise.
 */
int smp_serial_frame_init(SmpSerialFrameContext *ctx, const char *device,
        const SmpSerialFrameDecoderCallbacks *cbs, void *userdata)
{
    int ret;

    return_val_if_fail(ctx != NULL, -EINVAL);
    return_val_if_fail(device != NULL, -EINVAL);
    return_val_if_fail(cbs != NULL, -EINVAL);
    return_val_if_fail(cbs->new_frame != NULL, -EINVAL);
    return_val_if_fail(cbs->error != NULL, -EINVAL);

    ctx->serial_fd = serial_device_open(device);
    if (ctx->serial_fd < 0)
        return ctx->serial_fd;

    ret = smp_serial_frame_decoder_init(&ctx->decoder, cbs, userdata);
    if (ret < 0) {
        close(ctx->serial_fd);
        return ret;
    }

    return 0;
}

/**
 * \ingroup serial_frame
 * Deinitialize a SmpSerialFrameContext
 *
 * @param[in] ctx the SmpSerialFrameContext to deinitialize
 */
void smp_serial_frame_deinit(SmpSerialFrameContext *ctx)
{
    close(ctx->serial_fd);
}

/**
 * \ingroup serial_frame
 * Set serial device config. Depending on the system, it could be not
 * implemented.
 *
 * @param[in] ctx the SmpSerialFrameContext
 * @param[in] baudrate the baudrate
 * @param[in] parity 1 to enable parity, 0 to disable
 * @param[in] flow_control 1 to enable flow control, 0 to disable
 *
 * @return 0 on success, a negative errno otherwise.
 */
int smp_serial_frame_set_config(SmpSerialFrameContext *ctx,
        SmpSerialFrameBaudrate baudrate, int parity, int flow_control)
{
    return_val_if_fail(ctx != NULL, -EINVAL);

    return set_serial_config(ctx, baudrate, parity, flow_control);
}

/**
 * \ingroup serial_frame
 * Get the file descriptor of the opened serial device.
 *
 * @param[in] ctx the SmpSerialFrameContext
 *
 * @return the fd on success, a negative errno value otherwise.
 */
int smp_serial_frame_get_fd(SmpSerialFrameContext *ctx)
{
    return_val_if_fail(ctx != NULL, -EINVAL);

    return (ctx->serial_fd > 0) ? ctx->serial_fd : -EBADF;
}

/**
 * \ingroup serial_frame
 * Encode and send a payload over the serial line.
 *
 * @param[in] ctx the SmpSerialFrameContext
 * @param[in] buf the payload to send
 * @param[in] size size of the payload to send
 *
 * @return 0 on success, a negative errno value otherwise.
 */
int smp_serial_frame_send(SmpSerialFrameContext *ctx, const uint8_t *buf,
        size_t size)
{
    uint8_t txbuf[SMP_SERIAL_FRAME_MAX_FRAME_SIZE];
    size_t payload_size;
    size_t offset = 0;
    size_t i;
    int ret;

    return_val_if_fail(ctx != NULL, -EINVAL);
    return_val_if_fail(buf != NULL, -EINVAL);

    /* first, compute our payload size and make sure it doen't exceed our buffer
     * size. We need at least three extra bytes (START, END and CRC) + a number
     * of escape bytes */
    payload_size = size + 3;

    for (i = 0; i < size; i++) {
        if (is_magic_byte(buf[i]))
            payload_size++;
    }

    if (payload_size > SMP_SERIAL_FRAME_MAX_FRAME_SIZE)
        return -ENOMEM;

    /* prepare the buffer */
    txbuf[offset++] = START_BYTE;
    for (i = 0; i < size; i++)
        offset += smp_serial_frame_write_byte(txbuf + offset, buf[i]);

    offset += smp_serial_frame_write_byte(txbuf + offset,
            compute_checksum(buf, size));
    txbuf[offset++] = END_BYTE;

    /* send it */
    ret = write(ctx->serial_fd, txbuf, offset);
    if (ret < 0)
        return ret;

    if ((size_t) ret != offset)
        return -EFAULT;

    return 0;
}

/**
 * \ingroup serial_frame
 * Process incoming data on the serial file descriptor.
 * Decoded frame or errors during decoding are passed to their respective
 * callbacks.
 *
 * @param[in] ctx the SmpSerialFrameContext
 *
 * @return 0 on success, a negative errno otherwise.
 */
int smp_serial_frame_process_recv_fd(SmpSerialFrameContext *ctx)
{
    ssize_t rbytes;
    char c;

    while (1) {
        rbytes = read(ctx->serial_fd, &c, 1);
        if (rbytes < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                return 0;

            return -errno;
        } else if (rbytes == 0) {
            return 0;
        }

        smp_serial_frame_decoder_process_byte(&ctx->decoder, c);
    }

    return 0;
}
