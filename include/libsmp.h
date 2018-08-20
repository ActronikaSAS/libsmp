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
 */

#ifndef LIBSMP_H
#define LIBSMP_H

#include <stddef.h>
#include <stdint.h>

#ifndef __AVR
#include <sys/types.h>
#endif

#include <libsmp-config.h>
#include <libsmp-static.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Use to declare public API */
#ifdef SMP_EXPORT_API
#   ifdef _WIN32
#       define SMP_API __declspec(dllexport)
#   else
#       define SMP_API __attribute__((visibility("default")))
#   endif
#else
#   define SMP_API
#endif

#ifdef __AVR
/* we don't have sys/types.h in avr-libc so define ssize_t ourself */
typedef long ssize_t;
#elif defined(_WIN32) || defined(_WIN64)
typedef signed __int64 ssize_t;
#endif

typedef enum
{
    /** No error */
    SMP_ERROR_NONE = 0,
    /** Invalid parameter */
    SMP_ERROR_INVALID_PARAM = -1,
    /** Not enough memory */
    SMP_ERROR_NO_MEM = -2,
    /** No such device */
    SMP_ERROR_NO_DEVICE = -3,
    /** Entity not found */
    SMP_ERROR_NOT_FOUND = -4,
    /** Device busy */
    SMP_ERROR_BUSY = -5,
    /** Permission denied */
    SMP_ERROR_PERM = -6,
    /** Bad file descriptor */
    SMP_ERROR_BAD_FD = -7,
    /** Operation not supported */
    SMP_ERROR_NOT_SUPPORTED = -8,
    /** Operation would block */
    SMP_ERROR_WOULD_BLOCK = -9,
    /** IO error */
    SMP_ERROR_IO = -10,
    /** Entity already exist */
    SMP_ERROR_EXIST = -11,
    /** Too big */
    SMP_ERROR_TOO_BIG = -12,
    /** Operation timed out */
    SMP_ERROR_TIMEDOUT = -13,
    /** Overflow */
    SMP_ERROR_OVERFLOW = -14,
    /** Bad message */
    SMP_ERROR_BAD_MESSAGE = -15,
    /** Entity is not the right type */
    SMP_ERROR_BAD_TYPE = -16,

    /** Other error */
    SMP_ERROR_OTHER = -100,
} SmpError;

/* Message */

/**
 * \ingroup message
 * Type of an argument
 */
typedef enum
{
    SMP_TYPE_NONE = 0x00,
    SMP_TYPE_UINT8 = 0x01,
    SMP_TYPE_INT8 = 0x02,
    SMP_TYPE_UINT16 = 0x03,
    SMP_TYPE_INT16 = 0x04,
    SMP_TYPE_UINT32 = 0x05,
    SMP_TYPE_INT32 = 0x06,
    SMP_TYPE_UINT64 = 0x07,
    SMP_TYPE_INT64 = 0x08,
    SMP_TYPE_STRING = 0x09,
    SMP_TYPE_F32 = 0x0a,
    SMP_TYPE_F64 = 0x0b,
    SMP_TYPE_RAW = 0x10,

    SMP_TYPE_MAX = 0x7f
} SmpType;

/**
 * \ingroup message
 * Structure describing an argument in the message
 */
typedef struct
{
    SmpType type;
    union {
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        float f32;
        double f64;

        const char *cstring;

        struct {
            const uint8_t *craw;
            size_t craw_size;
        };
    } value;
} SmpValue;

/**
 * \ingroup message
 * Message structure
 */
typedef struct
{
    /** The message id */
    uint32_t msgid;

    SmpValue values[SMP_MESSAGE_MAX_VALUES];
} SmpMessage;

SMP_API void smp_message_init(SmpMessage *msg, uint32_t msgid);
SMP_API int smp_message_init_from_buffer(SmpMessage *msg, const uint8_t *buffer,
                size_t size);
SMP_API void smp_message_clear(SmpMessage *msg);

