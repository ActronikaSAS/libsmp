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

#include <CUnit/CUnit.h>

#define SMP_ENABLE_STATIC_API
#include <libsmp.h>
#include "tests.h"

#define SMP_N_ELEMENTS(arr) (sizeof((arr))/sizeof((arr)[0]))

/* use static variable to be sure while comparing */
static const float f32_orig_value = 1.42f;
static const double f64_orig_value = 3.14;

static void test_smp_message_new_with_id(void)
{
    SmpMessage *msg;

    msg = smp_message_new_with_id(42);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    CU_ASSERT_EQUAL(smp_message_get_msgid(msg), 42);
}

static void test_smp_message_new_from_static(void)
{
    SmpMessage *msg;
    SmpStaticMessage smsg;
    SmpValue storage[16];

    msg = smp_message_new_from_static(NULL, 0, NULL, 0);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static(&smsg, 0, NULL, 0);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static(&smsg, sizeof(smsg) - 1, NULL, 0);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static(&smsg, sizeof(smsg), NULL,
            SMP_N_ELEMENTS(storage));
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static(&smsg, sizeof(smsg), storage, 0);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static(&smsg, sizeof(smsg), storage,
            SMP_N_ELEMENTS(storage));
    CU_ASSERT_PTR_NOT_NULL(msg);

    msg = smp_message_new_from_static(&smsg, sizeof(smsg) + 10, storage,
            SMP_N_ELEMENTS(storage));
    CU_ASSERT_PTR_NOT_NULL(msg);
}

static void test_smp_message_new_from_static_with_id(void)
{
    SmpMessage *msg;
    SmpStaticMessage smsg;
    SmpValue storage[16];

    msg = smp_message_new_from_static_with_id(NULL, 0, NULL, 0, 42);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static_with_id(&smsg, 0, NULL, 0, 42);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static_with_id(&smsg, sizeof(smsg) - 1, NULL, 0,
            42);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static_with_id(&smsg, sizeof(smsg), NULL,
            SMP_N_ELEMENTS(storage), 42);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static_with_id(&smsg, sizeof(smsg), storage, 0,
            42);
    CU_ASSERT_PTR_NULL(msg);

    msg = smp_message_new_from_static_with_id(&smsg, sizeof(smsg), storage,
            SMP_N_ELEMENTS(storage), 42);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    CU_ASSERT_EQUAL(smp_message_get_msgid(msg), 42);

    msg = smp_message_new_from_static_with_id(&smsg, sizeof(smsg) + 10, storage,
            SMP_N_ELEMENTS(storage), 42);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    CU_ASSERT_EQUAL(smp_message_get_msgid(msg), 42);
}

static void setup_test_message_get(SmpMessage **msg)
{
    SmpMessage *tmp;

    tmp = smp_message_new_with_id(33);
    CU_ASSERT_PTR_NOT_NULL_FATAL(tmp);

    tmp->values[0].type = SMP_TYPE_UINT8;
    tmp->values[0].value.u8 = 33;
    tmp->values[1].type = SMP_TYPE_INT8;
    tmp->values[1].value.i8 = -23;
    tmp->values[2].type = SMP_TYPE_UINT16;
    tmp->values[2].value.u16 = 23291;
    tmp->values[3].type = SMP_TYPE_INT16;
    tmp->values[3].value.i16 = -12333;
    tmp->values[4].type = SMP_TYPE_UINT32;
    tmp->values[4].value.u32 = 4355435;
    tmp->values[5].type = SMP_TYPE_INT32;
    tmp->values[5].value.i32 = -233214;
    tmp->values[6].type = SMP_TYPE_UINT64;
    tmp->values[6].value.u64 = 423535346;
    tmp->values[7].type = SMP_TYPE_INT64;
    tmp->values[7].value.i64 = -453126;
    tmp->values[8].type = SMP_TYPE_STRING;
    tmp->values[8].value.cstring = "Hello world !";
    tmp->values[9].type = SMP_TYPE_UINT8;
    tmp->values[9].value.i64 = 0;

    /* for our test we process our raw data as a string */
    tmp->values[10].type = SMP_TYPE_RAW;
    tmp->values[10].value.craw = (uint8_t *) "rawdata";
    tmp->values[10].value.craw_size = 8;

    tmp->values[11].type = SMP_TYPE_F32;
    tmp->values[11].value.f32 = f32_orig_value;
    tmp->values[12].type = SMP_TYPE_F64;
    tmp->values[12].value.f64 = f64_orig_value;

    /* for out test, always keep a value at the end */
    tmp->values[13].type = SMP_TYPE_UINT8;
    tmp->values[13].value.i64 = 0;

    *msg = tmp;
}

