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

    ret = message_test_register();
    if (ret != CUE_SUCCESS)
        return ret;

    /* Run tests using Basic interface */
    CU_basic_run_tests();

    fail = CU_get_number_of_tests_failed();

    CU_cleanup_registry();

    return fail;
}
