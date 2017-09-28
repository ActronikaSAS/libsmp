/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef LIBSMP_H
#define LIBSMP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Serial frame API */

#define SMP_SERIAL_FRAME_MAX_FRAME_SIZE 1024

typedef enum
{
    SMP_SERIAL_FRAME_ERROR_NONE = 0,
    SMP_SERIAL_FRAME_ERROR_CORRUPTED = -1,
    SMP_SERIAL_FRAME_ERROR_PAYLOAD_TOO_BIG = -2,
} SmpSerialFrameError;

typedef enum
{
    SMP_SERIAL_FRAME_DECODER_STATE_WAIT_HEADER,
    SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME,
    SMP_SERIAL_FRAME_DECODER_STATE_IN_FRAME_ESC,
} SmpSerialFrameDecoderState;

typedef struct
{
    void (*new_frame)(uint8_t *frame, size_t size, void *userdata);
    void (*error)(SmpSerialFrameError error, void *userdata);
} SmpSerialFrameDecoderCallbacks;

typedef struct
{
    SmpSerialFrameDecoderState state;
    SmpSerialFrameDecoderCallbacks cbs;

    uint8_t frame[SMP_SERIAL_FRAME_MAX_FRAME_SIZE];
    size_t frame_offset;

    void *userdata;
} SmpSerialFrameDecoder;

typedef struct
{
    SmpSerialFrameDecoder decoder;

    int serial_fd;
} SmpSerialFrameContext;

int smp_serial_frame_init(SmpSerialFrameContext *ctx, const char *device,
        const SmpSerialFrameDecoderCallbacks *cbs, void *userdata);
void smp_serial_frame_deinit(SmpSerialFrameContext *ctx);

int smp_serial_frame_send(SmpSerialFrameContext *ctx, const uint8_t *buf,
        size_t size);
int smp_serial_frame_process_recv_fd(SmpSerialFrameContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
