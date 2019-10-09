/* libsmp
 * Copyright (C) 2017-2018 Actronika SAS
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>

#include "serial-protocol.h"
#include "tests.h"

#define START_BYTE 0x10
#define END_BYTE 0xFF
#define ESC_BYTE 0x1B

static void test_smp_serial_protocol_encode_simple_check_buffer(uint8_t *buffer,
        ssize_t size)
{
    ssize_t expected_size;
    off_t offset = 0;
    int len = strlen("Hello World !") + 1;

    /* when encoding succeed, we should have a result buffer with:
     * START_BYTE | Hello World !\0 | CRC | END_BYTE
     * so size should be 1 + (strlen(str) + 1) + 2 */
    expected_size = 1 + len + 2;
    CU_ASSERT_EQUAL(size, expected_size);

    CU_ASSERT_EQUAL(buffer[offset++], (uint8_t) START_BYTE);
    CU_ASSERT_STRING_EQUAL(buffer + offset, "Hello World !");
    offset += len;
    CU_ASSERT_EQUAL(buffer[offset++], 0x21); /* checksum */
    CU_ASSERT_EQUAL(buffer[offset++], (uint8_t) END_BYTE);
}

static void test_smp_serial_protocol_encode_simple(void)
{
    const char *str = "Hello World !";
    size_t len = strlen(str) + 1;
    ssize_t ret;
    uint8_t soutbuf[32];
    uint8_t *outbuf;

    /* calling with invalid args should fail */
    ret = smp_serial_protocol_encode(NULL, 0, NULL, 0);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_INVALID_PARAM);

    ret = smp_serial_protocol_encode((const uint8_t *)str, 0, NULL, 0);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_INVALID_PARAM);

    ret = smp_serial_protocol_encode((const uint8_t *)str, len, NULL, 0);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_INVALID_PARAM);

    outbuf = soutbuf;
    ret = smp_serial_protocol_encode((const uint8_t *)str, len, &outbuf, 0);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_INVALID_PARAM);

    outbuf = soutbuf;
    ret = smp_serial_protocol_encode((const uint8_t *)str, len, &outbuf,
            sizeof(soutbuf));
    CU_ASSERT_PTR_EQUAL(outbuf, soutbuf);
    test_smp_serial_protocol_encode_simple_check_buffer(outbuf, ret);

    /* test with auto allocated output buffer */
    outbuf = NULL;
    ret = smp_serial_protocol_encode((const uint8_t *)str, len, &outbuf, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(outbuf);
    test_smp_serial_protocol_encode_simple_check_buffer(outbuf, ret);
    free(outbuf);
}

static void test_smp_serial_protocol_encode_magic_bytes(void)
{
    uint8_t payload[] = {
        /* begin with start-byte and insert magic bytes to make sure they are
         * escaped */
        START_BYTE, 0x45, 0x23, 0x04, 0x00, ESC_BYTE, END_BYTE, END_BYTE,
        0x33, 0x44, ESC_BYTE, ESC_BYTE, START_BYTE, 0x42
    };
    uint8_t expected_frame[] = {
        START_BYTE,
        /* escaped payload */
        ESC_BYTE, START_BYTE, 0x45, 0x23, 0x04, 0x00, ESC_BYTE, ESC_BYTE,
        ESC_BYTE, END_BYTE, ESC_BYTE, END_BYTE, 0x33, 0x44, ESC_BYTE, ESC_BYTE,
        ESC_BYTE, ESC_BYTE, ESC_BYTE,
        START_BYTE, 0x42,
        /* checksum and end byte */
        0x4c, END_BYTE
    };
    uint8_t *outbuf = NULL;
    ssize_t ret;
    size_t i;

    ret = smp_serial_protocol_encode(payload, sizeof(payload), &outbuf, 0);
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(expected_frame));
    CU_ASSERT_PTR_NOT_NULL_FATAL(outbuf);

    /* we should read:
     * START_BYTE | (escaped payload = sizeof(payload) + 7) | CRC | END_BYTE */
    for (i = 0; i < sizeof(expected_frame); i++) {
        if (outbuf[i] != expected_frame[i]) {
            CU_FAIL("frame mismatch");
            break;
        }
    }
    free(outbuf);
}