static void test_smp_message_get(void)
{
    SmpMessage *msg;
    int ret;
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    int16_t i16;
    uint32_t u32;
    int32_t i32;
    uint64_t u64;
    int64_t i64;
    const char *str;
    const uint8_t *raw;
    size_t rawsize;
    float f32;
    double f64;

    setup_test_message_get(&msg);

    /* out of bound index should fail */
    ret = smp_message_get(msg, SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8,
            &u8, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    ret = smp_message_get(msg, 0, SMP_TYPE_UINT8, &u8,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, &u8,
            1, SMP_TYPE_INT8, &i8, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    ret = smp_message_get(msg, 0, SMP_TYPE_UINT8, &u8,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, &u8, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* Bad type should cause an error */
    ret = smp_message_get(msg, 0, SMP_TYPE_UINT32, &u32, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_TYPE);

    ret = smp_message_get(msg, 0, SMP_TYPE_UINT8, &u8,
            1, SMP_TYPE_UINT32, &u32, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_TYPE);

    /* get the good way so it should pass */
    ret = smp_message_get(msg,
            0, SMP_TYPE_UINT8, &u8,
            1, SMP_TYPE_INT8, &i8,
            2, SMP_TYPE_UINT16, &u16,
            3, SMP_TYPE_INT16, &i16,
            4, SMP_TYPE_UINT32, &u32,
            5, SMP_TYPE_INT32, &i32,
            6, SMP_TYPE_UINT64, &u64,
            7, SMP_TYPE_INT64, &i64,
            8, SMP_TYPE_STRING, &str,
            10, SMP_TYPE_RAW, &raw, &rawsize,
            11, SMP_TYPE_F32, &f32,
            12, SMP_TYPE_F64, &f64,
            -1);
    CU_ASSERT_EQUAL_FATAL(ret, 0);
    CU_ASSERT_EQUAL(u8, 33);
    CU_ASSERT_EQUAL(i8, -23);
    CU_ASSERT_EQUAL(u16, 23291);
    CU_ASSERT_EQUAL(i16, -12333);
    CU_ASSERT_EQUAL(u32, 4355435);
    CU_ASSERT_EQUAL(i32, -233214);
    CU_ASSERT_EQUAL(u64, 423535346);
    CU_ASSERT_EQUAL(i64, -453126);
    CU_ASSERT_STRING_EQUAL(str, "Hello world !");
    CU_ASSERT_STRING_EQUAL(raw, "rawdata");
    CU_ASSERT_EQUAL(rawsize, 8);
    CU_ASSERT_EQUAL(f32, f32_orig_value);
    CU_ASSERT_EQUAL(f64, f64_orig_value);

    smp_message_free(msg);
}

static void test_smp_message_get_value(void)
{
    SmpMessage *msg;
    SmpValue value;
    int ret;

    setup_test_message_get(&msg);

    /* out of bound index should fail */
    ret = smp_message_get_value(msg, SMP_MESSAGE_MAX_VALUES + 10, &value);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* not initialized value should failed */
    ret = smp_message_get_value(msg, SMP_MESSAGE_MAX_VALUES - 1,
            &value);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* we should get each value successfully */
    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 0, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(value.value.u8, 33);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 1, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(value.value.i8, -23);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 2, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(value.value.u16, 23291);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 3, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(value.value.i16, -12333);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 4, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(value.value.u32, 4355435);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 5, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(value.value.i32, -233214);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 6, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(value.value.u64, 423535346);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 7, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(value.value.i64, -453126);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 8, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_STRING);
    CU_ASSERT_EQUAL(value.value.cstring, "Hello world !");

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 10, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_RAW);
    CU_ASSERT_EQUAL(value.value.craw, "rawdata");
    CU_ASSERT_EQUAL(value.value.craw_size, 8);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 11, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_F32);
    CU_ASSERT_EQUAL(value.value.f32, f32_orig_value);

    memset(&value, 0, sizeof(value));
    ret = smp_message_get_value(msg, 12, &value);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(value.type, SMP_TYPE_F64);
    CU_ASSERT_EQUAL(value.value.f64, f64_orig_value);

    smp_message_free(msg);
}

