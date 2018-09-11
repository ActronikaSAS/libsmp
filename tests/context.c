#include <CUnit/CUnit.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#define SMP_ENABLE_STATIC_API
#include <libsmp.h>

#include "tests.h"

#define FIFO_PATH "/tmp/smp-test-fifo"

#define START_BYTE 0x10
#define END_BYTE 0xFF

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

static void on_new_message_simple(SmpContext *ctx, SmpMessage *msg,
        void *userdata)
{
}

static void on_error_simple(SmpContext *ctx, SmpError error, void *userdata)
{
}

static const SmpEventCallbacks simple_cbs = {
    .new_message_cb = on_new_message_simple,
    .error_cb = on_error_simple
};

static void test_smp_context_new(void)
{
    SmpContext *ctx;
    int foo = 42;

    /* calling with invalid args shall fails */
    ctx = smp_context_new(NULL, NULL);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new(&simple_cbs, NULL);
    CU_ASSERT_PTR_NOT_NULL(ctx);
    smp_context_free(ctx);

    ctx = smp_context_new(&simple_cbs, &foo);
    CU_ASSERT_PTR_NOT_NULL(ctx);
    smp_context_free(ctx);
}

static void test_smp_context_open(void)
{
    TestCtx tctx;
    SmpContext *ctx;

    test_setup(&tctx);

    ctx = smp_context_new(&simple_cbs, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);

    /* calling with invalid args should fail */
    CU_ASSERT_EQUAL(smp_context_open(NULL, NULL), SMP_ERROR_INVALID_PARAM);
    CU_ASSERT_EQUAL(smp_context_open(ctx, NULL), SMP_ERROR_INVALID_PARAM);
    CU_ASSERT_EQUAL(smp_context_open(NULL, FIFO_PATH), SMP_ERROR_INVALID_PARAM);

    /* opening a non-existant device should fail */
    CU_ASSERT_EQUAL(smp_context_open(ctx, "/sfnjiejdfeifsd"),
            SMP_ERROR_NO_DEVICE);

    /* this should be ok */
    CU_ASSERT_EQUAL(smp_context_open(ctx, FIFO_PATH), 0);

    /* calling open on an already opened context shall fail */
    CU_ASSERT_EQUAL(smp_context_open(ctx, FIFO_PATH), SMP_ERROR_BUSY);

    /* closing and opening should be fine */
    smp_context_close(ctx);
    CU_ASSERT_EQUAL(smp_context_open(ctx, FIFO_PATH), 0);

    smp_context_close(ctx);
    smp_context_free(ctx);
    test_teardown(&tctx);
}

static void test_smp_context_send_message(void)
{
    TestCtx tctx;
    SmpContext *ctx;
    SmpMessage msg;
    int ret;

    test_setup(&tctx);
    ctx = smp_context_new(&simple_cbs, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);

    smp_message_init(&msg, 1);

    /* sending a message when the context is not opened should fail */
    CU_ASSERT_EQUAL(smp_context_send_message(ctx, &msg), SMP_ERROR_BAD_FD);

    ret = smp_context_open(ctx, FIFO_PATH);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    /* calling with invalid args should fail */
    CU_ASSERT_EQUAL(smp_context_send_message(NULL, NULL),
            SMP_ERROR_INVALID_PARAM);
    CU_ASSERT_EQUAL(smp_context_send_message(ctx, NULL),
            SMP_ERROR_INVALID_PARAM);
    CU_ASSERT_EQUAL(smp_context_send_message(NULL, &msg),
            SMP_ERROR_INVALID_PARAM);

    /* should be ok */
    CU_ASSERT_EQUAL(smp_context_send_message(ctx, &msg), 0);

    smp_context_close(ctx);
    smp_context_free(ctx);
    test_teardown(&tctx);
}

typedef enum {
    VALID_PAYLOAD,
    CORRUPTED_PAYLOAD,
    STATIC_API_OK,
    STATIC_API_TOO_BIG,
} TestContextReceiveMessageCase;

static TestContextReceiveMessageCase test_smp_context_receive_message_case;
static bool test_smp_context_on_message_called;
static bool test_smp_context_on_error_called;

static void on_new_message(SmpContext *ctx, SmpMessage *msg,
        void *userdata)
{
    int ret;

    CU_ASSERT_EQUAL(userdata, &test_smp_context_receive_message_case);
    test_smp_context_on_message_called = true;

    switch (test_smp_context_receive_message_case) {
        case VALID_PAYLOAD: {
            uint32_t val;

            CU_ASSERT_EQUAL(msg->msgid, 1);
            ret = smp_message_get_uint32(msg, 0, &val);
            CU_ASSERT_EQUAL(ret, 0);
            CU_ASSERT_EQUAL(val, 0xabcdef42);
            break;
        }
        default:
            break;
    }
}

