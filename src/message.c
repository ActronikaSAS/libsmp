/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#include "libsmp.h"
#include "libsmp-private.h"
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define MSG_HEADER_SIZE 8

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
            return 4;
        case SMP_TYPE_INT64:
        case SMP_TYPE_UINT64:
            return 8;
        default:
            break;
    }

    return 0;
}

static ssize_t smp_message_decode_value(SmpValue *value, const uint8_t *buffer,
        size_t size)
{
    size_t argsize;

    /* we need at least 2 bytes */
    if (size < 2)
        return -EBADMSG;

    value->type = buffer[0];
    buffer++;

    argsize = 1 + smp_type_size(value->type);

    if (size < argsize)
        return -EBADMSG;

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
        default:
            return -EBADMSG;
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
            default:
                return 0;
    }

    return smp_type_size(value->type) + 1;
}

static size_t smp_message_compute_max_encoded_size(SmpMessage *msg)
{
    size_t ret = 0;
    int i;

    for (i = 0; i < SMP_MESSAGE_MAX_VALUES; i++)
        ret += smp_type_size(msg->values[i].type);

    return ret;
}

/* API */

void smp_message_init(SmpMessage *msg, uint32_t msgid)
{
    return_if_fail(msg != NULL);

    memset(msg, 0, sizeof(*msg));

    msg->msgid = msgid;
}

int smp_message_init_from_buffer(SmpMessage *msg, const uint8_t *buffer,
        size_t size)
{
    size_t argsize;
    size_t offset;
    int i;

    return_val_if_fail(msg != NULL, -EINVAL);
    return_val_if_fail(buffer != NULL, -EINVAL);

    if (size < MSG_HEADER_SIZE)
        return -EBADMSG;

    /* parse header */
    smp_message_init(msg, smp_read_uint32(buffer));
    argsize = smp_read_uint32(buffer + 4);

    if (size < MSG_HEADER_SIZE + argsize)
        return -EBADMSG;

    offset = 8;
    for (i = 0; size - offset > 0 && i < SMP_MESSAGE_MAX_VALUES; i++) {
        int ret;

        ret = smp_message_decode_value(&msg->values[i], buffer + offset,
                size - offset);
        if (ret < 0)
            return ret;

        offset += ret;
    }

    if (size - offset > 0) {
        /* we reached the maximum number of values */
        return -E2BIG;
    }

    return 0;
}

void smp_message_clear(SmpMessage *msg)
{
    return_if_fail(msg != NULL);
}

ssize_t smp_message_encode(SmpMessage *msg, uint8_t *buffer, size_t size)
{
    size_t offset = 0;
    int i;

    return_val_if_fail(msg != NULL, -EINVAL);
    return_val_if_fail(buffer != NULL, -EINVAL);

    if (size < smp_message_compute_max_encoded_size(msg) + MSG_HEADER_SIZE)
        return -ENOMEM;

    /* write header */
    smp_write_uint32(buffer, msg->msgid);
    offset += 8;

    for (i = 0; i < SMP_MESSAGE_MAX_VALUES; i++) {
        const SmpValue *val = &msg->values[i];

        if (val->type == SMP_TYPE_NONE)
            continue;

        offset += smp_message_encode_value(val, buffer + offset);
    }

    /* message size is current buffer offset - header size */
    smp_write_uint32(buffer + 4, offset - MSG_HEADER_SIZE);

    return offset;
}

uint32_t smp_message_get_msgid(SmpMessage *msg)
{
    return_val_if_fail(msg != NULL, 0);

    return msg->msgid;
}

int smp_message_get(SmpMessage *msg, int index, ...)
{
    va_list ap;

    return_val_if_fail(msg != NULL, -EINVAL);
    return_val_if_fail(index >= 0, -EINVAL);

    va_start(ap, index);

    do {
        SmpType type;

        if (index >= SMP_MESSAGE_MAX_VALUES)
            return -ENOENT;

        type = va_arg(ap, SmpType);
        if (type != msg->values[index].type)
            return -EBADF;

        switch (type) {
            case SMP_TYPE_UINT8: {
                uint8_t *ptr = va_arg(ap, uint8_t *);

                *ptr = msg->values[index].value.u8;
                break;
            }
            case SMP_TYPE_INT8: {
                int8_t *ptr = va_arg(ap, int8_t *);

                *ptr = msg->values[index].value.i8;
                break;
            }
            case SMP_TYPE_UINT16: {
                uint16_t *ptr = va_arg(ap, uint16_t *);

                *ptr = msg->values[index].value.u16;
                break;
            }

            case SMP_TYPE_INT16: {
                int16_t *ptr = va_arg(ap, int16_t *);

                *ptr = msg->values[index].value.i16;
                break;
            }
            case SMP_TYPE_UINT32: {
                uint32_t *ptr = va_arg(ap, uint32_t *);

                *ptr = msg->values[index].value.u32;
                break;
            }

            case SMP_TYPE_INT32: {
                int32_t *ptr = va_arg(ap, int32_t *);

                *ptr = msg->values[index].value.i32;
                break;
            }
            case SMP_TYPE_UINT64: {
                uint64_t *ptr = va_arg(ap, uint64_t *);

                *ptr = msg->values[index].value.u64;
                break;
            }

            case SMP_TYPE_INT64: {
                int64_t *ptr = va_arg(ap, int64_t *);

                *ptr = msg->values[index].value.i64;
                break;
            }
            default:
                return -EBADF;
        }

        index = va_arg(ap, int32_t);
    } while (index >= 0);

    va_end(ap);
    return 0;
}