#define DEFINE_GET_TYPE_TEST_FUNC(type, index, expected_value) \
static void test_smp_message_get_##type(void) \
{ \
    SmpMessage *msg; \
    int ret; \
    type##_t tmp; \
\
    setup_test_message_get(&msg); \
\
    /* out of bound index should fail */ \
    ret = smp_message_get_##type(msg, SMP_MESSAGE_MAX_VALUES + 10, &tmp); \
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND); \
\
    /* fail if value is not initialized, ie NONE */ \
    ret = smp_message_get_##type(msg, SMP_MESSAGE_MAX_VALUES - 1, &tmp); \
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND); \
\
    /* fail if we have the wrong type */ \
    ret = smp_message_get_##type(msg, (index) + 1, &tmp); \
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_TYPE); \
\
    /* good */ \
    ret = smp_message_get_##type(msg, (index), &tmp); \
    CU_ASSERT_EQUAL(ret, 0); \
    CU_ASSERT_EQUAL(tmp, (expected_value)); \
\
    smp_message_free(msg); \
\
}

DEFINE_GET_TYPE_TEST_FUNC(uint8, 0, 33);
DEFINE_GET_TYPE_TEST_FUNC(int8, 1, -23);
DEFINE_GET_TYPE_TEST_FUNC(uint16, 2, 23291);
DEFINE_GET_TYPE_TEST_FUNC(int16, 3, -12333);
DEFINE_GET_TYPE_TEST_FUNC(uint32, 4, 4355435);
DEFINE_GET_TYPE_TEST_FUNC(int32, 5, -233214);
DEFINE_GET_TYPE_TEST_FUNC(uint64, 6, 423535346);
DEFINE_GET_TYPE_TEST_FUNC(int64, 7, -453126);
DEFINE_GET_TYPE_TEST_FUNC(float, 11, f32_orig_value);
DEFINE_GET_TYPE_TEST_FUNC(double, 12, f64_orig_value);

static void test_smp_message_get_cstring(void)
{
    SmpMessage *msg;
    int ret;
    const char *str;

    setup_test_message_get(&msg);

    /* out of bound index should fail */
    ret = smp_message_get_cstring(msg, SMP_MESSAGE_MAX_VALUES + 10, &str);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* fail if value is not initialized, ie NONE */
    ret = smp_message_get_cstring(msg, SMP_MESSAGE_MAX_VALUES - 1, &str);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* fail if we have the wrong type */
    ret = smp_message_get_cstring(msg, 0, &str);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_TYPE);

    /* good */
    ret = smp_message_get_cstring(msg, 8, &str);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_STRING_EQUAL(str, "Hello world !");

    smp_message_free(msg);
}

static void test_smp_message_get_craw(void)
{
    SmpMessage *msg;
    int ret;
    const uint8_t *raw;
    size_t size;

    setup_test_message_get(&msg);

    /* out of bound index should fail */
    ret = smp_message_get_craw(msg, SMP_MESSAGE_MAX_VALUES + 10, &raw, &size);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* fail if value is not initialized, ie NONE */
    ret = smp_message_get_craw(msg, SMP_MESSAGE_MAX_VALUES - 1, &raw, &size);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);

    /* fail if we have the wrong type */
    ret = smp_message_get_craw(msg, 0, &raw, &size);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_TYPE);

    /* good */
    ret = smp_message_get_craw(msg, 10, &raw, &size);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_STRING_EQUAL(raw, "rawdata");
    CU_ASSERT_EQUAL(size, 8);

    smp_message_free(msg);
}