static void test_smp_serial_protocol_encode_magic_crc(void)
{
    uint8_t payload[3][1] = {
        /* special payload with checksum equals to magic bytes */
        { START_BYTE },
        { ESC_BYTE },
        { END_BYTE },
    };
    uint8_t expected_frame[3][6] = {
        {
            START_BYTE,              /* begin frame */
            ESC_BYTE, START_BYTE,    /* payload */
            ESC_BYTE, START_BYTE,    /* escaped CRC */
            END_BYTE                 /* end frame */
        },
        {
            START_BYTE,              /* begin frame */
            ESC_BYTE, ESC_BYTE,      /* payload */
            ESC_BYTE, ESC_BYTE,      /* escaped CRC */
            END_BYTE                 /* end frame */
        },
        {
            START_BYTE,              /* begin frame */
            ESC_BYTE, END_BYTE,      /* payload */
            ESC_BYTE, END_BYTE,      /* escaped CRC */
            END_BYTE                 /* end frame */
        },

    };
    int i;

    for (i = 0; i < 3; i++) {
        uint8_t *outbuf = NULL;
        ssize_t ret;
        ssize_t j;

        ret = smp_serial_protocol_encode(payload[i], sizeof(payload[i]),
                &outbuf, 0);
        CU_ASSERT_EQUAL(ret, sizeof(expected_frame[i]));
        CU_ASSERT_PTR_NOT_NULL_FATAL(outbuf);
        if (ret != sizeof(expected_frame[i])) {
            free(outbuf);
            continue;
        }

        for (j = 0; j < ret; j++) {
            if (outbuf[j] != expected_frame[i][j]) {
                CU_FAIL("frame mismatch");
                free(outbuf);
                break;
            }
        }
        free(outbuf);
    }
}

static void test_smp_serial_protocol_decoder_new(void)
{
    SmpSerialProtocolDecoder *decoder;

    decoder = smp_serial_protocol_decoder_new(0);
    CU_ASSERT_PTR_NOT_NULL(decoder);

    smp_serial_protocol_decoder_free(decoder);
}

static void test_smp_serial_protocol_decoder_new_from_static(void)
{
    SmpSerialProtocolDecoder *decoder;
    SmpStaticSerialProtocolDecoder sdecoder;
    uint8_t buf[32];

    decoder = smp_serial_protocol_decoder_new_from_static(NULL, 0, NULL, 0);
    CU_ASSERT_PTR_NULL(decoder);

    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder) - 1, buf, sizeof(buf));
    CU_ASSERT_PTR_NULL(decoder);

    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder), NULL, 0);
    CU_ASSERT_PTR_NULL(decoder);

    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder), buf, 0);
    CU_ASSERT_PTR_NULL(decoder);

    /* should work */
    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder), buf, sizeof(buf));
    CU_ASSERT_PTR_NOT_NULL(decoder);
    CU_ASSERT_PTR_EQUAL(decoder, &sdecoder);
    CU_ASSERT_PTR_EQUAL(decoder->buf, buf);
    CU_ASSERT_PTR_EQUAL(decoder->bufsize, sizeof(buf));
    CU_ASSERT_PTR_EQUAL(decoder->maxsize, sizeof(buf));
    CU_ASSERT_PTR_EQUAL(decoder->statically_allocated, true);

    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder) + 1, buf, sizeof(buf));
    CU_ASSERT_PTR_NOT_NULL(decoder);
    CU_ASSERT_PTR_EQUAL(decoder, &sdecoder);
    CU_ASSERT_PTR_EQUAL(decoder->buf, buf);
    CU_ASSERT_PTR_EQUAL(decoder->bufsize, sizeof(buf));
    CU_ASSERT_PTR_EQUAL(decoder->maxsize, sizeof(buf));
    CU_ASSERT_PTR_EQUAL(decoder->statically_allocated, true);
}

typedef struct {
    SmpSerialProtocolDecoder *decoder;

    uint8_t *payload;
    size_t psize;

    uint8_t *encoded_payload;
    size_t esize;

    size_t offset;

    uint8_t *frame;
    size_t framesize;
} TestDecoderCtx;

static TestDecoderCtx *test_decoder_new_full(size_t decoder_bsize,
        uint8_t *payload, size_t psize, bool encode)
{
    TestDecoderCtx *ctx;
    ssize_t ret;

    ctx = calloc(1, sizeof(*ctx));
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);

    ctx->decoder = smp_serial_protocol_decoder_new(decoder_bsize);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->decoder);

    ctx->payload = payload;
    ctx->psize = psize;

    if (encode) {
        ret = smp_serial_protocol_encode(payload, psize, &ctx->encoded_payload,
                0);
        CU_ASSERT_TRUE_FATAL(ret > 0);
        CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->encoded_payload);
        ctx->esize = ret;
    } else {
        ctx->encoded_payload = payload;
        ctx->esize = psize;
    }

    ctx->offset = 0;
    return ctx;
}

