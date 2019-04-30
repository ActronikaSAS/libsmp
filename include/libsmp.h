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
#include <stdbool.h>

#ifndef __AVR
#include <sys/types.h>
#endif

#include <libsmp-config.h>

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

/* deprecation attribute */
#ifndef SMP_DISABLE_DEPRECATED
#ifdef _WIN32
#define SMP_DEPRECATED __declspec(deprecated)
#else
#define SMP_DEPRECATED __attribute__ ((deprecated))
#endif

#ifdef _WIN32
#define SMP_DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define SMP_DEPRECATED_FOR(f) __attribute__((__deprecated__("Use '" #f "' instead")))
#endif

#else /* SMP_DISABLE_DEPRECATED */
#define SMP_DEPRECATED
#define SMP_DEPRECATED_FOR(f)
#endif /* SMP_DISABLE_DEPRECATED */

/* ssize_t definition */
#ifdef __AVR
/* we don't have sys/types.h in avr-libc so define ssize_t ourself */
typedef long ssize_t;
#elif defined(_WIN32) || defined(_WIN64)
typedef signed __int64 ssize_t;
#endif

/**
 * \defgroup error Error
 * Error enumeration
 */
/**
 * \ingroup error
 */
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
    /** Pipe error: device disconnected */
    SMP_ERROR_PIPE = -17,

    /** Other error */
    SMP_ERROR_OTHER = -100,
} SmpError;

const char *smp_error_to_string(SmpError error);

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
 *
 * @warning using this structure directly is deprecated, use smp_message_new or
 * smp_message_new_from_static to get a SmpMessage object
 */
typedef struct SmpMessage SmpMessage;
struct SmpMessage
{
    /** The message id */
    uint32_t msgid;

    SmpValue values[SMP_MESSAGE_MAX_VALUES];
    SmpValue *pvalues;
    size_t capacity;

    bool statically_allocated;
};

SMP_API SmpMessage *smp_message_new(void);
SMP_API SmpMessage *smp_message_new_with_id(uint32_t id);
SMP_API void smp_message_free(SmpMessage *msg);

SMP_API void smp_message_init(SmpMessage *msg, uint32_t msgid);
SMP_API int smp_message_init_from_buffer(SmpMessage *msg, const uint8_t *buffer,
                size_t size);
SMP_API void smp_message_clear(SmpMessage *msg);

SMP_API ssize_t smp_message_encode(SmpMessage *msg, uint8_t *buffer, size_t size);
SMP_API size_t smp_message_get_encoded_size(SmpMessage *msg);

SMP_API uint32_t smp_message_get_msgid(SmpMessage *msg);
SMP_API void smp_message_set_id(SmpMessage *msg, uint32_t id);
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

/* Serial API */

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

/* Context API */
typedef struct SmpContext SmpContext;

typedef struct
{
    void (*new_message_cb)(SmpContext *ctx, SmpMessage *msg, void *userdata);
    void (*error_cb)(SmpContext *ctx, SmpError error, void *userdata);
} SmpEventCallbacks;

SMP_API SmpContext *smp_context_new(const SmpEventCallbacks *cbs, void *userdata);
SMP_API void smp_context_free(SmpContext *ctx);

SMP_API int smp_context_open(SmpContext *ctx, const char *device);
SMP_API void smp_context_close(SmpContext *ctx);
SMP_API int smp_context_set_serial_config(SmpContext *ctx,
                SmpSerialBaudrate baudrate, SmpSerialParity parity,
                int flow_control);
SMP_API intptr_t smp_context_get_fd(SmpContext *ctx);
SMP_API int smp_context_send_message(SmpContext *ctx, SmpMessage *msg);
SMP_API int smp_context_process_fd(SmpContext *ctx);
SMP_API int smp_context_wait_and_process(SmpContext *ctx, int timeout_ms);

SMP_API int smp_context_set_decoder_maximum_capacity(SmpContext *ctx, size_t max);

/* Buffer API */
typedef struct SmpBuffer SmpBuffer;

/**
 * \ingroup buffer
 * Callback used to free data contained in buffer.
 *
 * @param[in] buffer the SmpBuffer
 */
typedef void (*SmpBufferFreeFunc)(SmpBuffer *buffer);

typedef struct SmpSerialProtocolDecoder SmpSerialProtocolDecoder;

/* Static API */
#ifdef SMP_ENABLE_STATIC_API
#include <libsmp-static.h>