static void test_smp_message_set(void)
{
    SmpMessage *msg;
    int ret;

    msg = smp_message_new();
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    /* out of bound index should return in error */
    smp_message_init(msg, 33);
    ret = smp_message_set(msg,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, 3, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);
    smp_message_clear(msg);

    smp_message_init(msg, 33);
    ret = smp_message_set(msg,
            0, SMP_TYPE_UINT8, (uint8_t) 5,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, 3,
            0, SMP_TYPE_UINT8, (uint8_t) 4, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);
    smp_message_clear(msg);

    smp_message_init(msg, 33);
    ret = smp_message_set(msg,
            0, SMP_TYPE_UINT8, 5,
            SMP_MESSAGE_MAX_VALUES + 10, SMP_TYPE_UINT8, 3, -1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);
    smp_message_clear(msg);

    /* Test some simple unique value */
    smp_message_init(msg, 33);
    ret = smp_message_set(msg,
            0, SMP_TYPE_UINT8, 33, -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[0].type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(msg->values[0].value.u8, 33);
    smp_message_clear(msg);

    smp_message_init(msg, 33);
    ret = smp_message_set(msg,
            2, SMP_TYPE_INT32, (int32_t) 334222, -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[2].type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(msg->values[2].value.i32, 334222);
    smp_message_clear(msg);

    /* test a more complex use case */
    smp_message_init(msg, 33);
    ret = smp_message_set(msg,
            0, SMP_TYPE_UINT8, 33,
            1, SMP_TYPE_INT8, -4,
            2, SMP_TYPE_UINT16, 24356,
            3, SMP_TYPE_INT16, -16533,
            4, SMP_TYPE_UINT32, (uint32_t) 554323,
            5, SMP_TYPE_INT32, (int32_t) -250002,
            6, SMP_TYPE_UINT64, (uint64_t) 1 << 55,
            7, SMP_TYPE_INT64, -((int64_t) 1 << 33),
            8, SMP_TYPE_STRING, "Working",
            9, SMP_TYPE_RAW, "RawWorking", 11,
            10, SMP_TYPE_F32, f32_orig_value,
            11, SMP_TYPE_F64, f64_orig_value,
            -1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[0].type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(msg->values[0].value.u8, 33);
    CU_ASSERT_EQUAL(msg->values[1].type, SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(msg->values[1].value.i8, -4);
    CU_ASSERT_EQUAL(msg->values[2].type, SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(msg->values[2].value.u16, 24356);
    CU_ASSERT_EQUAL(msg->values[3].type, SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(msg->values[3].value.i16, -16533);
    CU_ASSERT_EQUAL(msg->values[4].type, SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(msg->values[4].value.u32, 554323);
    CU_ASSERT_EQUAL(msg->values[5].type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(msg->values[5].value.i32, -250002);
    CU_ASSERT_EQUAL(msg->values[6].type, SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(msg->values[6].value.u64, (uint64_t) 1 << 55);
    CU_ASSERT_EQUAL(msg->values[7].type, SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(msg->values[7].value.i64, -((int64_t) 1 << 33));
    CU_ASSERT_EQUAL(msg->values[8].type, SMP_TYPE_STRING);
    CU_ASSERT_STRING_EQUAL(msg->values[8].value.cstring, "Working");
    CU_ASSERT_EQUAL(msg->values[9].type, SMP_TYPE_RAW);
    CU_ASSERT_EQUAL(msg->values[9].value.craw, "RawWorking");
    CU_ASSERT_EQUAL(msg->values[9].value.craw_size, 11);
    CU_ASSERT_EQUAL(msg->values[10].type, SMP_TYPE_F32);
    CU_ASSERT_EQUAL(msg->values[10].value.f32, f32_orig_value);
    CU_ASSERT_EQUAL(msg->values[11].type, SMP_TYPE_F64);
    CU_ASSERT_EQUAL(msg->values[11].value.f64, f64_orig_value);
    smp_message_clear(msg);

    smp_message_free(msg);
}

static void test_smp_message_set_value(void)
{
    SmpValue value1 = { .type = SMP_TYPE_UINT8, .value.u8 = 112 };
    SmpValue value2 = { .type = SMP_TYPE_INT16, .value.i16 = -12344 };
    SmpMessage *msg;
    int ret;

    msg = smp_message_new();
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    /* out of bound index should return in error */
    smp_message_init(msg, 33);
    ret = smp_message_set_value(msg, SMP_MESSAGE_MAX_VALUES + 10, &value1);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);
    smp_message_clear(msg);

    /* should pass */
    smp_message_init(msg, 33);
    ret = smp_message_set_value(msg, 0, &value1);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[0].type, value1.type);
    CU_ASSERT_EQUAL(msg->values[0].value.u8, value1.value.u8);
    smp_message_clear(msg);

    smp_message_init(msg, 33);
    ret = smp_message_set_value(msg, 4, &value2);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[4].type, value2.type);
    CU_ASSERT_EQUAL(msg->values[4].value.i16, value2.value.i16);
    smp_message_clear(msg);

    smp_message_free(msg);
}

#define DEFINE_SET_TYPE_TEST_FUNC(type_name, SMP_TYPE, val_name, svalue) \
static void test_smp_message_set_##type_name(void) \
{ \
    SmpMessage *msg; \
    int ret; \
\
    msg = smp_message_new(); \
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg); \
\
    /* out of bound index should fail */ \
    smp_message_init(msg, 33);\
    ret = smp_message_set_##type_name(msg, SMP_MESSAGE_MAX_VALUES + 10,\
            (svalue)); \
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND); \
    smp_message_clear(msg); \
\
    /* good */ \
    smp_message_init(msg, 33); \
    ret = smp_message_set_##type_name(msg, 0, (svalue)); \
    CU_ASSERT_EQUAL(ret, 0); \
    CU_ASSERT_EQUAL(msg->values[0].type, SMP_TYPE); \
    CU_ASSERT_EQUAL(msg->values[0].value.val_name, (svalue)); \
    smp_message_clear(msg); \
\
    smp_message_free(msg);\
}

DEFINE_SET_TYPE_TEST_FUNC(uint8, SMP_TYPE_UINT8, u8, 33);
DEFINE_SET_TYPE_TEST_FUNC(int8, SMP_TYPE_INT8, i8, -23);
DEFINE_SET_TYPE_TEST_FUNC(uint16, SMP_TYPE_UINT16, u16, 23291);
DEFINE_SET_TYPE_TEST_FUNC(int16, SMP_TYPE_INT16, i16, -12333);
DEFINE_SET_TYPE_TEST_FUNC(uint32, SMP_TYPE_UINT32, u32, 4355435);
DEFINE_SET_TYPE_TEST_FUNC(int32, SMP_TYPE_INT32, i32, -233214);
DEFINE_SET_TYPE_TEST_FUNC(uint64, SMP_TYPE_UINT64, u64, 423535346);
DEFINE_SET_TYPE_TEST_FUNC(int64, SMP_TYPE_INT64, i64, -453126);
DEFINE_SET_TYPE_TEST_FUNC(float, SMP_TYPE_F32, f32, f32_orig_value);
DEFINE_SET_TYPE_TEST_FUNC(double, SMP_TYPE_F64, f64, f64_orig_value);

static void test_smp_message_set_cstring(void)
{
    SmpMessage *msg;
    int ret;

    msg = smp_message_new();
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    /* out of bound index should fail */
    smp_message_init(msg, 33);
    ret = smp_message_set_cstring(msg, SMP_MESSAGE_MAX_VALUES + 10, "foo");
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);
    smp_message_clear(msg);

    /* good */
    smp_message_init(msg, 33);
    ret = smp_message_set_cstring(msg, 0, "foobar");
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[0].type, SMP_TYPE_STRING);
    CU_ASSERT_STRING_EQUAL(msg->values[0].value.cstring, "foobar");
    smp_message_clear(msg);

    smp_message_free(msg);
}

static void test_smp_message_set_craw(void)
{
    SmpMessage *msg;
    int ret;

    msg = smp_message_new();
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    /* out of bound index should fail */
    smp_message_init(msg, 33);
    ret = smp_message_set_craw(msg, SMP_MESSAGE_MAX_VALUES + 10,
            (uint8_t *) "foo", 4);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NOT_FOUND);
    smp_message_clear(msg);

    /* good */
    smp_message_init(msg, 33);
    ret = smp_message_set_craw(msg, 0, (uint8_t *) "foobar", 7);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(msg->values[0].type, SMP_TYPE_RAW);
    CU_ASSERT_STRING_EQUAL(msg->values[0].value.craw, "foobar");
    CU_ASSERT_EQUAL(msg->values[0].value.craw_size, 7);
    smp_message_clear(msg);

    smp_message_free(msg);
}

static void test_smp_message_encode(void)
{
    SmpMessage *msg;
    uint8_t buffer[1024];
    int ret;
    const char *str = "Little string to check string works";
    const uint8_t rawdata[] = { 0x56, 0xff, 0x42, 0xa5, 0xbd, 0x16, 0x0f, 0x99,
        0x8c, 0x65, 0xa4, 0x88, 0x72 };
    size_t rawdata_len = SMP_N_ELEMENTS(rawdata);
    size_t i;

    msg = smp_message_new();
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    smp_message_init(msg, 42);
    smp_message_set(msg,
            0, SMP_TYPE_UINT8, 33,
            1, SMP_TYPE_INT8, -4,
            2, SMP_TYPE_UINT16, 24356,
            3, SMP_TYPE_INT16, -16533,
            4, SMP_TYPE_UINT32, (uint32_t) 554323,
            5, SMP_TYPE_INT32, (int32_t) -250002,
            6, SMP_TYPE_UINT64, (uint64_t) 1 << 55,
            7, SMP_TYPE_INT64, -((int64_t) 1 << 33),
            8, SMP_TYPE_STRING, str,
            9, SMP_TYPE_RAW, rawdata, rawdata_len,
            10, SMP_TYPE_F32, f32_orig_value,
            11, SMP_TYPE_F64, f64_orig_value,
            -1);

    /* encoding in a too small buffer should fail */
    ret = smp_message_encode(msg, buffer, 10);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_NO_MEM);

    /* This should work and we should get the following payload */
    ret = smp_message_encode(msg, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(ret, 46 + 4 + strlen(str) + (3 + rawdata_len) + 5 + 9);

    CU_ASSERT_EQUAL(*((uint32_t *)buffer), 42);
    CU_ASSERT_EQUAL(*((uint32_t *)(buffer + 4)), 38 + 4 + strlen(str)
            + (3 + rawdata_len) + 5 + 9);
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
    CU_ASSERT_EQUAL(*(buffer + 46), SMP_TYPE_STRING);
    CU_ASSERT_EQUAL(*((uint16_t *)(buffer + 47)), strlen(str) + 1);
    CU_ASSERT_STRING_EQUAL(buffer + 49, str);
    CU_ASSERT_EQUAL(*(buffer + 49 + strlen(str)), '\0');
    CU_ASSERT_EQUAL(*(buffer + 50 + strlen(str)), SMP_TYPE_RAW);
    CU_ASSERT_EQUAL(*(buffer + 51 + strlen(str)), rawdata_len);

    for (i = 0; i < rawdata_len; i++) {
        if (*(buffer + 53 + strlen(str) + i) != rawdata[i])
            CU_FAIL_FATAL("rawdata value mismatch");
    }

    CU_ASSERT_EQUAL(*(buffer + 53 + strlen(str) + rawdata_len), SMP_TYPE_F32);
    CU_ASSERT_EQUAL(*((float *)(buffer + 54 + strlen(str) + rawdata_len)),
            f32_orig_value);

    CU_ASSERT_EQUAL(*(buffer + 58 + strlen(str) + rawdata_len), SMP_TYPE_F64);
    CU_ASSERT_EQUAL(*((double *)(buffer + 59 + strlen(str) + rawdata_len)),
            f64_orig_value);

    smp_message_clear(msg);

    smp_message_free(msg);
}

union pval
{
    uint64_t u64;
    int64_t i64;
    float f32;
    double f64;

    uint8_t bytes[8];
};

static void test_smp_message_init_from_buffer(void)
{
    SmpMessage *msg;
    union pval u64, i64, f32, f64;
    int ret;

    u64.u64 = 0x0004000000000312;
    i64.i64 = -(0x0a0403d0340312);
    f32.f32 = f32_orig_value;
    f64.f64 = f64_orig_value;

    uint8_t buffer[] = {
        0x03, 0x33, 0x24, 0x02,       /* message id */
        0x2f, 0x00, 0x00, 0x00,       /* argument size */
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

        /* str : "hello" */
        0x09, 0x06, 0x00, 'h', 'e', 'l', 'l', 'o', '\0',

        0x04, 0x2a, 0x80,             /* int16_t = -32726 */
        0x06, 0x2a, 0x80, 0xff, 0xff, /* int32_t = -32726 */

        /* raw data: 0x42 0x66 0x36 0xa5 0xff */
        0x10, 0x05, 0x00, 0x42, 0x66, 0x36, 0xa5, 0xff,

        /* float */
        0x0a, f32.bytes[0], f32.bytes[1], f32.bytes[2], f32.bytes[3],

        /* double */
        0x0b, f64.bytes[0], f64.bytes[1], f64.bytes[2], f64.bytes[3],
        f64.bytes[4], f64.bytes[5], f64.bytes[6], f64.bytes[7],
    };

    msg = smp_message_new();
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);

    /* a too small buffer should return in error */
    ret = smp_message_init_from_buffer(msg, buffer, 4);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);
    smp_message_clear(msg);

    /* a too small buffer should return in error */
    ret = smp_message_init_from_buffer(msg, buffer, 10);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);
    /* check a valid payload */
    ret = smp_message_init_from_buffer(msg, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(ret, 0);

    CU_ASSERT_EQUAL(msg->msgid, 0x02243303);
    CU_ASSERT_EQUAL(msg->values[0].type, SMP_TYPE_UINT32);
    CU_ASSERT_EQUAL(msg->values[0].value.u32, 804);
    CU_ASSERT_EQUAL(msg->values[1].type, SMP_TYPE_UINT16);
    CU_ASSERT_EQUAL(msg->values[1].value.u16, 58);
    CU_ASSERT_EQUAL(msg->values[2].type, SMP_TYPE_INT8);
    CU_ASSERT_EQUAL(msg->values[2].value.i8, -15);
    CU_ASSERT_EQUAL(msg->values[3].type, SMP_TYPE_UINT8);
    CU_ASSERT_EQUAL(msg->values[3].value.u8, 10);
    CU_ASSERT_EQUAL(msg->values[4].type, SMP_TYPE_UINT64);
    CU_ASSERT_EQUAL(msg->values[4].value.u64, u64.u64);
    CU_ASSERT_EQUAL(msg->values[5].type, SMP_TYPE_INT64);
    CU_ASSERT_EQUAL(msg->values[5].value.i64, i64.i64);
    CU_ASSERT_EQUAL(msg->values[6].type, SMP_TYPE_STRING);
    CU_ASSERT_STRING_EQUAL(msg->values[6].value.cstring, "hello");
    CU_ASSERT_EQUAL(msg->values[7].type, SMP_TYPE_INT16);
    CU_ASSERT_EQUAL(msg->values[7].value.i16, -32726);
    CU_ASSERT_EQUAL(msg->values[8].type, SMP_TYPE_INT32);
    CU_ASSERT_EQUAL(msg->values[8].value.i32, -32726);
    CU_ASSERT_EQUAL(msg->values[9].type, SMP_TYPE_RAW);
    CU_ASSERT_EQUAL(msg->values[9].value.craw_size, 5);
    CU_ASSERT_EQUAL(msg->values[9].value.craw[0], 0x42);
    CU_ASSERT_EQUAL(msg->values[9].value.craw[1], 0x66);
    CU_ASSERT_EQUAL(msg->values[9].value.craw[2], 0x36);
    CU_ASSERT_EQUAL(msg->values[9].value.craw[3], 0xa5);
    CU_ASSERT_EQUAL(msg->values[9].value.craw[4], 0xff);
    CU_ASSERT_EQUAL(msg->values[10].type, SMP_TYPE_F32);
    CU_ASSERT_EQUAL(msg->values[10].value.f32, f32_orig_value);
    CU_ASSERT_EQUAL(msg->values[11].type, SMP_TYPE_F64);
    CU_ASSERT_EQUAL(msg->values[11].value.f64, f64_orig_value);

    smp_message_clear(msg);

    /* a corrupted size in buffer should cause an error */
    buffer[4] = 0xff;
    ret = smp_message_init_from_buffer(msg, buffer, sizeof(buffer));
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);

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

        ret = smp_message_init_from_buffer(msg, buffer2, sizeof(buffer2));
        CU_ASSERT_EQUAL(ret, SMP_ERROR_TOO_BIG);
        smp_message_clear(msg);
    }

    /* a buffer with a bad string size should fail */
    {
        uint8_t buffer2[] = {
            0x03, 0x33, 0x24, 0x02, /* message id */
            0x09, 0x00, 0x00, 0x00, /* argument size */

            /* str : "hello" */
            0x09, 0x44, 0x00, 'h', 'e', 'l', 'l', 'o', '\0',
        };

        ret = smp_message_init_from_buffer(msg, buffer2, sizeof(buffer2));
        CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);
    }

    /* a buffer with a non nul-terminated string should fail */
    {
        uint8_t buffer2[] = {
            0x03, 0x33, 0x24, 0x02, /* message id */
            0x09, 0x00, 0x00, 0x00, /* argument size */

            /* str : "hello" */
            0x09, 0x06, 0x00, 'h', 'e', 'l', 'l', 'o', 0x45,
        };

        ret = smp_message_init_from_buffer(msg, buffer2, sizeof(buffer2));
        CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);
    }

    smp_message_free(msg);
}

