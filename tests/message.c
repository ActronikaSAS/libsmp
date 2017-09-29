/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#include <CUnit/CUnit.h>
#include <libsmp.h>
#include "tests.h"

static void setup_test_message_get(SmpMessage *msg)
{
    smp_message_init(msg, 33);

    msg->values[0].type = SMP_TYPE_UINT8;
    msg->values[0].value.u8 = 33;
    msg->values[1].type = SMP_TYPE_INT8;
    msg->values[1].value.i8 = -23;
    msg->values[2].type = SMP_TYPE_UINT16;
    msg->values[2].value.u16 = 23291;
    msg->values[3].type = SMP_TYPE_INT16;
    msg->values[3].value.i16 = -12333;
    msg->values[4].type = SMP_TYPE_UINT32;
    msg->values[4].value.u32 = 4355435;
    msg->values[5].type = SMP_TYPE_INT32;
    msg->values[5].value.i32 = -233214;
    msg->values[6].type = SMP_TYPE_UINT64;
    msg->values[6].value.u64 = 423535346;
    msg->values[7].type = SMP_TYPE_INT64;
    msg->values[7].value.i64 = -453126;
    msg->values[8].type = SMP_TYPE_UINT8;
    msg->values[8].value.i64 = 0;
}

static void test_smp_message_get(void)
{
    SmpMessage msg;
    int ret;
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    int16_t i16;
    uint32_t u32;
    int32_t i32;
    uint64_t u64;
    int64_t i64;

    setup_test_message_get(&msg);

    /* out of bound index should fail */
    ret = smp_message_get(&msg, SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8,
            &u8, -1);
    CU_ASSERT_EQUAL(ret, -ENOENT);

    ret = smp_message_get(&msg, 0, SMP_TYPE_UINT8, &u8,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, &u8,
            1, SMP_TYPE_INT8, &i8, -1);
    CU_ASSERT_EQUAL(ret, -ENOENT);

    ret = smp_message_get(&msg, 0, SMP_TYPE_UINT8, &u8,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, &u8, -1);
    CU_ASSERT_EQUAL(ret, -ENOENT);

    /* Bad type should cause an error */
    ret = smp_message_get(&msg, 0, SMP_TYPE_UINT32, &u32, -1);
    CU_ASSERT_EQUAL(ret, -EBADF);

    ret = smp_message_get(&msg, 0, SMP_TYPE_UINT8, &u8,
            1, SMP_TYPE_UINT32, &u32, -1);
    CU_ASSERT_EQUAL(ret, -EBADF);

    /* get the good way so it should pass */
    ret = smp_message_get(&msg,
            0, SMP_TYPE_UINT8, &u8,
            1, SMP_TYPE_INT8, &i8,
            2, SMP_TYPE_UINT16, &u16,
            3, SMP_TYPE_INT16, &i16,
            4, SMP_TYPE_UINT32, &u32,
            5, SMP_TYPE_INT32, &i32,
            6, SMP_TYPE_UINT64, &u64,
            7, SMP_TYPE_INT64, &i64,
            -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(u8, 33);
    CU_ASSERT_EQUAL(i8, -23);
    CU_ASSERT_EQUAL(u16, 23291);
    CU_ASSERT_EQUAL(i16, -12333);
    CU_ASSERT_EQUAL(u32, 4355435);
    CU_ASSERT_EQUAL(i32, -233214);
    CU_ASSERT_EQUAL(u64, 423535346);
    CU_ASSERT_EQUAL(i64, -453126);

    smp_message_clear(&msg);
}

static void test_smp_message_get_value(void)
{
    SmpMessage msg;
    SmpValue value;
    int ret;

    setup_test_message_get(&msg);

    /* out of bound index should fail */
    ret = smp_message_get_value(&msg, SMP_MESSAGE_MAX_VALUES + 10, &value);
    CU_ASSERT_EQUAL(ret, -ENOENT);

    /* not initialized value should failed */
    ret = smp_message_get_value(&msg, SMP_MESSAGE_MAX_VALUES - 1,
            &value);
    CU_ASSERT_EQUAL(ret, -ENOENT);

    /* we should get each value successfully */
    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 0, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(value.value.u8, 33);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 1, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(value.value.i8, -23);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 2, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(value.value.u16, 23291);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 3, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(value.value.i16, -12333);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 4, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(value.value.u32, 4355435);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 5, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(value.value.i32, -233214);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 6, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(value.value.u64, 423535346);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(&msg, 7, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(value.value.i64, -453126);
}

#define DEFINE_GET_TYPE_TEST_FUNC(type, index, expected_value) \
static void test_smp_message_get_##type(void) \
{ \
    SmpMessage msg; \
    int ret; \
    type##_t tmp; \
\
    setup_test_message_get(&msg); \
\
    /* out of bound index should fail */ \
    ret = smp_message_get_##type(&msg, SMP_MESSAGE_MAX_VALUES + 10, &tmp); \
    CU_ASSERT_EQUAL(ret, -ENOENT); \
\
    /* fail if value is not initialized, ie NONE */ \
    ret = smp_message_get_##type(&msg, SMP_MESSAGE_MAX_VALUES - 1, &tmp); \
    CU_ASSERT_EQUAL(ret, -EBADF); \
\
    /* fail if we have the wrong type */ \
    ret = smp_message_get_##type(&msg, (index) + 1, &tmp); \
    CU_ASSERT_EQUAL(ret, -EBADF); \
\
    /* good */ \
    ret = smp_message_get_##type(&msg, (index), &tmp); \
    CU_ASSERT_EQUAL(ret, 0); \
    CU_ASSERT_EQUAL(tmp, (expected_value)); \
}

DEFINE_GET_TYPE_TEST_FUNC(uint8, 0, 33);
DEFINE_GET_TYPE_TEST_FUNC(int8, 1, -23);
DEFINE_GET_TYPE_TEST_FUNC(uint16, 2, 23291);
DEFINE_GET_TYPE_TEST_FUNC(int16, 3, -12333);
DEFINE_GET_TYPE_TEST_FUNC(uint32, 4, 4355435);
DEFINE_GET_TYPE_TEST_FUNC(int32, 5, -233214);
DEFINE_GET_TYPE_TEST_FUNC(uint64, 6, 423535346);
DEFINE_GET_TYPE_TEST_FUNC(int64, 7, -453126);

static void test_smp_message_set(void)
{
    SmpMessage msg;
    int ret;

    /* out of bound index should return in error */
    smp_message_init(&msg, 33);
    ret = smp_message_set(&msg,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, 3, -1);
    CU_ASSERT_EQUAL(ret, -ENOENT);
    smp_message_clear(&msg);

    smp_message_init(&msg, 33);
    ret = smp_message_set(&msg,
            0, SMP_TYPE_UINT8, (uint8_t) 5,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, 3,
            0, SMP_TYPE_UINT8, (uint8_t) 4, -1);
    CU_ASSERT_EQUAL(ret, -ENOENT);
    smp_message_clear(&msg);

    smp_message_init(&msg, 33);
    ret = smp_message_set(&msg,
            0, SMP_TYPE_UINT8, 5,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, 3, -1);
    CU_ASSERT_EQUAL(ret, -ENOENT);
    smp_message_clear(&msg);

    /* Test some simple unique value */
    smp_message_init(&msg, 33);
    ret = smp_message_set(&msg,
            0, SMP_TYPE_UINT8, 33, -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg.values[0].type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(msg.values[0].value.u8, 33);
    smp_message_clear(&msg);

    smp_message_init(&msg, 33);
    ret = smp_message_set(&msg,
            2, SMP_TYPE_INT32, (int32_t) 334222, -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg.values[2].type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(msg.values[2].value.i32, 334222);
    smp_message_clear(&msg);

    /* test a more complex use case */
    smp_message_init(&msg, 33);
    ret = smp_message_set(&msg,
            0, SMP_TYPE_UINT8, 33,
            1, SMP_TYPE_INT8, -4,
            2, SMP_TYPE_UINT16, 24356,
            3, SMP_TYPE_INT16, -16533,
            4, SMP_TYPE_UINT32, (uint32_t) 554323,
            5, SMP_TYPE_INT32, (int32_t) -250002,
            6, SMP_TYPE_UINT64, (uint64_t) 1 << 55,
            7, SMP_TYPE_INT64, -((int64_t) 1 << 33),
            -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg.values[0].type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(msg.values[0].value.u8, 33);
    CU_ASSERT_EQUAL(msg.values[1].type, SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(msg.values[1].value.i8, -4);
    CU_ASSERT_EQUAL(msg.values[2].type, SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(msg.values[2].value.u16, 24356);
    CU_ASSERT_EQUAL(msg.values[3].type, SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(msg.values[3].value.i16, -16533);
    CU_ASSERT_EQUAL(msg.values[4].type, SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(msg.values[4].value.u32, 554323);
    CU_ASSERT_EQUAL(msg.values[5].type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(msg.values[5].value.i32, -250002);
    CU_ASSERT_EQUAL(msg.values[6].type, SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(msg.values[6].value.u64, (uint64_t) 1 << 55);
    CU_ASSERT_EQUAL(msg.values[7].type, SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(msg.values[7].value.i64, -((int64_t) 1 << 33));
    smp_message_clear(&msg);
}

static void test_smp_message_set_value(void)
{
    SmpValue value1 = { .type = SMP_TYPE_UINT8, .value.u8 = 112 };
    SmpValue value2 = { .type = SMP_TYPE_INT16, .value.i16 = -12344 };
    SmpMessage msg;
    int ret;

    /* out of bound index should return in error */
    smp_message_init(&msg, 33);
    ret = smp_message_set_value(&msg, SMP_MESSAGE_MAX_VALUES + 10, &value1);
    CU_ASSERT_EQUAL(ret, -ENOENT);
    smp_message_clear(&msg);

    /* should pass */
    smp_message_init(&msg, 33);
    ret = smp_message_set_value(&msg, 0, &value1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg.values[0].type, value1.type);
    CU_ASSERT_EQUAL(msg.values[0].value.u8, value1.value.u8);
    smp_message_clear(&msg);

    smp_message_init(&msg, 33);
    ret = smp_message_set_value(&msg, 4, &value2);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg.values[4].type, value2.type);
    CU_ASSERT_EQUAL(msg.values[4].value.i16, value2.value.i16);
    smp_message_clear(&msg);
}

#define DEFINE_SET_TYPE_TEST_FUNC(type_name, SMP_TYPE, val_name, svalue) \
static void test_smp_message_set_##type_name(void) \
{ \
    SmpMessage msg; \
    int ret; \
\
    /* out of bound index should fail */ \
    smp_message_init(&msg, 33);\
    ret = smp_message_set_##type_name(&msg, SMP_MESSAGE_MAX_VALUES + 10,\
            (svalue)); \
    CU_ASSERT_EQUAL(ret, -ENOENT); \
    smp_message_clear(&msg); \
\
    /* good */ \
    smp_message_init(&msg, 33); \
    ret = smp_message_set_##type_name(&msg, 0, (svalue)); \
    CU_ASSERT_EQUAL(ret, 0); \
    CU_ASSERT_EQUAL(msg.values[0].type, SMP_TYPE); \
    CU_ASSERT_EQUAL(msg.values[0].value.val_name, (svalue)); \
    smp_message_clear(&msg); \
}

DEFINE_SET_TYPE_TEST_FUNC(uint8, SMP_TYPE_UINT8, u8, 33);
DEFINE_SET_TYPE_TEST_FUNC(int8, SMP_TYPE_INT8, i8, -23);
DEFINE_SET_TYPE_TEST_FUNC(uint16, SMP_TYPE_UINT16, u16, 23291);
DEFINE_SET_TYPE_TEST_FUNC(int16, SMP_TYPE_INT16, i16, -12333);
DEFINE_SET_TYPE_TEST_FUNC(uint32, SMP_TYPE_UINT32, u32, 4355435);
DEFINE_SET_TYPE_TEST_FUNC(int32, SMP_TYPE_INT32, i32, -233214);
DEFINE_SET_TYPE_TEST_FUNC(uint64, SMP_TYPE_UINT64, u64, 423535346);
DEFINE_SET_TYPE_TEST_FUNC(int64, SMP_TYPE_INT64, i64, -453126);

static void test_smp_message_encode(void)
{
    SmpMessage msg;
    uint8_t buffer[1024];
    int ret;

    smp_message_init(&msg, 42);
    smp_message_set(&msg,
            0, SMP_TYPE_UINT8, 33,
            1, SMP_TYPE_INT8, -4,
            2, SMP_TYPE_UINT16, 24356,
            3, SMP_TYPE_INT16, -16533,
            4, SMP_TYPE_UINT32, (uint32_t) 554323,
            5, SMP_TYPE_INT32, (int32_t) -250002,
            6, SMP_TYPE_UINT64, (uint64_t) 1 << 55,
            7, SMP_TYPE_INT64, -((int64_t) 1 << 33),
            -1);

    /* encoding in a too small buffer should fail */
    ret = smp_message_encode(&msg, buffer, 10);
    CU_ASSERT_EQUAL(ret, -ENOMEM);

    /* This should work and we should get the following payload */
    ret = smp_message_encode(&msg, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(ret, 46);

    CU_ASSERT_EQUAL(*((uint32_t *)buffer), 42);
    CU_ASSERT_EQUAL(*((uint32_t *)(buffer + 4)), 38);
    CU_ASSERT_EQUAL(*(buffer + 8), SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(*(buffer + 9), 33);
    CU_ASSERT_EQUAL(*(buffer + 10), SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(*((int8_t *)(buffer + 11)), -4);
    CU_ASSERT_EQUAL(*(buffer + 12), SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(*((uint16_t *)(buffer + 13)), 24356);
    CU_ASSERT_EQUAL(*(buffer + 15), SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(*((int16_t *)(buffer + 16)), -16533);
    CU_ASSERT_EQUAL(*(buffer + 18), SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(*((uint32_t *)(buffer + 19)), 554323);
    CU_ASSERT_EQUAL(*(buffer + 23), SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(*((int32_t *)(buffer + 24)), -250002);
    CU_ASSERT_EQUAL(*(buffer + 28), SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(*((uint64_t *)(buffer + 29)), (uint64_t) 1 << 55);
    CU_ASSERT_EQUAL(*(buffer + 37), SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(*((int64_t *)(buffer + 38)), -((int64_t) 1 << 33));

    smp_message_clear(&msg);
}

union pval
{
    uint64_t u64;
    int64_t i64;

    uint8_t bytes[8];
};

static void test_smp_message_init_from_buffer(void)
{
    SmpMessage msg;
    union pval u64, i64;
    int ret;

    u64.u64 = 0x0004000000000312;
    i64.i64 = -(0x0a0403d0340312);

    uint8_t buffer[] = {
        0x03, 0x33, 0x24, 0x02,       /* message id */
        0x26, 0x00, 0x00, 0x00,       /* argument size */
        0x05, 0x24, 0x03, 0x00, 0x00, /* uint32_t = 804 */
        0x03, 0x3a, 0x00,             /* uint16_t = 58 */
        0x02, 0xf1,                   /* int8_t = -15 */
        0x01, 0x0a,                   /* uint8_t = 10 */

        /* uint64_t */
        0x07, u64.bytes[0], u64.bytes[1], u64.bytes[2], u64.bytes[3],
        u64.bytes[4], u64.bytes[5], u64.bytes[6], u64.bytes[7],

        /* int64_t */
        0x08, i64.bytes[0], i64.bytes[1], i64.bytes[2], i64.bytes[3],
        i64.bytes[4], i64.bytes[5], i64.bytes[6], i64.bytes[7],

        0x04, 0x2a, 0x80,             /* int16_t = -32726 */
        0x06, 0x2a, 0x80, 0xff, 0xff  /* int32_t = -32726 */
    };

    /* a too small buffer should return in error */
    ret = smp_message_init_from_buffer(&msg, buffer, 4);
    CU_ASSERT_EQUAL(ret, -EBADMSG);
    smp_message_clear(&msg);

    /* a too small buffer should return in error */
    ret = smp_message_init_from_buffer(&msg, buffer, 10);
    CU_ASSERT_EQUAL(ret, -EBADMSG);
    smp_message_clear(&msg);

    /* check a valid payload */
    ret = smp_message_init_from_buffer(&msg, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(ret, 0);

    CU_ASSERT_EQUAL(msg.msgid, 0x02243303);
    CU_ASSERT_EQUAL(msg.values[0].type, SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(msg.values[0].value.u32, 804);
    CU_ASSERT_EQUAL(msg.values[1].type, SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(msg.values[1].value.u16, 58);
    CU_ASSERT_EQUAL(msg.values[2].type, SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(msg.values[2].value.i8, -15);
    CU_ASSERT_EQUAL(msg.values[3].type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(msg.values[3].value.u8, 10);
    CU_ASSERT_EQUAL(msg.values[4].type, SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(msg.values[4].value.u64, u64.u64);
    CU_ASSERT_EQUAL(msg.values[5].type, SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(msg.values[5].value.i64, i64.i64);
    CU_ASSERT_EQUAL(msg.values[6].type, SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(msg.values[6].value.i16, -32726);
    CU_ASSERT_EQUAL(msg.values[7].type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(msg.values[7].value.i32, -32726);

    smp_message_clear(&msg);

    /* a corrupted size in buffer should cause an error */
    buffer[4] = 0xff;
    ret = smp_message_init_from_buffer(&msg, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(ret, -EBADMSG);

    /* a buffer with too many values should fail with the right error */
    {
        uint8_t buffer2[2 * (SMP_MESSAGE_MAX_VALUES + 2) + 8] = {
            0x03, 0x33, 0x24, 0x02,       /* message id */
            0x00,
        };
        int i;

        *((uint32_t *)(buffer2 + 4)) = 2 * (SMP_MESSAGE_MAX_VALUES + 2);

        for (i = 0; i < SMP_MESSAGE_MAX_VALUES + 2; i++) {
            buffer2[8 + 2 * i] = SMP_TYPE_UINT8;
            buffer2[8 + 2 * i + 1] = 0x42;
        }

        ret = smp_message_init_from_buffer(&msg, buffer2, sizeof(buffer2));
        CU_ASSERT_EQUAL(ret, -E2BIG);
        smp_message_clear(&msg);
    }
}

typedef struct
{
    const char *name;
    CU_TestFunc func;
} Test;

static Test tests[] = {
    { "test_smp_message_get", test_smp_message_get },
    { "test_smp_message_get_value", test_smp_message_get_value },
    { "test_smp_message_get_uint8", test_smp_message_get_uint8 },
    { "test_smp_message_get_int8", test_smp_message_get_int8 },
    { "test_smp_message_get_uint16", test_smp_message_get_uint16 },
    { "test_smp_message_get_int16", test_smp_message_get_int16 },
    { "test_smp_message_get_uint32", test_smp_message_get_uint32 },
    { "test_smp_message_get_int32", test_smp_message_get_int32 },
    { "test_smp_message_get_uint64", test_smp_message_get_uint64 },
    { "test_smp_message_get_int64", test_smp_message_get_int64 },
    { "test_smp_message_set", test_smp_message_set },
    { "test_smp_message_set_value", test_smp_message_set_value },
    { "test_smp_message_set_uint8", test_smp_message_set_uint8 },
    { "test_smp_message_set_int8", test_smp_message_set_int8 },
    { "test_smp_message_set_uint16", test_smp_message_set_uint16 },
    { "test_smp_message_set_int16", test_smp_message_set_int16 },
    { "test_smp_message_set_uint32", test_smp_message_set_uint32 },
    { "test_smp_message_set_int32", test_smp_message_set_int32 },
    { "test_smp_message_set_uint64", test_smp_message_set_uint64 },
    { "test_smp_message_set_int64", test_smp_message_set_int64 },
    { "test_smp_message_encode", test_smp_message_encode },
    { "test_smp_message_init_from_buffer", test_smp_message_init_from_buffer },
    { NULL, NULL }
};

CU_ErrorCode message_test_register(void)
{
    CU_pSuite suite = NULL;
    Test *t;

    suite = CU_add_suite("Message Suite", NULL, NULL);
    if (suite == NULL) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    for (t = tests; t->name != NULL; t++) {
        CU_pTest tret = CU_add_test(suite, t->name, t->func);
        if (tret == NULL)
            return CU_get_error();

    }

    return CUE_SUCCESS;
}