/**
 * \ingroup context
 * Helper macro to completely define a SmpContext using static storage with
 * provided buffer size.
 * It defines a function which should be called in your code. The function
 * name is the concatenation of provided 'name' and '_create' and return
 * a SmpContext*. The function prototype is:
 * `SmpContext *name_create(const SmpEventCallbacks *cbs, void *userdata)`
 * See smp_context_new() for parameters description.
 *
 * @param[in] name the name of the context
 * @param[in] serial_rx_bufsize the size of the serial rx buffer
 * @param[in] serial_tx_bufsize the size of the serial tx buffer
 * @param[in] msg_tx_bufsize the size of the message buffer for tx
 * @param[in] msg_rx_values_size the maximum number of values in the rx message
 */
#define SMP_DEFINE_STATIC_CONTEXT(name, serial_rx_bufsize, serial_tx_bufsize, \
        msg_tx_bufsize, msg_rx_values_size)            \
static SmpContext* name##_create(const SmpEventCallbacks *cbs, void *userdata)\
{                                                                             \
    static struct {                                                           \
        SmpStaticContext ctx;                                                 \
        SmpStaticSerialProtocolDecoder decoder;                               \
        SmpStaticBuffer serial_tx;                                            \
        SmpStaticBuffer msg_tx;                                               \
        SmpStaticMessage msg_rx;                                              \
        uint8_t serial_rx_buf[serial_rx_bufsize];                             \
        uint8_t serial_tx_buf[serial_tx_bufsize];                             \
        uint8_t msg_tx_buf[msg_tx_bufsize];                                   \
        SmpValue msg_rx_values[msg_rx_values_size];                           \
    } sctx;                                                                   \
    SmpContext *ctx;                                                          \
    SmpSerialProtocolDecoder *decoder;                                        \
    SmpBuffer *serial_tx;                                                     \
    SmpBuffer *msg_tx;                                                        \
    SmpMessage *msg_rx;                                                       \
                                                                              \
    serial_tx = smp_buffer_new_from_static(&sctx.serial_tx,                   \
            sizeof(sctx.serial_tx), sctx.serial_tx_buf,                       \
            sizeof(sctx.serial_tx_buf), NULL);                                \
    msg_tx = smp_buffer_new_from_static(&sctx.msg_tx, sizeof(sctx.msg_tx),    \
            sctx.msg_tx_buf, sizeof(sctx.msg_tx_buf), NULL);                  \
    msg_rx = smp_message_new_from_static(&sctx.msg_rx, sizeof(sctx.msg_rx),   \
            sctx.msg_rx_values, msg_rx_values_size);                          \
    decoder = smp_serial_protocol_decoder_new_from_static(&sctx.decoder,      \
            sizeof(sctx.decoder), sctx.serial_rx_buf,                         \
            sizeof(sctx.serial_rx_buf));                                      \
    ctx = smp_context_new_from_static(&sctx.ctx, sizeof(sctx.ctx), cbs,       \
        userdata, decoder, serial_tx, msg_tx, msg_rx);                        \
    return ctx;                                                               \
}

/**
 * \ingroup message
 * Helper macro to define a SmpMessage using static storage.
 * It defines a function to be called in your code by concatenating `name` and
 * `_create()` and return a pointer to the SmpMessage.
 * The created function takes the msg id as the first parameter.
 *
 * @param[in] name the name to use
 * @param[in] max_values the maximum number of values in the message
 */
#define SMP_DEFINE_STATIC_MESSAGE(name, max_values)                           \
static SmpMessage* name##_create(int id)                                      \
{                                                                             \
    static SmpStaticMessage smsg;                                             \
    static SmpValue values[max_values];                                       \
                                                                              \
    return smp_message_new_from_static_with_id(&smsg, sizeof(smsg), values,   \
            max_values, id);                                                  \
}

SMP_API SmpBuffer *smp_buffer_new_from_static(SmpStaticBuffer *sbuffer,
                size_t struct_size, uint8_t *data, size_t maxsize,
                SmpBufferFreeFunc free_func);

SMP_API SmpSerialProtocolDecoder *
smp_serial_protocol_decoder_new_from_static(SmpStaticSerialProtocolDecoder *sdec,
                size_t struct_size, uint8_t *buf, size_t bufsize);

SMP_API SmpContext *smp_context_new_from_static(SmpStaticContext *sctx,
                size_t struct_size, const SmpEventCallbacks *cbs,
                void *userdata, SmpSerialProtocolDecoder *decoder,
                SmpBuffer *serial_tx, SmpBuffer *msg_tx, SmpMessage *msg_rx);

SMP_API SmpMessage *smp_message_new_from_static(SmpStaticMessage *smsg,
                size_t struct_size, SmpValue *values, size_t capacity);
SMP_API SmpMessage *smp_message_new_from_static_with_id(SmpStaticMessage *smsg,
                size_t struct_size, SmpValue *values, size_t capacity,
                uint32_t id);
#endif

#ifdef __cplusplus
}
#endif

#endif