int smp_message_get_value(SmpMessage *msg, int index, SmpValue *value)
{
    return_val_if_fail(msg != NULL, -EINVAL);
    return_val_if_fail(index >= 0, -EINVAL);
    return_val_if_fail(value != NULL, -EINVAL);

    if (index > SMP_MESSAGE_MAX_VALUES)
        return -ENOENT;

    if (msg->values[index].type == SMP_TYPE_NONE)
        return -ENOENT;

    *value = msg->values[index];
    return 0;
}

int smp_message_get_uint8(SmpMessage *msg, int index, uint8_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT8, value, -1);
}

int smp_message_get_int8(SmpMessage *msg, int index, int8_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT8, value, -1);
}

int smp_message_get_uint16(SmpMessage *msg, int index, uint16_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT16, value, -1);
}

int smp_message_get_int16(SmpMessage *msg, int index, int16_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT16, value, -1);
}

int smp_message_get_uint32(SmpMessage *msg, int index, uint32_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT32, value, -1);
}

int smp_message_get_int32(SmpMessage *msg, int index, int32_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT32, value, -1);
}

int smp_message_get_uint64(SmpMessage *msg, int index, uint64_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_UINT64, value, -1);
}

int smp_message_get_int64(SmpMessage *msg, int index, int64_t *value)
{
    return smp_message_get(msg, index, SMP_TYPE_INT64, value, -1);
}

int smp_message_set(SmpMessage *msg, int index, ...)
{
    va_list ap;
    int ret;

    return_val_if_fail(msg != NULL, -EINVAL);
    return_val_if_fail(index >= 0, -EINVAL);

    va_start(ap, index);

    do {
        SmpValue val;

        val.type = va_arg(ap, SmpType);

        switch (val.type) {
            case SMP_TYPE_UINT8:
                val.value.u8 = va_arg(ap, int);
                break;
            case SMP_TYPE_INT8:
                val.value.i8 = va_arg(ap, int);
                break;
            case SMP_TYPE_UINT16:
                val.value.u16 = va_arg(ap, int);
                break;
            case SMP_TYPE_INT16:
                val.value.i16 = va_arg(ap, int);
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
            default:
                break;
        }

        ret = smp_message_set_value(msg, index, &val);
        if (ret < 0)
            return ret;

        index = va_arg(ap, int);
    } while (index >= 0);

    va_end(ap);
    return 0;
}

int smp_message_set_value(SmpMessage *msg, int index, const SmpValue *value)
{
    return_val_if_fail(msg != NULL, -EINVAL);
    return_val_if_fail(index >= 0, -EINVAL);
    return_val_if_fail(value != NULL, -EINVAL);

    if (index >= SMP_MESSAGE_MAX_VALUES)
        return -ENOENT;

    msg->values[index] = *value;
    return 0;
}

int smp_message_set_uint8(SmpMessage *msg, int index, uint8_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT8, value, -1);
}

int smp_message_set_int8(SmpMessage *msg, int index, int8_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT8, value, -1);
}

int smp_message_set_uint16(SmpMessage *msg, int index, uint16_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT16, value, -1);
}

int smp_message_set_int16(SmpMessage *msg, int index, int16_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT16, value, -1);
}

int smp_message_set_uint32(SmpMessage *msg, int index, uint32_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT32, value, -1);
}

int smp_message_set_int32(SmpMessage *msg, int index, int32_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT32, value, -1);
}

int smp_message_set_uint64(SmpMessage *msg, int index, uint64_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_UINT64, value, -1);
}

int smp_message_set_int64(SmpMessage *msg, int index, int64_t value)
{
    return smp_message_set(msg, index, SMP_TYPE_INT64, value, -1);
}
