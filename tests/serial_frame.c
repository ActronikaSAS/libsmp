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
#include <libsmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tests.h"

#define FIFO_PATH "/tmp/smp-test-fifo"

#define START_BYTE 0x10
#define END_BYTE 0xFF
#define ESC_BYTE 0x1B

typedef struct
{
    int fd; /* device fd */
} TestCtx;

static void test_setup(TestCtx *ctx)
{
    int ret;

    /* ensure fifo doesn't exist */
    unlink(FIFO_PATH);

    ret = mkfifo(FIFO_PATH, S_IWUSR | S_IRUSR);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    ctx->fd = open(FIFO_PATH, O_RDWR | O_NONBLOCK);
    if (ctx->fd < 0)
        CU_FAIL_FATAL("failed to open fifo");
}

static void test_teardown(TestCtx *ctx)
{
    close(ctx->fd);
    unlink(FIFO_PATH);
}

static void simple_new_frame_cb(uint8_t *frame, size_t size, void *userdata)
{
}

static void simple_error_cb(SmpSerialFrameError error, void *userdata)
{
}

static const SmpSerialFrameDecoderCallbacks simple_cbs = {
    .new_frame = simple_new_frame_cb,
    .error = simple_error_cb
};

static void test_smp_serial_frame_init(void)
{
    TestCtx tctx;
    SmpSerialFrameContext ctx;
    int ret;
    const SmpSerialFrameDecoderCallbacks *cbs = &simple_cbs;

    test_setup(&tctx);

    /* calling with invalid args shall fails */
    ret = smp_serial_frame_init(NULL, NULL, NULL, NULL);
    CU_ASSERT_EQUAL(ret, -EINVAL);

    ret = smp_serial_frame_init(NULL, FIFO_PATH, cbs, NULL);
    CU_ASSERT_EQUAL(ret, -EINVAL);

    ret = smp_serial_frame_init(&ctx, NULL, cbs, NULL);
    CU_ASSERT_EQUAL(ret, -EINVAL);

    ret = smp_serial_frame_init(&ctx, "/sfnjiejdfeifsd", cbs, NULL);
    CU_ASSERT_EQUAL(ret, -ENOENT);

    ret = smp_serial_frame_init(&ctx, FIFO_PATH, NULL, NULL);
    CU_ASSERT_EQUAL(ret, -EINVAL);

    /* calling with good argument shall pass */
    ret = smp_serial_frame_init(&ctx, FIFO_PATH, cbs, NULL);
    CU_ASSERT_EQUAL(ret, 0);

    smp_serial_frame_deinit(&ctx);

    test_teardown(&tctx);
}

static void test_smp_serial_frame_send_simple(void)
{
    TestCtx tctx;
    SmpSerialFrameContext ctx;
    const char *str = "Hello World !";
    char rbuf[64];
    ssize_t rbytes;
    int ret;

    test_setup(&tctx);

    ret = smp_serial_frame_init(&ctx, FIFO_PATH, &simple_cbs, NULL);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    /* calling without context or buffer should fail */
    ret = smp_serial_frame_send(NULL, (const uint8_t *) str, strlen(str) + 1);
    CU_ASSERT_EQUAL(ret, -EINVAL);

    ret = smp_serial_frame_send(&ctx, NULL, strlen(str) + 1);
    CU_ASSERT_EQUAL(ret, -EINVAL);

    /* try to send a message and read it back */
    ret = smp_serial_frame_send(&ctx, (const uint8_t *) str, strlen(str) + 1);
    CU_ASSERT_EQUAL(ret, 0);

    rbytes = read(tctx.fd, rbuf, sizeof(rbuf));

    /* we should have a buffer with:
     * START_BYTE | Hello World !\0 | CRC | END_BYTE
     * so size should be 1 + (strlen(str) + 1) + 2 */
    CU_ASSERT_EQUAL(rbytes, strlen(str) + 4);

    CU_ASSERT_EQUAL(rbuf[0], (char) START_BYTE);
    CU_ASSERT_STRING_EQUAL(rbuf + 1, str);
    CU_ASSERT_EQUAL(rbuf[1 + strlen(str) + 1], 0x21); /* checksum */
    CU_ASSERT_EQUAL(rbuf[1 + strlen(str) + 2], (char) END_BYTE);

    smp_serial_frame_deinit(&ctx);

    test_teardown(&tctx);
}

