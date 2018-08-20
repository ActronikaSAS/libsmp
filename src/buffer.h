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

#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>

#include "libsmp.h"

struct SmpBuffer
{
    uint8_t *data;     /* pointer to the data */
    size_t maxsize;    /* size of the memory pointed by data */
    SmpBufferFreeFunc free;
    bool statically_allocated;
};

SmpBuffer *smp_buffer_new_allocate(size_t size);
void smp_buffer_free(SmpBuffer *buffer);

#endif