SMP_API ssize_t smp_message_encode(SmpMessage *msg, uint8_t *buffer, size_t size);
SMP_API size_t smp_message_get_encoded_size(SmpMessage *msg);

SMP_API uint32_t smp_message_get_msgid(SmpMessage *msg);
SMP_API int smp_message_n_args(SmpMessage *msg);

SMP_API int smp_message_get(SmpMessage *msg, int index, ...);
SMP_API int smp_message_get_value(SmpMessage *msg, int index, SmpValue *value);
SMP_API int smp_message_get_uint8(SmpMessage *msg, int index, uint8_t *value);
SMP_API int smp_message_get_int8(SmpMessage *msg, int index, int8_t *value);
SMP_API int smp_message_get_uint16(SmpMessage *msg, int index, uint16_t *value);
SMP_API int smp_message_get_int16(SmpMessage *msg, int index, int16_t *value);
SMP_API int smp_message_get_uint32(SmpMessage *msg, int index, uint32_t *value);
SMP_API int smp_message_get_int32(SmpMessage *msg, int index, int32_t *value);
SMP_API int smp_message_get_uint64(SmpMessage *msg, int index, uint64_t *value);
SMP_API int smp_message_get_int64(SmpMessage *msg, int index, int64_t *value);
SMP_API int smp_message_get_float(SmpMessage *msg, int index, float *value);
SMP_API int smp_message_get_double(SmpMessage *msg, int index, double *value);
SMP_API int smp_message_get_cstring(SmpMessage *msg, int index, const char **value);
SMP_API int smp_message_get_craw(SmpMessage *msg, int index, const uint8_t **raw,
                size_t *size);

SMP_API int smp_message_set(SmpMessage *msg, int index, ...);
SMP_API int smp_message_set_value(SmpMessage *msg, int index, const SmpValue *value);
SMP_API int smp_message_set_uint8(SmpMessage *msg, int index, uint8_t value);
SMP_API int smp_message_set_int8(SmpMessage *msg, int index, int8_t value);
SMP_API int smp_message_set_uint16(SmpMessage *msg, int index, uint16_t value);
SMP_API int smp_message_set_int16(SmpMessage *msg, int index, int16_t value);
SMP_API int smp_message_set_uint32(SmpMessage *msg, int index, uint32_t value);
SMP_API int smp_message_set_int32(SmpMessage *msg, int index, int32_t value);
SMP_API int smp_message_set_uint64(SmpMessage *msg, int index, uint64_t value);
SMP_API int smp_message_set_int64(SmpMessage *msg, int index, int64_t value);
SMP_API int smp_message_set_float(SmpMessage *msg, int index, float value);
SMP_API int smp_message_set_double(SmpMessage *msg, int index, double value);
SMP_API int smp_message_set_cstring(SmpMessage *msg, int index, const char *value);
SMP_API int smp_message_set_craw(SmpMessage *msg, int index, const uint8_t *raw,
                size_t size);

/* Serial frame API */

/**
 * \ingroup serial_frame
 * Error thrown during decoding
 */
typedef enum
{
    /** No error */
    SMP_SERIAL_FRAME_ERROR_NONE = 0,
    /** Payload is corrupted (bad crc, missing end byte...) */
    SMP_SERIAL_FRAME_ERROR_CORRUPTED = -1,
    /** Payload too big to fit in buffer */
    SMP_SERIAL_FRAME_ERROR_PAYLOAD_TOO_BIG = -2,
} SmpSerialFrameError;

/**
 * \ingroup serial_frame
 * Serial baudrate
 */
typedef enum
{
    SMP_SERIAL_BAUDRATE_1200,      /**< 1200 bauds */
    SMP_SERIAL_BAUDRATE_2400,      /**< 2400 bauds */
    SMP_SERIAL_BAUDRATE_4800,      /**< 4800 bauds */
    SMP_SERIAL_BAUDRATE_9600,      /**< 9600 bauds */
    SMP_SERIAL_BAUDRATE_19200,     /**< 19200 bauds */
    SMP_SERIAL_BAUDRATE_38400,     /**< 38400 bauds */
    SMP_SERIAL_BAUDRATE_57600,     /**< 57600 bauds */
    SMP_SERIAL_BAUDRATE_115200,    /**< 115200 bauds */
} SmpSerialBaudrate;