static TestDecoderCtx *test_decoder_new(uint8_t *payload, size_t psize)
{
    return test_decoder_new_full(1024, payload, psize, false);
}

static void test_decoder_free(TestDecoderCtx *ctx)
{
    if (ctx->encoded_payload != ctx->payload)
        free(ctx->encoded_payload);

    smp_serial_protocol_decoder_free(ctx->decoder);
    free(ctx);
}

#define PAYLOAD_PROCESSED INT_MAX
static int test_decoder_process_payload(TestDecoderCtx *ctx)
{
    int ret = PAYLOAD_PROCESSED;

    ctx->frame = NULL;
    ctx->framesize = 0;

    while (ctx->offset < ctx->esize) {
        ret = smp_serial_protocol_decoder_process_byte(ctx->decoder,
                ctx->encoded_payload[ctx->offset++], &ctx->frame,
                &ctx->framesize);
        if (ret < 0 || ctx->frame != NULL)
            break;

        ret = PAYLOAD_PROCESSED;
    }

    return ret;
}

static void test_decoder_check_frame(TestDecoderCtx *ctx,
        uint8_t *expected_frame, size_t expected_framesize)
{
    size_t i;

    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->frame);
    CU_ASSERT_EQUAL(ctx->framesize, expected_framesize);

    for (i = 0; i < ctx->framesize; i++) {
        if (ctx->frame[i] != expected_frame[i]) {
            CU_FAIL("frame mismatch");
            break;
        }
    }
}

static void test_smp_serial_protocol_decoder_simple_payload(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = {
        START_BYTE, 0x45, 0x23, 0x04, 0x00, ESC_BYTE, END_BYTE, END_BYTE,
        0x33, 0x44, ESC_BYTE, ESC_BYTE, START_BYTE, 0x42
    };
    int ret;

    ctx = test_decoder_new_full(1024, payload, sizeof(payload), true);

    /* the first part of the payload should output a frame */
    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    test_decoder_check_frame(ctx, payload, sizeof(payload));

    /* the remaining garbage should not trigger anything */
    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, PAYLOAD_PROCESSED);
    CU_ASSERT_PTR_NULL(ctx->frame);

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_start_without_end(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = {
        START_BYTE, 0x43, 0x23, START_BYTE, 0x22, 0x33, 0x32, 0x23, END_BYTE
    };
    int ret;

    ctx = test_decoder_new(payload, sizeof(payload));

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);
    CU_ASSERT_PTR_NULL(ctx->frame);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    test_decoder_check_frame(ctx, payload + 4, 3);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, PAYLOAD_PROCESSED);

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_bad_crc(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = { START_BYTE, 0x42, 0x33, 0x00, END_BYTE };
    int ret;

    ctx = test_decoder_new(payload, sizeof(payload));

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);
    CU_ASSERT_PTR_NULL(ctx->frame);

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_too_big(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[8] = { START_BYTE, 0x00, };
    int ret;

    ctx = test_decoder_new_full(4, payload, sizeof(payload), false);

    /* switch to statically_allocated to check the path */
    ctx->decoder->statically_allocated = true;

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_TOO_BIG);
    CU_ASSERT_PTR_NULL(ctx->frame);

    ctx->decoder->statically_allocated = false;
    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_too_big_esc(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = { START_BYTE, 0x00, 0x00, 0x00, 0x00, ESC_BYTE, 0x00 };
    int ret;

    ctx = test_decoder_new_full(4, payload, sizeof(payload), false);

    /* switch to statically_allocated to check the path */
    ctx->decoder->statically_allocated = true;

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_TOO_BIG);
    CU_ASSERT_PTR_NULL(ctx->frame);

    ctx->decoder->statically_allocated = false;
    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_crc_escaped(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = {
        START_BYTE, ESC_BYTE, START_BYTE, ESC_BYTE, START_BYTE, END_BYTE,
        START_BYTE, ESC_BYTE, END_BYTE, ESC_BYTE, END_BYTE, END_BYTE,
        START_BYTE, ESC_BYTE, ESC_BYTE, ESC_BYTE, ESC_BYTE, END_BYTE,
    };
    int ret;

    ctx = test_decoder_new(payload, sizeof(payload));

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->frame);
    CU_ASSERT_EQUAL(ctx->framesize, 1);
    CU_ASSERT_EQUAL(ctx->frame[0], START_BYTE);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->frame);
    CU_ASSERT_EQUAL(ctx->framesize, 1);
    CU_ASSERT_EQUAL(ctx->frame[0], END_BYTE);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->frame);
    CU_ASSERT_EQUAL(ctx->framesize, 1);
    CU_ASSERT_EQUAL(ctx->frame[0], ESC_BYTE);

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_frames_and_garbage(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = {
        /* some garbage first */
        0x33, 0x22, 0x01, 0x0a, END_BYTE, ESC_BYTE,

        /* now the first frame */
        START_BYTE, 0x12, 0x4e, 0x1f, 0xb0, 0x00, 0x33, 0xc0, END_BYTE,

        /* now some garbage */
        0x19, 0xaf, 0x43, 0x92, 0x09,

        /* the second frame */
        START_BYTE, 0x12, 0x4e, 0x1f, 0xb0, 0x00, 0x33, 0xc0, END_BYTE
    };
    int ret;

    ctx = test_decoder_new(payload, sizeof(payload));

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    test_decoder_check_frame(ctx, payload + 7, 6);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    test_decoder_check_frame(ctx, payload + 21, 6);

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_start_end(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = {
        /* just a start byte followed by an end byte which causes an underflow
         * on framesize variable computation and causes an out of bound read
         * when computing checksum */
        START_BYTE, END_BYTE
    };
    int ret;

    ctx = test_decoder_new(payload, sizeof(payload));

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_BAD_MESSAGE);

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_resize_decoder(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    int ret;

    ctx = test_decoder_new_full(8, payload, sizeof(payload), true);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx->decoder);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    test_decoder_check_frame(ctx, payload, sizeof(payload));

    test_decoder_free(ctx);
}