static void on_error(SmpContext *ctx, SmpError error, void *userdata)
{
    CU_ASSERT_EQUAL(userdata, &test_smp_context_receive_message_case);
    test_smp_context_on_error_called = true;

    switch (test_smp_context_receive_message_case) {
        case CORRUPTED_PAYLOAD:
            CU_ASSERT_EQUAL(error, SMP_ERROR_BAD_MESSAGE);
            break;
        case STATIC_API_TOO_BIG:
            CU_ASSERT_EQUAL(error, SMP_ERROR_TOO_BIG);
            break;
        default:
            break;
    }
}

static const SmpEventCallbacks test_cbs = {
    .new_message_cb = on_new_message,
    .error_cb = on_error
};

static void test_smp_context_receive_valid_message(void)
{
    TestCtx tctx;
    SmpContext *ctx;
    SmpMessage msg;
    int ret;

    test_setup(&tctx);
    ctx = smp_context_new(&test_cbs, &test_smp_context_receive_message_case);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);

    CU_ASSERT_EQUAL_FATAL(smp_context_open(ctx, FIFO_PATH), 0);

    test_smp_context_receive_message_case = VALID_PAYLOAD;
    smp_message_init(&msg, 1);
    smp_message_set_uint32(&msg, 0, 0xabcdef42);
    ret = smp_context_send_message(ctx, &msg);
    CU_ASSERT_EQUAL_FATAL(ret, 0);

    test_smp_context_on_message_called = false;
    test_smp_context_on_error_called = false;
    ret = smp_context_process_fd(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_TRUE(test_smp_context_on_message_called);
    CU_ASSERT_FALSE(test_smp_context_on_error_called);

    smp_context_close(ctx);
    smp_context_free(ctx);
    test_teardown(&tctx);
}

static void test_smp_context_receive_corrupted_message(void)
{
    TestCtx tctx;
    SmpContext *ctx;
    uint8_t payload[] = { START_BYTE, 0x42, 0x33, 0x00, END_BYTE };
    int ret;

    /* we use a bad CRC to corrupt message */
    test_setup(&tctx);
    ctx = smp_context_new(&test_cbs, &test_smp_context_receive_message_case);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);

    CU_ASSERT_EQUAL_FATAL(smp_context_open(ctx, FIFO_PATH), 0);

    test_smp_context_receive_message_case = CORRUPTED_PAYLOAD;
    ret = write(tctx.fd, payload, sizeof(payload));
    CU_ASSERT_EQUAL_FATAL(ret, sizeof(payload));

    test_smp_context_on_message_called = false;
    test_smp_context_on_error_called = false;
    ret = smp_context_process_fd(ctx);
    CU_ASSERT_EQUAL(ret, 0);
    CU_ASSERT_FALSE(test_smp_context_on_message_called);
    CU_ASSERT_TRUE(test_smp_context_on_error_called);

    smp_context_close(ctx);
    smp_context_free(ctx);
    test_teardown(&tctx);
}