/**
 * \ingroup serial_frame
 * Parity
 */
typedef enum
{
    SMP_SERIAL_PARITY_NONE,    /**< No parity */
    SMP_SERIAL_PARITY_ODD,     /**< Odd parity */
    SMP_SERIAL_PARITY_EVEN,    /**< Even parity */
} SmpSerialParity;

/**
 * \ingroup serial_frame
 * Callback structure passed to decoder
 */
typedef struct
{
    /** called when a new valid frame is available */
    void (*new_frame)(uint8_t *frame, size_t size, void *userdata);

    /** called when an error occurs while decoding incoming data */
    void (*error)(SmpSerialFrameError error, void *userdata);
} SmpSerialFrameDecoderCallbacks;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
typedef struct
{
    HANDLE handle;
} SmpSerialDevice;
#else
typedef struct
{
    int fd;
} SmpSerialDevice;
#endif

typedef SmpSerialBaudrate SmpSerialFrameBaudrate;
typedef SmpSerialParity SmpSerialFrameParity;

#define SMP_SERIAL_FRAME_BAUDRATE_1200 SMP_SERIAL_BAUDRATE_1200
#define SMP_SERIAL_FRAME_BAUDRATE_2400 SMP_SERIAL_BAUDRATE_2400
#define SMP_SERIAL_FRAME_BAUDRATE_4800 SMP_SERIAL_BAUDRATE_4800
#define SMP_SERIAL_FRAME_BAUDRATE_9600 SMP_SERIAL_BAUDRATE_9600
#define SMP_SERIAL_FRAME_BAUDRATE_19200 SMP_SERIAL_BAUDRATE_19200
#define SMP_SERIAL_FRAME_BAUDRATE_38400 SMP_SERIAL_BAUDRATE_38400
#define SMP_SERIAL_FRAME_BAUDRATE_57600 SMP_SERIAL_BAUDRATE_57600
#define SMP_SERIAL_FRAME_BAUDRATE_115200 SMP_SERIAL_BAUDRATE_115200

#define SMP_SERIAL_FRAME_PARITY_NONE SMP_SERIAL_PARITY_NONE
#define SMP_SERIAL_FRAME_PARITY_ODD SMP_SERIAL_PARITY_ODD
#define SMP_SERIAL_FRAME_PARITY_EVEN SMP_SERIAL_PARITY_EVEN

typedef struct
{
    SmpStaticSerialProtocolDecoder decoder;
    SmpSerialDevice device;

    uint8_t frame[SMP_SERIAL_FRAME_MAX_FRAME_SIZE];
    SmpSerialFrameDecoderCallbacks cbs;
    void *userdata;
} SmpSerialFrameContext;

SMP_API int smp_serial_frame_init(SmpSerialFrameContext *ctx,
                const char *device, const SmpSerialFrameDecoderCallbacks *cbs,
                void *userdata);
SMP_API void smp_serial_frame_deinit(SmpSerialFrameContext *ctx);

SMP_API int smp_serial_frame_set_config(SmpSerialFrameContext *ctx,
                SmpSerialBaudrate baudrate, SmpSerialParity parity,
                int flow_control);

SMP_API intptr_t smp_serial_frame_get_fd(SmpSerialFrameContext *ctx);

SMP_API int smp_serial_frame_send(SmpSerialFrameContext *ctx,
                const uint8_t *buf, size_t size);
SMP_API int smp_serial_frame_process_recv_fd(SmpSerialFrameContext *ctx);

SMP_API int smp_serial_frame_wait_and_process(SmpSerialFrameContext *ctx,
                int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