static void test_smp_serial_protocol_decoder_resize_decoder_limit(void)
{
    TestDecoderCtx *ctx;
    uint8_t payload[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    int ret;

    ctx = test_decoder_new_full(8, payload, sizeof(payload), true);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);

    ret = smp_serial_protocol_decoder_set_maximum_capacity(NULL, 8);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_INVALID_PARAM);

    ret = smp_serial_protocol_decoder_set_maximum_capacity(ctx->decoder, 4);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_INVALID_PARAM);

    smp_serial_protocol_decoder_set_maximum_capacity(ctx->decoder, 8);

    ret = test_decoder_process_payload(ctx);
    CU_ASSERT_EQUAL(ret, SMP_ERROR_TOO_BIG);

    test_decoder_free(ctx);
}

typedef struct
{
    const char *name;
    CU_TestFunc func;
} Test;

static Test tests[] = {
    DEFINE_TEST(test_smp_serial_protocol_encode_simple),
    DEFINE_TEST(test_smp_serial_protocol_encode_magic_bytes),
    DEFINE_TEST(test_smp_serial_protocol_encode_magic_crc),
    DEFINE_TEST(test_smp_serial_protocol_decoder_new),
    DEFINE_TEST(test_smp_serial_protocol_decoder_new_from_static),
    DEFINE_TEST(test_smp_serial_protocol_decoder_simple_payload),
    DEFINE_TEST(test_smp_serial_protocol_decoder_start_without_end),
    DEFINE_TEST(test_smp_serial_protocol_decoder_bad_crc),
    DEFINE_TEST(test_smp_serial_protocol_decoder_too_big),
    DEFINE_TEST(test_smp_serial_protocol_decoder_too_big_esc),
    DEFINE_TEST(test_smp_serial_protocol_decoder_crc_escaped),
    DEFINE_TEST(test_smp_serial_protocol_decoder_frames_and_garbage),
    DEFINE_TEST(test_smp_serial_protocol_decoder_start_end),
    DEFINE_TEST(test_smp_serial_protocol_decoder_resize_decoder),
    DEFINE_TEST(test_smp_serial_protocol_decoder_resize_decoder_limit),
    { NULL, NULL }
};

CU_ErrorCode serial_protocol_test_register(void)
{
    CU_pSuite suite = NULL;
    Test *t;

    suite = CU_add_suite("Serial Protocol Suite", NULL, NULL);
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
