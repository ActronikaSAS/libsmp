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
 * \defgroup message Message
 *
 * Encode and decode message.
 */

#include "config.h"

#ifndef SMP_ENABLE_STATIC_API
#define SMP_ENABLE_STATIC_API
#endif

#include "libsmp.h"
#include "libsmp-private.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define MSG_HEADER_SIZE 8

SMP_STATIC_ASSERT(sizeof(SmpMessage) == sizeof(SmpStaticMessage));

#ifdef HAVE_UNALIGNED_ACCESS
#define DEFINE_READ_FUNC(type) \
static inline type##_t smp_read_##type(const uint8_t *data) \
{ \
    return *((const type##_t *) data); \
}

#define DEFINE_WRITE_FUNC(type) \
static inline void smp_write_##type(uint8_t *data, type##_t value) \
{ \
    *((type##_t *) data) = value; \
}
#else
#define DEFINE_READ_FUNC(type) \
static inline type##_t smp_read_##type(const uint8_t *data) \
{ \
    type##_t val; \
    memcpy(&val, data, sizeof(val)); \
    return val; \
}

#define DEFINE_WRITE_FUNC(type) \
static inline void smp_write_##type(uint8_t *data, type##_t value) \
{ \
    memcpy(data, &value, sizeof(value)); \
}
#endif /* HAVE_UNALIGNED_ACCESS */

DEFINE_READ_FUNC(int16);
DEFINE_READ_FUNC(uint16);
DEFINE_READ_FUNC(int32);
DEFINE_READ_FUNC(uint32);
DEFINE_READ_FUNC(int64);
DEFINE_READ_FUNC(uint64);

DEFINE_WRITE_FUNC(int16);
DEFINE_WRITE_FUNC(uint16);
DEFINE_WRITE_FUNC(int32);
DEFINE_WRITE_FUNC(uint32);
DEFINE_WRITE_FUNC(int64);
DEFINE_WRITE_FUNC(uint64);

static void smp_write_f32(uint8_t *data, float value)
{
    union {
        float f32;
        uint32_t u32;
    } v;
    v.f32 = value;

    smp_write_uint32(data, v.u32);
}

static float smp_read_f32(const uint8_t *data)
{
    union {
        float f32;
        uint32_t u32;
    } v;

    v.u32 = smp_read_uint32(data);
    return v.f32;
}

static void smp_write_f64(uint8_t *data, double value)
{
    union {
        double f64;
        uint64_t u64;
    } v;
    v.f64 = value;

    smp_write_uint64(data, v.u64);
}

static double smp_read_f64(const uint8_t *data)
{
    union {
        double f64;
        uint64_t u64;
    } v;

    v.u64 = smp_read_uint64(data);
    return v.f64;
}

/* warning: for string/raw types, it returns the mininum payload size */
static size_t smp_type_size(SmpType type)
{
    switch (type) {
        case SMP_TYPE_INT8:
        case SMP_TYPE_UINT8:
            return 1;
        case SMP_TYPE_INT16:
        case SMP_TYPE_UINT16:
            return 2;
        case SMP_TYPE_INT32:
        case SMP_TYPE_UINT32:
        case SMP_TYPE_F32:
            return 4;
        case SMP_TYPE_INT64:
        case SMP_TYPE_UINT64:
        case SMP_TYPE_F64:
            return 8;
        case SMP_TYPE_STRING:
            return 3;
        case SMP_TYPE_RAW:
            return 2;
        default:
            break;
    }

    return 0;
}

static size_t smp_value_compute_size(const SmpValue *value)
{
    size_t size;

    switch (value->type) {
        case SMP_TYPE_STRING:
            /* size + nul byte */
            size = 3;

            if (value->value.cstring != NULL)
                size += strlen(value->value.cstring);

            break;
        case SMP_TYPE_RAW:
            /* raw data size */
            size = 2;

            if (value->value.craw != NULL)
                size += value->value.craw_size;

            break;
        default:
            /* scalar types have known size */
            size = smp_type_size(value->type);
            break;
    }

    return size;
}

static const char *smp_message_decode_string(const uint8_t *buffer, size_t size)
{
    size_t strsize;

    if (size < 2)
        return NULL;

    strsize = smp_read_uint16(buffer);

    if (size < 2 + strsize)
        return NULL;

    /* make sure there is a nul byte at the end, note that strsize includes
     * the nul byte */
    if (buffer[2 + strsize - 1] != '\0')
        return NULL;

    return ((const char *) buffer) + 2;
}

static int smp_message_decode_raw(SmpValue *value, const uint8_t *buffer,
        size_t size)
{
    size_t rawsize;

    if (size < 2)
        return SMP_ERROR_BAD_MESSAGE;

    rawsize = smp_read_uint16(buffer);

    if (size < 2 + rawsize)
        return SMP_ERROR_BAD_MESSAGE;

    value->value.craw = buffer + 2;
    value->value.craw_size = rawsize;

    return 0;
}

static ssize_t smp_message_decode_value(SmpValue *value, const uint8_t *buffer,
        size_t size)
{
    size_t argsize;

    /* we need at least 2 bytes */
    if (size < 2)
        return SMP_ERROR_BAD_MESSAGE;

    value->type = buffer[0];
    buffer++;

    argsize = 1 + smp_type_size(value->type);

    if (size < argsize)
        return SMP_ERROR_BAD_MESSAGE;

    switch (value->type) {
        case SMP_TYPE_UINT8:
            value->value.u8 = *buffer;
            break;
        case SMP_TYPE_INT8:
            value->value.i8 = *buffer;
            break;
        case SMP_TYPE_UINT16:
            value->value.u16 = smp_read_uint16(buffer);
            break;
        case SMP_TYPE_INT16:
            value->value.i16 = smp_read_int16(buffer);
            break;
        case SMP_TYPE_UINT32:
            value->value.u32 = smp_read_uint32(buffer);
            break;
        case SMP_TYPE_INT32:
            value->value.i32 = smp_read_int32(buffer);
            break;
        case SMP_TYPE_UINT64:
            value->value.u64 = smp_read_uint64(buffer);
            break;
        case SMP_TYPE_INT64:
            value->value.i64 = smp_read_int64(buffer);
            break;
        case SMP_TYPE_STRING:
            value->value.cstring = smp_message_decode_string(buffer, size - 1);

            /* recalculate argsize with string size */
            argsize = 1 + smp_value_compute_size(value);
            break;
        case SMP_TYPE_RAW: {
            int ret = smp_message_decode_raw(value, buffer, size - 1);
            if (ret < 0)
                break;

            argsize = 1 + smp_value_compute_size(value);
            break;
        }
        case SMP_TYPE_F32:
            value->value.f32 = smp_read_f32(buffer);
            break;
        case SMP_TYPE_F64:
            value->value.f64 = smp_read_f64(buffer);
            break;
        default:
            return SMP_ERROR_BAD_MESSAGE;
    }

    return argsize;
}


static ssize_t smp_message_encode_value(const SmpValue *value, uint8_t *buffer)
{
    buffer[0] = value->type;
    buffer++;

    switch (value->type) {
            case SMP_TYPE_UINT8:
                *buffer = value->value.u8;
                break;
            case SMP_TYPE_INT8:
                *buffer = (uint8_t) value->value.i8;
                break;
            case SMP_TYPE_UINT16:
                smp_write_uint16(buffer, value->value.u16);
                break;
            case SMP_TYPE_INT16:
                smp_write_int16(buffer, value->value.i16);
                break;
            case SMP_TYPE_UINT32:
                smp_write_uint32(buffer, value->value.u32);
                break;
            case SMP_TYPE_INT32:
                smp_write_int32(buffer, value->value.i32);
                break;
            case SMP_TYPE_UINT64:
                smp_write_uint64(buffer, value->value.u64);
                break;
            case SMP_TYPE_INT64:
                smp_write_int64(buffer, value->value.i64);
                break;
            case SMP_TYPE_STRING: {
                size_t len = strlen(value->value.cstring);
                if (len > (UINT16_MAX - 1)) {
                    /* string too long */
                    return 0;
                }

                /* size | data | nul byte */
                smp_write_uint16(buffer, (uint16_t)(len + 1));
                memcpy(buffer + 2, value->value.cstring, len);
                buffer[2 + len] = '\0';
                break;
            }
            case SMP_TYPE_RAW: {
                if (value->value.craw_size > UINT16_MAX)
                    return 0;

                /* size | data */
                smp_write_uint16(buffer, (uint16_t) value->value.craw_size);
                memcpy(buffer + 2, value->value.craw, value->value.craw_size);
                break;
            }
            case SMP_TYPE_F32:
                smp_write_f32(buffer, value->value.f32);
                break;
            case SMP_TYPE_F64:
                smp_write_f64(buffer, value->value.f64);
                break;
            default:
                return 0;
    }

    return smp_value_compute_size(value) + 1;
}

/* return the maximum size of the encoded payload (w/o smp header) */
static size_t smp_message_compute_max_encoded_size(SmpMessage *msg)
{
    size_t ret = 0;
    size_t i;

    for (i = 0; i < msg->capacity; i++) {
        if (msg->pvalues[i].type != SMP_TYPE_NONE)
            ret += (1 + smp_value_compute_size(&msg->pvalues[i]));
    }

    return ret;
}

/* Internal API */
int smp_message_build_from_buffer(SmpMessage *msg, const uint8_t *buffer,
        size_t size)
{
    size_t argsize;
    size_t offset;
    size_t i;

    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(buffer != NULL, SMP_ERROR_INVALID_PARAM);

    if (size < MSG_HEADER_SIZE)
        return SMP_ERROR_BAD_MESSAGE;

    /* parse header */
    msg->msgid = smp_read_uint32(buffer);
    argsize = smp_read_uint32(buffer + 4);

    if (size < MSG_HEADER_SIZE + argsize)
        return SMP_ERROR_BAD_MESSAGE;

    offset = MSG_HEADER_SIZE;
    for (i = 0; size - offset > 0 && i < msg->capacity; i++) {
        ssize_t ret;

        ret = smp_message_decode_value(&msg->pvalues[i], buffer + offset,
                size - offset);
        if (ret < 0)
            return (int) ret;

        offset += ret;
    }

    if (size - offset > 0) {
        /* we reached the maximum number of values */
        return SMP_ERROR_TOO_BIG;
    }

    return 0;
}

/* API */

/**
 * \ingroup message
 * Create a new SmpMessage.
 *
 * @return a new SmpMessage or NULL on error.
 */
SmpMessage *smp_message_new(void)
{
    return smp_message_new_with_id(0);
}

/**
 * \ingroup message
 * Create a new SmpMessage with given id.
 *
 * @param[in] id the ID of the message
 *
 * @return a new SmpMessage or NULL on error.
 */
SmpMessage *smp_message_new_with_id(uint32_t id)
{
    SmpMessage *msg;

    msg = smp_new(SmpMessage);
    if (msg == NULL)
        return NULL;

    smp_message_init(msg, id);
    msg->statically_allocated = false;

    return msg;
}

/**
 * \ingroup message
 * Create a new SmpMessage from a static memory space.
 * @warning: for now DON'T use smp_message_init() or
 * smp_message_init_from_buffer() on a buffer created using this func.
 *
 * @param[in] smsg a SmpStaticMessage
 * @param[in] struct_size the size of smsg
 * @param[in] values pointer to an array of values to use
 * @param[in] capacity capacity of the provided values array
 *
 * @return a new SmpMessage or NULL on error.
 */
SmpMessage *smp_message_new_from_static(SmpStaticMessage *smsg,
        size_t struct_size, SmpValue *values, size_t capacity)
{
    return smp_message_new_from_static_with_id(smsg, struct_size, values,
            capacity, 0);
}

/**
 * \ingroup message
 * Create a new SmpMessage from a static memory space
 * @warning: for now DON'T use smp_message_init() or
 * smp_message_init_from_buffer() on a buffer created using this func.
 *
 * @param[in] smsg a SmpStaticMessage
 * @param[in] struct_size the size of smsg
 * @param[in] values pointer to an array of values to use
 * @param[in] capacity capacity of the provided values array
 * @param[in] id the ID of the message
 *
 * @return a new SmpMessage or NULL on error.
 */
SmpMessage *smp_message_new_from_static_with_id(SmpStaticMessage *smsg,
        size_t struct_size, SmpValue *values, size_t capacity, uint32_t id)
{
    SmpMessage *msg = (SmpMessage *) smsg;

    return_val_if_fail(smsg != NULL, NULL);
    return_val_if_fail(struct_size >= sizeof(SmpMessage), NULL);
    return_val_if_fail(values != NULL, NULL);
    return_val_if_fail(capacity > 0, NULL);

    smp_message_init(msg, id);
    msg->pvalues = values;
    msg->capacity = capacity;

    memset(msg->pvalues, 0, capacity * sizeof(SmpValue));
    msg->statically_allocated = true;

    return msg;
}

/**
 * \ingroup message
 * Free a previously allocated SmpMessage.
 *
 * @param[in] msg a SmpMessage
 */
void smp_message_free(SmpMessage *msg)
{
    return_if_fail(msg != NULL);

    if (msg->statically_allocated)
        return;

    free(msg);
}

/**
 * \ingroup message
 * Initialze a SmpMessage
 *
 * @param[in] msg a SmpMessage
 * @param[in] msgid the ID of the message
 */
void smp_message_init(SmpMessage *msg, uint32_t msgid)
{
    return_if_fail(msg != NULL);

    memset(msg, 0, sizeof(*msg));

    msg->msgid = msgid;
    msg->pvalues = msg->values;
    msg->capacity = SMP_MESSAGE_MAX_VALUES;
}

/**
 * \ingroup message
 * Initialize a SmpMessage from the given buffer. Buffer will be parsed to
 * populate the message.
 * Warning: if message contains strings or raw data, the buffer shall not be
 * overwritten or freed as long as message exist as strings only points to
 * buffer.
 *
 * @param[in] msg a SmpMessage
 * @param[in] buffer the buffer to parse
 * @param[in] size of the buffer
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_init_from_buffer(SmpMessage *msg, const uint8_t *buffer,
        size_t size)
{
    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(buffer != NULL, SMP_ERROR_INVALID_PARAM);

    smp_message_init(msg, 0);
    return smp_message_build_from_buffer(msg, buffer, size);
}

/**
 * \ingroup message
 * Clear a SmpMessage.
 *
 * @param[in] msg a SmpMessage
 */
void smp_message_clear(SmpMessage *msg)
{
    return_if_fail(msg != NULL);

    msg->msgid = 0;
    memset(msg->pvalues, 0, msg->capacity * sizeof(SmpValue));
}

/**
 * \ingroup message
 * Encode a SmpMessage in the provided buffer.
 *
 * @param[in] msg a SmpMessage
 * @param[in] buffer the destination buffer
 * @param[in] size size of the buffer
 *
 * @return the number of bytes written in the buffer or a SmpError otherwise.
 */
ssize_t smp_message_encode(SmpMessage *msg, uint8_t *buffer, size_t size)
{
    size_t offset = 0;
    size_t i;

    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(buffer != NULL, SMP_ERROR_INVALID_PARAM);

    if (size < smp_message_get_encoded_size(msg))
        return SMP_ERROR_NO_MEM;

    /* write header */
    smp_write_uint32(buffer, msg->msgid);
    offset += MSG_HEADER_SIZE;

    for (i = 0; i < msg->capacity; i++) {
        const SmpValue *val = &msg->pvalues[i];

        if (val->type == SMP_TYPE_NONE)
            continue;

        offset += smp_message_encode_value(val, buffer + offset);
    }

    if (offset - MSG_HEADER_SIZE > UINT32_MAX)
        return SMP_ERROR_OVERFLOW;

    /* message size is current buffer offset - header size */
    smp_write_uint32(buffer + 4, (uint32_t) (offset - MSG_HEADER_SIZE));

    return offset;
}

size_t smp_message_get_encoded_size(SmpMessage *message)
{
    return smp_message_compute_max_encoded_size(message) + MSG_HEADER_SIZE;
}

/**
 * \ingroup message
 * Get the message ID of a SmpMessage.
 *
 * @param[in] msg a SmpMessage
 *
 * @return the ID of the message.
 */
uint32_t smp_message_get_msgid(SmpMessage *msg)
{
    return_val_if_fail(msg != NULL, 0);

    return msg->msgid;
}

/**
 * \ingroup message
 * Set the ID of the message without resetting the values
 *
 * @param[in] msg a SmpMessage
 * @param[in] id the id to set
 */
void smp_message_set_id(SmpMessage *msg, uint32_t id)
{
    return_if_fail(msg != NULL);

    msg->msgid = id;
}

/**
 * \ingroup message
 * Get the number of valid arguments in a message.
 *
 * @param[in] msg a SmpMessage
 *
 * @return The number of arguments.
 */
int smp_message_n_args(SmpMessage *msg)
{
    size_t i;

    return_val_if_fail(msg != NULL, -1);

    for (i = 0; i < msg->capacity; i++) {
        if (msg->pvalues[i].type == SMP_TYPE_NONE)
            break;
    }

    if (i > INT_MAX)
        return SMP_ERROR_OVERFLOW;

    return (int) i;
}

/**
 * \ingroup message
 * Get arguments from the message. Variable arguments should be the index
 * of the message argument, type of the argument as a SmpType and pointer to
 * a location to store the return value. The last variable should be set to -1.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index the index of the first argument to get
 * @param[in] ... variable argument
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get(SmpMessage *msg, int index, ...)
{
    va_list ap;
    int ret;

    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(index >= 0, SMP_ERROR_INVALID_PARAM);

    va_start(ap, index);

    do {
        SmpType type;
        SmpValue val;

        ret = smp_message_get_value(msg, index, &val);
        if (ret < 0)
            goto done;

        type = va_arg(ap, int);
        if (type != val.type) {
            ret = SMP_ERROR_BAD_TYPE;
            goto done;
        }

        switch (type) {
            case SMP_TYPE_UINT8: {
                uint8_t *ptr = va_arg(ap, uint8_t *);

                *ptr = val.value.u8;
                break;
            }
            case SMP_TYPE_INT8: {
                int8_t *ptr = va_arg(ap, int8_t *);

                *ptr = val.value.i8;
                break;
            }
            case SMP_TYPE_UINT16: {
                uint16_t *ptr = va_arg(ap, uint16_t *);

                *ptr = val.value.u16;
                break;
            }

            case SMP_TYPE_INT16: {
                int16_t *ptr = va_arg(ap, int16_t *);

                *ptr = val.value.i16;
                break;
            }
            case SMP_TYPE_UINT32: {
                uint32_t *ptr = va_arg(ap, uint32_t *);

                *ptr = val.value.u32;
                break;
            }

            case SMP_TYPE_INT32: {
                int32_t *ptr = va_arg(ap, int32_t *);

                *ptr = val.value.i32;
                break;
            }
            case SMP_TYPE_UINT64: {
                uint64_t *ptr = va_arg(ap, uint64_t *);

                *ptr = val.value.u64;
                break;
            }

            case SMP_TYPE_INT64: {
                int64_t *ptr = va_arg(ap, int64_t *);

                *ptr = val.value.i64;
                break;
            }

            case SMP_TYPE_STRING: {
                const char **ptr = va_arg(ap, const char **);

                *ptr = val.value.cstring;
                break;
            }

            case SMP_TYPE_RAW: {
                const uint8_t **ptr = va_arg(ap, const uint8_t **);
                size_t *psize = va_arg(ap, size_t *);

                *ptr = val.value.craw;
                *psize = val.value.craw_size;
                break;
            }

            case SMP_TYPE_F32: {
                float *ptr = va_arg(ap, float *);

                *ptr = val.value.f32;
                break;
            }

            case SMP_TYPE_F64: {
                double *ptr = va_arg(ap, double *);

                *ptr = val.value.f64;
                break;
            }

            default:
                ret = SMP_ERROR_BAD_TYPE;
                goto done;
        }

        index = va_arg(ap, int);
    } while (index >= 0);

    ret = 0;

done:
    va_end(ap);
    return ret;
}

/**
 * \ingroup message
 * Get the message value pointed by index
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to a SmpValue to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_value(SmpMessage *msg, int index, SmpValue *value)
{
    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(index >= 0, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(value != NULL, SMP_ERROR_INVALID_PARAM);

    if ((size_t) index >= msg->capacity)
        return SMP_ERROR_NOT_FOUND;

    if (msg->pvalues[index].type == SMP_TYPE_NONE)
        return SMP_ERROR_NOT_FOUND;

    *value = msg->pvalues[index];
    return 0;
}

/**
 * \ingroup message
 * Get the message uint8 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an uint8 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_uint8(SmpMessage *msg, int index, uint8_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT8, value, -1);
}

/**
 * \ingroup message
 * Get the message int8 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an int8 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_int8(SmpMessage *msg, int index, int8_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT8, value, -1);
}

/**
 * \ingroup message
 * Get the message uint16 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an uint16 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_uint16(SmpMessage *msg, int index, uint16_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT16, value, -1);
}

/**
 * \ingroup message
 * Get the message int16 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an int16 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_int16(SmpMessage *msg, int index, int16_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT16, value, -1);
}

/**
 * \ingroup message
 * Get the message uint32 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an uint32 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_uint32(SmpMessage *msg, int index, uint32_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT32, value, -1);
}

/**
 * \ingroup message
 * Get the message int32 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an int32 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_int32(SmpMessage *msg, int index, int32_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT32, value, -1);
}

/**
 * \ingroup message
 * Get the message uint64 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an uint64 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_uint64(SmpMessage *msg, int index, uint64_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT64, value, -1);
}

/**
 * \ingroup message
 * Get the message int64 value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to an int64 to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_int64(SmpMessage *msg, int index, int64_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT64, value, -1);
}

/**
 * \ingroup message
 * Get the message float value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to a float to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_float(SmpMessage *msg, int index, float *value)
{
    return smp_message_get(msg, index, SMP_TYPE_F32, value, -1);
}

/**
 * \ingroup message
 * Get the message double value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to a double to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_double(SmpMessage *msg, int index, double *value)
{
    return smp_message_get(msg, index, SMP_TYPE_F64, value, -1);
}

/**
 * \ingroup message
 * Get the message string value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 * The returned value is valid as long as the message exist.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to a string to hold the result
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_cstring(SmpMessage *msg, int index, const char **value)
{
    return smp_message_get(msg, index, SMP_TYPE_STRING, value, -1);
}

/**
 * \ingroup message
 * Get the message raw buffer value pointed by index and store it into value.
 * Caller is responsible to ensure that the value at index has the correct
 * type.
 * The returned value is valid as long as the message exist.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] raw a pointer to hold the result
 * @param[in] size the size of the raw data
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_get_craw(SmpMessage *msg, int index, const uint8_t **raw,
        size_t *size)
{
    return smp_message_get(msg, index, SMP_TYPE_RAW, raw, size, -1);
}

/**
 * \ingroup message
 * Set arguments in the message. Variable arguments should be the index
 * of the message argument, type of the argument as a SmpType and pointer to
 * a location to store the return value. The last variable should be set to -1.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index the index of the first argument to set
 * @param[in] ... variable argument
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set(SmpMessage *msg, int index, ...)
{
    va_list ap;
    int ret;

    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(index >= 0, SMP_ERROR_INVALID_PARAM);

    va_start(ap, index);

    do {
        SmpValue val;

        val.type = va_arg(ap, int);

        switch (val.type) {
            case SMP_TYPE_UINT8:
                val.value.u8 = (uint8_t) va_arg(ap, int);
                break;
            case SMP_TYPE_INT8:
                val.value.i8 = (int8_t) va_arg(ap, int);
                break;
            case SMP_TYPE_UINT16:
                val.value.u16 = (uint16_t) va_arg(ap, int);
                break;
            case SMP_TYPE_INT16:
                val.value.i16 = (int16_t) va_arg(ap, int);
                break;
            case SMP_TYPE_UINT32:
                val.value.u32 = va_arg(ap, uint32_t);
                break;
            case SMP_TYPE_INT32:
                val.value.i32 = va_arg(ap, int32_t);
                break;
            case SMP_TYPE_UINT64:
                val.value.u64 = va_arg(ap, uint64_t);
                break;
            case SMP_TYPE_INT64:
                val.value.i64 = va_arg(ap, int64_t);
                break;
            case SMP_TYPE_STRING:
                val.value.cstring = va_arg(ap, const char *);
                break;
            case SMP_TYPE_RAW:
                val.value.craw = va_arg(ap, const uint8_t *);
                val.value.craw_size = va_arg(ap, size_t);
                break;
            case SMP_TYPE_F32:
                val.value.f32 = (float) va_arg(ap, double);
                break;
            case SMP_TYPE_F64:
                val.value.f64 = va_arg(ap, double);
                break;
            default:
                break;
        }

        ret = smp_message_set_value(msg, index, &val);
        if (ret < 0)
            goto done;

        index = va_arg(ap, int);
    } while (index >= 0);

    ret = 0;

done:
    va_end(ap);
    return ret;
}

/**
 * \ingroup message
 * Set the message value pointed by index
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value a pointer to a SmpValue to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_value(SmpMessage *msg, int index, const SmpValue *value)
{
    return_val_if_fail(msg != NULL, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(index >= 0, SMP_ERROR_INVALID_PARAM);
    return_val_if_fail(value != NULL, SMP_ERROR_INVALID_PARAM);

    if ((size_t) index >= msg->capacity)
        return SMP_ERROR_NOT_FOUND;

    msg->pvalues[index] = *value;
    return 0;
}

/**
 * \ingroup message
 * Set the message value pointed by index to given uint8 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the uint8 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_uint8(SmpMessage *msg, int index, uint8_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT8, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given int8 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the int8 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_int8(SmpMessage *msg, int index, int8_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT8, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given uint16 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the uint16 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_uint16(SmpMessage *msg, int index, uint16_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT16, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given int16 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the int16 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_int16(SmpMessage *msg, int index, int16_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT16, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given uint32 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the uint32 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_uint32(SmpMessage *msg, int index, uint32_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT32, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given int32 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the int32 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_int32(SmpMessage *msg, int index, int32_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT32, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given uint64 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the uint64 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_uint64(SmpMessage *msg, int index, uint64_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT64, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given int64 value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the int64 to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_int64(SmpMessage *msg, int index, int64_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT64, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given float value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the float to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_float(SmpMessage *msg, int index, float value)
{
    return smp_message_set(msg, index, SMP_TYPE_F32, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given double value.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the double to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_double(SmpMessage *msg, int index, double value)
{
    return smp_message_set(msg, index, SMP_TYPE_F64, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given string.
 * Warning: the string is not copied so it shall exist as long as message exist.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] value the string to set
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_cstring(SmpMessage *msg, int index, const char *value)
{
    return smp_message_set(msg, index, SMP_TYPE_STRING, value, -1);
}

/**
 * \ingroup message
 * Set the message value pointed by index to given raw data.
 * Warning: data is not copied so it shall exist as long as message exist.
 *
 * @param[in] msg a SmpMessage
 * @param[in] index index of the value
 * @param[in] raw the raw data to set
 * @param[in] size the raw data size
 *
 * @return 0 on success, a SmpError otherwise.
 */
int smp_message_set_craw(SmpMessage *msg, int index, const uint8_t *raw,
        size_t size)
{
    return smp_message_set(msg, index, SMP_TYPE_RAW, raw, size, -1);
}
