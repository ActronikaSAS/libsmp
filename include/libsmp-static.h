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

#ifndef LIBSMP_STATIC_H
#define LIBSMP_STATIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SmpStaticBuffer SmpStaticBuffer;
typedef struct SmpStaticContext SmpStaticContext;
typedef struct SmpStaticMessage SmpStaticMessage;
typedef struct SmpStaticSerialProtocolDecoder SmpStaticSerialProtocolDecoder;

struct SmpStaticBuffer {
    uint8_t data[16];
};

struct SmpStaticContext {
    uint8_t data[36];
};

struct SmpStaticMessage {
    uint8_t data[16];
};

struct SmpStaticSerialProtocolDecoder {
    uint8_t data[24];
};

#ifdef __cplusplus
}
#endif

#endif