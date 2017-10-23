/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef LIBSMP_PRIVATE_H
#define LIBSMP_PRIVATE_H

#include "serial-device.h"

#include "libsmp-private-posix.h"

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

#endif
