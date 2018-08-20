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

#ifndef LIBSMP_CONTEXT_H
#define LIBSMP_CONTEXT_H

#include "libsmp.h"

#include <stdbool.h>

#include "serial-protocol.h"

struct SmpContext
{
    SmpSerialProtocolDecoder *decoder;
    SmpSerialDevice device;

    SmpEventCallbacks cbs;
    void *userdata;

    bool opened;

    bool statically_allocated;
    SmpBuffer *msg_tx;
    SmpBuffer *serial_tx;
};

#endif