static void test_smp_serial_frame_send(void)
{
    TestCtx tctx;
    SmpSerialFrameContext ctx;
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
    uint8_t big_payload[SMP_SERIAL_FRAME_MAX_FRAME_SIZE + 1] = { 0, };
    uint8_t rbuf[64];
    ssize_t rbytes;
    size_t i;
    int ret;

    test_setup(&tctx);
    ret = smp_serial_frame_init(&ctx, FIFO_PATH, &simple_cbs, NULL);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    /* make sure big payload is rejected */
    ret = smp_serial_frame_send(&ctx, big_payload, sizeof(big_payload));
    CU_ASSERT_EQUAL(ret, -ENOMEM);

    /* make sure payload with magic bytes are handled correctly */
    ret = smp_serial_frame_send(&ctx, payload, sizeof(payload));
    CU_ASSERT_EQUAL(ret, 0);

    /* we should read:
     * START_BYTE | (escaped payload = sizeof(payload) + 7) | CRC | END_BYTE */
    rbytes = read(tctx.fd, rbuf, sizeof(rbuf));
    CU_ASSERT_EQUAL(rbytes, sizeof(expected_frame));

    for (i = 0; i < sizeof(expected_frame); i++) {
        if (rbuf[i] != expected_frame[i]) {
            CU_FAIL("frame mismatch");
            break;
        }
    }

    smp_serial_frame_deinit(&ctx);
    test_teardown(&tctx);
}

static void test_smp_serial_frame_send_magic_crc(void)
{
    TestCtx tctx;
    SmpSerialFrameContext ctx;
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
    int ret;

    test_setup(&tctx);
    ret = smp_serial_frame_init(&ctx, FIFO_PATH, &simple_cbs, NULL);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    for (i = 0; i < 3; i++) {
        uint8_t rbuf[64];
        ssize_t rbytes;
        ssize_t j;

        ret = smp_serial_frame_send(&ctx, payload[i], sizeof(payload[i]));
        CU_ASSERT_EQUAL(ret, 0);

        rbytes = read(tctx.fd, rbuf, sizeof(rbuf));
        CU_ASSERT_EQUAL(rbytes, sizeof(expected_frame[i]));

        for (j = 0; j < rbytes; j++) {
            if (rbuf[j] != expected_frame[i][j]) {
                CU_FAIL("frame mismatch");
                break;
            }
        }
    }

    smp_serial_frame_deinit(&ctx);
    test_teardown(&tctx);
}

typedef enum
{
    NONE,
    PAYLOAD_WITH_MAGIC_BYTES,
    START_WITHOUT_END,
    BAD_CRC,
    FRAME_TOO_BIG,
    FRAME_TOO_BIG_ESC,
    CRC_ESCAPED,
    FRAMES_AND_GARBAGE,
    START_END,
} TestSerialFrameRecvTestCase;

static uint8_t test_smp_serial_frame_recv_payload1[] = {
    START_BYTE, 0x45, 0x23, 0x04, 0x00, ESC_BYTE, END_BYTE, END_BYTE,
    0x33, 0x44, ESC_BYTE, ESC_BYTE, START_BYTE, 0x42
};

static uint8_t test_smp_serial_frame_recv_start_without_end_frame[] = {
    START_BYTE, 0x43, 0x23, START_BYTE, 0x22, 0x33, 0x32, 0x23, END_BYTE
};

static uint8_t test_smp_serial_frame_recv_bad_crc[] = {
    START_BYTE, 0x42, 0x33, 0x00, END_BYTE
};

static uint8_t
test_smp_serial_frame_recv_too_big[SMP_SERIAL_FRAME_MAX_FRAME_SIZE + 10] = {
    START_BYTE, 0x00,
};

static uint8_t
test_smp_serial_frame_recv_too_big_esc[SMP_SERIAL_FRAME_MAX_FRAME_SIZE + 10] = {
    START_BYTE, 0x00,
};

static uint8_t
test_smp_serial_frame_recv_crc_escaped[] = {
    START_BYTE, ESC_BYTE, START_BYTE, ESC_BYTE, START_BYTE, END_BYTE,
    START_BYTE, ESC_BYTE, END_BYTE, ESC_BYTE, END_BYTE, END_BYTE,
    START_BYTE, ESC_BYTE, ESC_BYTE, ESC_BYTE, ESC_BYTE, END_BYTE,
};

static uint8_t test_smp_serial_frame_recv_frames_and_garbage[] = {
    /* some garbage first */
    0x33, 0x22, 0x01, 0x0a, END_BYTE, ESC_BYTE,

    /* now the first frame */
    START_BYTE, 0x12, 0x4e, 0x1f, 0xb0, 0x00, 0x33, 0xc0, END_BYTE,

    /* now some garbage */
    0x19, 0xaf, 0x43, 0x92, 0x09,

    /* the second frame */
    START_BYTE, 0x12, 0x4e, 0x1f, 0xb0, 0x00, 0x33, 0xc0, END_BYTE
};

static uint8_t test_smp_serial_frame_recv_start_end[] = {
    /* just a start byte followed by an end byte which causes an underflow
     * on framesize variable computation and causes an out of bound read
     * when computing checksum */
    START_BYTE, END_BYTE
};

