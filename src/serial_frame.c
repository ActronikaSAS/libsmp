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
/**
 * @file
 * \defgroup serial_frame SerialFrame
 *
 * Encode and decode a payload to be sent over a serial line with some
 * error detection.
 */

#include "config.h"

#include "libsmp.h"
#include <stdio.h>
#include <string.h>

#include "libsmp-private.h"
#include "serial-device.h"
#include "serial-protocol.h"

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
 * @return 0 on success, a SmpError otherwise.
 */
int smp_serial_frame_init(SmpSerialFrameContext *ctx, const char *device,
        const SmpSerialFrameDecoderCallbacks *cbs, void *userdata)
{
    SmpSerialProtocolDecoder *dec;
    int ret;

    return_val_if_fail(ctx != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(device != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(cbs != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(cbs->new_frame != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(cbs->error != NULL, SMP_ERROR_INVALID_PARAM);

    ctx->cbs = *cbs;
    ctx->userdata = userdata;

    ret = smp_serial_device_open(&ctx->device, device);
    if (ret < 0)
        return ret;

    dec = smp_serial_protocol_decoder_new_from_static(&ctx->decoder,
            sizeof(ctx->decoder), ctx->frame, SMP_SERIAL_FRAME_MAX_FRAME_SIZE);
    if (dec == NULL) {
        smp_serial_device_close(&ctx->device);
        return SMP_ERROR_NO_MEM;
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
    smp_serial_device_close(&ctx->device);
}

/**
 * \ingroup serial_frame
 * Set serial device config. Depending on the system, it could be not
 * implemented.
 *
 * @param[in] ctx the SmpSerialFrameContext
 * @param[in] baudrate the baudrate
 * @param[in] parity the parity configuration
 * @param[in] flow_control 1 to enable flow control, 0 to disable
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_serial_frame_set_config(SmpSerialFrameContext *ctx,
        SmpSerialBaudrate baudrate, SmpSerialParity parity, int flow_control)
{
    return_val_if_fail(ctx != NULL, SMP_ERROR_INVALID_PARAM);

    return smp_serial_device_set_config(&ctx->device, baudrate, parity,
            flow_control);
}

/**
 * \ingroup serial_frame
 * Get the file descriptor of the opened serial device.
 *
 * @param[in] ctx the SmpSerialFrameContext
 *
 * @return the fd (or a handle on Win32) on success, a SmpError otherwise.
 */
intptr_t smp_serial_frame_get_fd(SmpSerialFrameContext *ctx)
{
    return_val_if_fail(ctx != NULL, -1);

    return smp_serial_device_get_fd(&ctx->device);
}

/**
 * \ingroup serial_frame
 * Encode and send a payload over the serial line.
 *
 * @param[in] ctx the SmpSerialFrameContext
 * @param[in] buf the payload to send
 * @param[in] size size of the payload to send
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_serial_frame_send(SmpSerialFrameContext *ctx, const uint8_t *buf,
        size_t size)
{
    uint8_t txbuf[SMP_SERIAL_FRAME_MAX_FRAME_SIZE];
    uint8_t *ptxbuf = txbuf;
    size_t txbuf_size = SMP_SERIAL_FRAME_MAX_FRAME_SIZE;
    ssize_t encoded_size;
    int ret;

    return_val_if_fail(ctx != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(buf != NULL, SMP_ERROR_INVALID_PARAM);

    encoded_size = smp_serial_protocol_encode(buf, size, &ptxbuf, txbuf_size);
    if (encoded_size < 0)
        return encoded_size;

    /* send it */
    ret = smp_serial_device_write(&ctx->device, txbuf, encoded_size);
    if (ret < 0)
        return ret;

    if (ret != encoded_size)
        return SMP_ERROR_OTHER;

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
 * @return 0 on success, a SmpError otherwise.
 */
int smp_serial_frame_process_recv_fd(SmpSerialFrameContext *ctx)
{
    SmpSerialProtocolDecoder *dec = (SmpSerialProtocolDecoder *) &ctx->decoder;
    ssize_t rbytes;
    char c;
    uint8_t *frame;
    size_t framesize;
    int ret;

    while (1) {
        rbytes = smp_serial_device_read(&ctx->device, &c, 1);
        if (rbytes < 0) {
            if (rbytes == SMP_ERROR_WOULD_BLOCK)
                return 0;

            return rbytes;
        } else if (rbytes == 0) {
            return 0;
        }

        ret = smp_serial_protocol_decoder_process_byte(dec, c, &frame,
                &framesize);
        if (ret < 0) {
            SmpSerialFrameError error;

            if (ret == SMP_ERROR_TOO_BIG)
                error = SMP_SERIAL_FRAME_ERROR_PAYLOAD_TOO_BIG;
            else
                error = SMP_SERIAL_FRAME_ERROR_CORRUPTED;

            ctx->cbs.error(error, ctx->userdata);
        }

        if (frame != NULL)
            ctx->cbs.new_frame(frame, framesize, ctx->userdata);
    }

    return 0;
}

/**
 * \ingroup serial_frame
 * Wait for an event on the serial and process it.
 *
 * @param[in] ctx the SmpSerialFrameContext
 * @param[in] timeout_ms a timeout in milliseconds. A negative value means no
 *                       timeout
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_serial_frame_wait_and_process(SmpSerialFrameContext *ctx,
        int timeout_ms)
{
    int ret;

    ret = smp_serial_device_wait(&ctx->device, timeout_ms);
    if (ret == 0)
        ret = smp_serial_frame_process_recv_fd(ctx);

    return ret;
}
