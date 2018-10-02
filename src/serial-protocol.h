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

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#ifndef SMP_ENABLE_STATIC_API
#define SMP_ENABLE_STATIC_API
#endif

#include "libsmp.h"
#include "libsmp-private.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Decoder API */
typedef enum
{
    SMP_SERIAL_PROTOCOL_DECODER_STATE_WAIT_HEADER,
    SMP_SERIAL_PROTOCOL_DECODER_STATE_IN_FRAME,
    SMP_SERIAL_PROTOCOL_DECODER_STATE_IN_FRAME_ESC,
} SmpSerialProtocolDecoderState;

struct SmpSerialProtocolDecoder
{
    SmpSerialProtocolDecoderState state;

    uint8_t *buf;
    size_t bufsize;
    size_t offset;
    size_t maxsize;

    bool statically_allocated;
};

SmpSerialProtocolDecoder *smp_serial_protocol_decoder_new(size_t bufsize);
void smp_serial_protocol_decoder_free(SmpSerialProtocolDecoder * decoder);
int smp_serial_protocol_decoder_process_byte(SmpSerialProtocolDecoder *decoder,
        uint8_t byte, uint8_t **frame, size_t *framesize);
int smp_serial_protocol_decoder_set_maximum_capacity(SmpSerialProtocolDecoder *decoder,
        size_t max);

/* Encoder API */
ssize_t smp_serial_protocol_encode(const uint8_t *inbuf, size_t insize,
        uint8_t **outbuf, size_t outsize);

#ifdef __cplusplus
}
#endif

#endif