static void test_smp_context_static_api(void)
{
    TestCtx tctx;
    SmpContext *ctx;
    SmpSerialProtocolDecoder *decoder;
    SmpBuffer *serial_tx;
    SmpBuffer *msg_tx;
    SmpStaticContext sctx;
    SmpStaticSerialProtocolDecoder sdecoder;
    SmpStaticBuffer sserial_tx;
    SmpStaticBuffer smsg_tx;
    SmpStaticMessage smsg_rx;
    SmpValue values_storage[16];
    uint8_t tx_serial_buffer[32];
    uint8_t rx_serial_buffer[32];
    uint8_t msg_buffer[16];
    SmpMessage *msg_rx;
    SmpMessage msg;

    test_setup(&tctx);

    serial_tx = smp_buffer_new_from_static(&sserial_tx, sizeof(sserial_tx),
            tx_serial_buffer, sizeof(tx_serial_buffer), NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(serial_tx);

    msg_tx = smp_buffer_new_from_static(&smsg_tx, sizeof(smsg_tx), msg_buffer,
            sizeof(msg_buffer), NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg_tx);

    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder), rx_serial_buffer, sizeof(rx_serial_buffer));
    CU_ASSERT_PTR_NOT_NULL_FATAL(decoder);

    msg_rx = smp_message_new_from_static(&smsg_rx, sizeof(smsg_rx),
            values_storage, SMP_N_ELEMENTS(values_storage));
    CU_ASSERT_PTR_NOT_NULL_FATAL(decoder);

    /* testing with invalid args should fail */
    ctx = smp_context_new_from_static(NULL, 0, NULL, NULL, NULL, NULL, NULL,
            NULL);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(NULL, sizeof(sctx), &simple_cbs, NULL,
            decoder, serial_tx, msg_tx, msg_rx);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx) - 1, &simple_cbs, NULL,
            decoder, serial_tx, msg_tx, msg_rx);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx), NULL, NULL,
            decoder, serial_tx, msg_tx, msg_rx);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx), &simple_cbs, NULL,
            NULL, serial_tx, msg_tx, msg_rx);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx), &simple_cbs, NULL,
            decoder, NULL, msg_tx, msg_rx);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx), &simple_cbs, NULL,
            decoder, serial_tx, NULL, msg_rx);
    CU_ASSERT_PTR_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx), &simple_cbs, NULL,
            decoder, serial_tx, msg_tx, NULL);
    CU_ASSERT_PTR_NULL(ctx);

    /* should work */
    ctx = smp_context_new_from_static(&sctx, sizeof(sctx), &test_cbs,
            &test_smp_context_receive_message_case, decoder, serial_tx, msg_tx,
            msg_rx);
    CU_ASSERT_PTR_NOT_NULL(ctx);

    ctx = smp_context_new_from_static(&sctx, sizeof(sctx) + 1, &test_cbs,
            &test_smp_context_receive_message_case, decoder, serial_tx, msg_tx,
            msg_rx);
    CU_ASSERT_PTR_NOT_NULL(ctx);

    /* sending a basic message should work */
    CU_ASSERT_EQUAL_FATAL(smp_context_open(ctx, FIFO_PATH), 0);

    smp_message_init(&msg, 1);
    CU_ASSERT_EQUAL(smp_context_send_message(ctx, &msg), 0);

    test_smp_context_receive_message_case = STATIC_API_OK;
    test_smp_context_on_message_called = false;
    test_smp_context_on_error_called = false;
    CU_ASSERT_EQUAL(smp_context_process_fd(ctx), 0);
    CU_ASSERT_TRUE(test_smp_context_on_message_called);
    CU_ASSERT_FALSE(test_smp_context_on_error_called);

    /* reset msg_tx buffer to have a size lesser than the message header size */
    msg_tx = smp_buffer_new_from_static(&smsg_tx, sizeof(smsg_tx), msg_buffer,
            4, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg_tx);

    /* sending a message which exceed the message buffer should fail */
    CU_ASSERT_EQUAL(smp_context_send_message(ctx, &msg), SMP_ERROR_OVERFLOW);

    /* reset msg_tx to a normal size and reduce serial_tx to cause an error */
    msg_tx = smp_buffer_new_from_static(&smsg_tx, sizeof(smsg_tx), msg_buffer,
            sizeof(msg_buffer), NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg_tx);

    serial_tx = smp_buffer_new_from_static(&sserial_tx, sizeof(sserial_tx),
            tx_serial_buffer, 8, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(serial_tx);

    CU_ASSERT_EQUAL(smp_context_send_message(ctx, &msg), SMP_ERROR_OVERFLOW);

    /* reset serial_tx buffer and reduce serial_rx to cause a too big error */
    serial_tx = smp_buffer_new_from_static(&sserial_tx, sizeof(sserial_tx),
            tx_serial_buffer, sizeof(tx_serial_buffer), NULL);

    decoder = smp_serial_protocol_decoder_new_from_static(&sdecoder,
            sizeof(sdecoder), rx_serial_buffer, 8);
    CU_ASSERT_PTR_NOT_NULL_FATAL(decoder);

    CU_ASSERT_EQUAL(smp_context_send_message(ctx, &msg), 0);

    test_smp_context_receive_message_case = STATIC_API_TOO_BIG;
    test_smp_context_on_message_called = false;
    test_smp_context_on_error_called = false;
    CU_ASSERT_EQUAL(smp_context_process_fd(ctx), 0);
    CU_ASSERT_FALSE(test_smp_context_on_message_called);
    CU_ASSERT_TRUE(test_smp_context_on_error_called);

    smp_context_close(ctx);

    test_teardown(&tctx);
}

SMP_DEFINE_STATIC_CONTEXT(test_macro, 32, 64, 128, 16);
static void test_smp_context_static_macro_helper()
{
    SmpContext *ctx;

    ctx = test_macro_create(&simple_cbs, NULL);
    CU_ASSERT_PTR_NOT_NULL(ctx);
}

typedef struct
{
    const char *name;
    CU_TestFunc func;
} Test;

static Test tests[] = {
    DEFINE_TEST(test_smp_context_new),
    DEFINE_TEST(test_smp_context_open),
    DEFINE_TEST(test_smp_context_send_message),
    DEFINE_TEST(test_smp_context_receive_valid_message),
    DEFINE_TEST(test_smp_context_receive_corrupted_message),
    DEFINE_TEST(test_smp_context_static_api),
    DEFINE_TEST(test_smp_context_static_macro_helper),
    { NULL, NULL }
};

CU_ErrorCode context_test_register(void)
{
    CU_pSuite suite = NULL;
    Test *t;

    suite = CU_add_suite("Context Test Suite", NULL, NULL);
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
