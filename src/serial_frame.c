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

#include "libsmp.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "libsmp-private.h"

#define START_BYTE 0x10
#define END_BYTE 0xFF
#define ESC_BYTE 0x1B

static const uint8_t crc8_ccitt_lut[256] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
    0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
    0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
    0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
    0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
    0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
    0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
    0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
    0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
    0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
    0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
    0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
    0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
    0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
    0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
    0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
    0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3,
};

static inline int is_magic_byte(uint8_t byte)
{
    return (byte == START_BYTE || byte == END_BYTE || byte == ESC_BYTE);
}

static uint8_t compute_checksum(const uint8_t *buf, size_t size)
{
    uint8_t checksum = 0;
    size_t i;

    for (i = 0; i < size; i++) {
        checksum = crc8_ccitt_lut[checksum ^ buf[i]];
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

    ctx->serial_fd = open(device, O_RDWR | O_NONBLOCK);
    if (ctx->serial_fd < 0)
        return -errno;

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