SMP_DEFINE_STATIC_MESSAGE(test_macro, 4)
static void test_smp_message_static_helper_macro(void)
{
    SmpMessage *message;

    message = test_macro_create(42);
    CU_ASSERT_PTR_NOT_NULL_FATAL(message);

    CU_ASSERT_EQUAL(smp_message_get_msgid(message), 42);
}

typedef struct
{
    const char *name;
    CU_TestFunc func;
} Test;

static Test tests[] = {
    { "test_smp_message_new_with_id", test_smp_message_new_with_id },
    { "test_smp_message_new_from_static", test_smp_message_new_from_static },
    { "test_smp_message_new_from_static_with_id",
        test_smp_message_new_from_static_with_id },
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
    { "test_smp_message_get_float", test_smp_message_get_float },
    { "test_smp_message_get_double", test_smp_message_get_double },
    { "test_smp_message_get_cstring", test_smp_message_get_cstring },
    { "test_smp_message_get_craw", test_smp_message_get_craw },
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
    { "test_smp_message_set_float", test_smp_message_set_float },
    { "test_smp_message_set_double", test_smp_message_set_double },
    { "test_smp_message_set_cstring", test_smp_message_set_cstring },
    { "test_smp_message_set_craw", test_smp_message_set_craw },
    { "test_smp_message_encode", test_smp_message_encode },
    { "test_smp_message_init_from_buffer", test_smp_message_init_from_buffer },
    { "test_smp_message_static_helper_macro",
        test_smp_message_static_helper_macro},
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

