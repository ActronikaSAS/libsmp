/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "tests.h"

int main(int argc, char *argv[])
{
    CU_ErrorCode ret;
    int fail = 0;

    ret = CU_initialize_registry();
    if (ret != CUE_SUCCESS)
        return CU_get_error();

    ret = serial_frame_test_register();
    if (ret != CUE_SUCCESS)
        return ret;

    /* Run tests using Basic interface */
    CU_basic_run_tests();

    fail = CU_get_number_of_tests_failed();

    CU_cleanup_registry();

    return fail;
}
