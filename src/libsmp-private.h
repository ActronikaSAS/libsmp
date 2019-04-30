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

#ifndef LIBSMP_PRIVATE_H
#define LIBSMP_PRIVATE_H

#include <stddef.h>
#include <stdint.h>

#ifndef __AVR
#include <sys/types.h>
#endif

#include "libsmp.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __AVR
/* we don't have sys/types.h in avr-libc so define ssize_t ourself */
typedef long ssize_t;
#elif defined(_WIN32) || defined(_WIN64)
typedef signed __int64 ssize_t;
#endif

#define return_if_fail(expr) \
    do { \
        if (!(expr)) \
            return; \
    } while (0);

#define return_val_if_fail(expr, val) \
    do { \
        if (!(expr)) \
            return val; \
    } while (0);

#define SMP_N_ELEMENTS(arr) (sizeof((arr))/sizeof((arr)[0]))

#define smp_new(Type) calloc(1, sizeof(Type))

#define SMP_DO_CONCAT(a, b) a ## b
#define SMP_CONCAT(a, b) SMP_DO_CONCAT(a,b)
#define SMP_STATIC_ASSERT(cond) \
    typedef char SMP_CONCAT(static_assertion, __LINE__)[cond ? 1 : -1];

struct SmpMessage
{
    /** The message id */
    uint32_t msgid;

    SmpValue values[SMP_MESSAGE_MAX_VALUES];
    SmpValue *pvalues;
    size_t capacity;

    bool statically_allocated;
};

int smp_message_build_from_buffer(SmpMessage *msg, const uint8_t *buffer,
        size_t size);

#ifdef __cplusplus
}
#endif

#endif