/* make it static to check address in callback */
static TestSerialFrameRecvTestCase test_smp_serial_frame_recv_test_case;

static int test_smp_serial_frame_recv_new_frame_called;

static void test_smp_serial_frame_recv_new_frame(uint8_t *frame, size_t size,
        void *userdata)
{
    size_t i;

    CU_ASSERT_EQUAL(userdata, &test_smp_serial_frame_recv_test_case);

    test_smp_serial_frame_recv_new_frame_called++;

    switch (test_smp_serial_frame_recv_test_case) {
        case PAYLOAD_WITH_MAGIC_BYTES:
            CU_ASSERT_EQUAL(size, sizeof(test_smp_serial_frame_recv_payload1));
            for (i = 0; i < size; i++) {
                if (frame[i] != test_smp_serial_frame_recv_payload1[i]) {
                    CU_FAIL("frame mismatch");
                    break;
                }
            }
            break;

        case START_WITHOUT_END:
            /* we should have the second frame, ie: 0x22, 0x33, 0x32 */
            CU_ASSERT_EQUAL(size, 3);
            CU_ASSERT_EQUAL(frame[0], 0x22);
            CU_ASSERT_EQUAL(frame[1], 0x33);
            CU_ASSERT_EQUAL(frame[2], 0x32);
            break;
        case CRC_ESCAPED:
            CU_ASSERT_EQUAL(size, 1);
            switch (test_smp_serial_frame_recv_new_frame_called) {
                case 1:
                    CU_ASSERT_EQUAL(frame[0], START_BYTE);
                    break;
                case 2:
                    CU_ASSERT_EQUAL(frame[0], END_BYTE);
                    break;
                case 3:
                    CU_ASSERT_EQUAL(frame[0], ESC_BYTE);
                    break;
                default:
                    break;
            }
            break;
        case FRAMES_AND_GARBAGE:
            switch (test_smp_serial_frame_recv_new_frame_called) {
                case 1:
                case 2:
                     CU_ASSERT_EQUAL(frame[0], 0x12);
                     CU_ASSERT_EQUAL(frame[1], 0x4e);
                     CU_ASSERT_EQUAL(frame[2], 0x1f);
                     CU_ASSERT_EQUAL(frame[3], 0xb0);
                     CU_ASSERT_EQUAL(frame[4], 0x00);
                     CU_ASSERT_EQUAL(frame[5], 0x33);
                    break;
                default:
                    CU_FAIL("not reached");
                    break;
            }
            break;
        default:
            break;
    }
}

static int test_smp_serial_frame_recv_error_called;

static void test_smp_serial_frame_recv_error(SmpSerialFrameError error, void *userdata)
{
    CU_ASSERT_EQUAL(userdata, &test_smp_serial_frame_recv_test_case);

    test_smp_serial_frame_recv_error_called = 1;
    switch (test_smp_serial_frame_recv_test_case) {
        case START_END:
        case START_WITHOUT_END:
            CU_ASSERT_EQUAL(error, SMP_SERIAL_FRAME_ERROR_CORRUPTED);
            break;
        case FRAME_TOO_BIG:
        case FRAME_TOO_BIG_ESC:
            CU_ASSERT_EQUAL(error, SMP_SERIAL_FRAME_ERROR_PAYLOAD_TOO_BIG);
            break;
        default:
            break;
    }
}

static void test_smp_serial_frame_terminate_frame(TestCtx *tctx,
        SmpSerialFrameContext *ctx)
{
    uint8_t byte = END_BYTE;
    int ret;

    test_smp_serial_frame_recv_test_case = NONE;
    ret = write(tctx->fd, &byte, 1);
    CU_ASSERT_EQUAL_FATAL(ret, 1);
    smp_serial_frame_process_recv_fd(ctx);
}

