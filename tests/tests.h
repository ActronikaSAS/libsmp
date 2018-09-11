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

#ifndef TESTS_H
#define TESTS_H

#include <CUnit/CUnit.h>

#define DEFINE_TEST(func) { #func, func }

#ifndef SMP_N_ELEMENTS
#define SMP_N_ELEMENTS(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

CU_ErrorCode context_test_register(void);
CU_ErrorCode serial_frame_test_register(void);
CU_ErrorCode serial_protocol_test_register(void);
CU_ErrorCode message_test_register(void);

#endif
