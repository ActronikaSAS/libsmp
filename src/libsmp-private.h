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

#endif