static void test_smp_serial_frame_recv(void)
{
    TestCtx tctx;
    SmpSerialFrameContext ctx;
    SmpSerialFrameDecoderCallbacks cbs = {
        .new_frame = test_smp_serial_frame_recv_new_frame,
        .error = test_smp_serial_frame_recv_error,
    };
    int ret;

    test_setup(&tctx);
    ret = smp_serial_frame_init(&ctx, FIFO_PATH, &cbs,
            &test_smp_serial_frame_recv_test_case);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    /* send a payload and make sure we got it */
    ret = smp_serial_frame_send(&ctx, test_smp_serial_frame_recv_payload1,
            sizeof(test_smp_serial_frame_recv_payload1));
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = PAYLOAD_WITH_MAGIC_BYTES;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_TRUE(test_smp_serial_frame_recv_new_frame_called);
    CU_ASSERT_FALSE(test_smp_serial_frame_recv_error_called);

    /* send a payload with two start byte with no end marker between */
    ret = write(tctx.fd, test_smp_serial_frame_recv_start_without_end_frame,
            sizeof(test_smp_serial_frame_recv_start_without_end_frame));
    CU_ASSERT_EQUAL_FATAL(ret,
            sizeof(test_smp_serial_frame_recv_start_without_end_frame));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = START_WITHOUT_END;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_TRUE(test_smp_serial_frame_recv_new_frame_called);
    CU_ASSERT_TRUE(test_smp_serial_frame_recv_error_called);

    /* send a payload with a bad crc */
    ret = write(tctx.fd, test_smp_serial_frame_recv_bad_crc,
            sizeof(test_smp_serial_frame_recv_bad_crc));
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(test_smp_serial_frame_recv_bad_crc));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = BAD_CRC;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_FALSE(test_smp_serial_frame_recv_new_frame_called);
    CU_ASSERT_TRUE(test_smp_serial_frame_recv_error_called);

    /* frame too big */
    ret = write(tctx.fd, test_smp_serial_frame_recv_too_big,
            sizeof(test_smp_serial_frame_recv_too_big));
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(test_smp_serial_frame_recv_too_big));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = FRAME_TOO_BIG;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_FALSE(test_smp_serial_frame_recv_new_frame_called);
    CU_ASSERT_TRUE(test_smp_serial_frame_recv_error_called);

    /* send an END_BYTE to terminate previous test frame */
    test_smp_serial_frame_terminate_frame(&tctx, &ctx);

    /* frame too big with escape char at the end */
    test_smp_serial_frame_recv_too_big_esc[SMP_SERIAL_FRAME_MAX_FRAME_SIZE] =
        ESC_BYTE;
    ret = write(tctx.fd, test_smp_serial_frame_recv_too_big_esc,
            sizeof(test_smp_serial_frame_recv_too_big_esc));
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(test_smp_serial_frame_recv_too_big_esc));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = FRAME_TOO_BIG_ESC;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_FALSE(test_smp_serial_frame_recv_new_frame_called);
    CU_ASSERT_TRUE(test_smp_serial_frame_recv_error_called);

    /* send an END_BYTE to terminate any previous test frame */
    test_smp_serial_frame_terminate_frame(&tctx, &ctx);

    ret = write(tctx.fd, test_smp_serial_frame_recv_crc_escaped,
            sizeof(test_smp_serial_frame_recv_crc_escaped));
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(test_smp_serial_frame_recv_crc_escaped));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = CRC_ESCAPED;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(test_smp_serial_frame_recv_new_frame_called, 3);
    CU_ASSERT_FALSE(test_smp_serial_frame_recv_error_called);

    /* frame and garbage */
    ret = write(tctx.fd, test_smp_serial_frame_recv_frames_and_garbage,
            sizeof(test_smp_serial_frame_recv_frames_and_garbage));
    CU_ASSERT_EQUAL_FATAL(ret,
            sizeof(test_smp_serial_frame_recv_frames_and_garbage));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = FRAMES_AND_GARBAGE;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(test_smp_serial_frame_recv_new_frame_called, 2);
    CU_ASSERT_FALSE(test_smp_serial_frame_recv_error_called);

    /* Empty frame without CRC (just START and STOP) */
    ret = write(tctx.fd, test_smp_serial_frame_recv_start_end,
            sizeof(test_smp_serial_frame_recv_start_end));
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(test_smp_serial_frame_recv_start_end));

    test_smp_serial_frame_recv_new_frame_called = 0;
    test_smp_serial_frame_recv_error_called = 0;
    test_smp_serial_frame_recv_test_case = START_END;
    ret = smp_serial_frame_process_recv_fd(&ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_EQUAL(test_smp_serial_frame_recv_new_frame_called, 0);
    CU_ASSERT_EQUAL(test_smp_serial_frame_recv_error_called, 1);

    smp_serial_frame_deinit(&ctx);
    test_teardown(&tctx);
}

typedef struct
{
    const char *name;
    CU_TestFunc func;
} Test;

static Test tests[] = {
    { "test_smp_serial_frame_init", test_smp_serial_frame_init },
    { "test_smp_serial_frame_send_simple", test_smp_serial_frame_send_simple },
    { "test_smp_serial_frame_send", test_smp_serial_frame_send },
    { "test_smp_serial_frame_send_magic_crc",
        test_smp_serial_frame_send_magic_crc },
    { "test_smp_serial_frame_recv", test_smp_serial_frame_recv },
    { NULL, NULL }
};

CU_ErrorCode serial_frame_test_register(void)
{
    CU_pSuite suite = NULL;
    Test *t;

    suite = CU_add_suite("Serial Frame Suite", NULL, NULL);
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
