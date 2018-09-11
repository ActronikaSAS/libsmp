/* libsmp
 * Copyright (C) 2018 Actronika SAS
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
 */

#include "libsmp.h"
#include "libsmp-private.h"

#define ERR_STR_ARRAY_SIZE ((-SMP_ERROR_BAD_TYPE) + 1)
static const char * const errstr[ERR_STR_ARRAY_SIZE] = {
    "none",
    "invalid parameter",
    "no more memory",
    "no such device",
    "entity not found",
    "device busy",
    "permission denied",
    "bad file descriptor",
    "operation not supported",
    "operation would block",
    "io error",
    "entity already exist",
    "too big",
    "operation timed out",
    "overflow",
    "bad message",
    "bad field type",
};

/**
 * \ingroup error
 * Return a string describing the SmpError
 *
 * @param[in] error the SmpError
 *
 * @return a string describing the error.
 */
const char *smp_error_to_string(SmpError error)
{
    if (error <= 0 && error >= -ERR_STR_ARRAY_SIZE)
        return errstr[-error];
    else if (error == SMP_ERROR_OTHER)
        return "other error";
    else
        return "unknown";
}
