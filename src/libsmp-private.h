/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef LIBSMP_PRIVATE_H
#define LIBSMP_PRIVATE_H

#include "serial-device.h"

#ifdef __AVR
#include "libsmp-private-avr.h"
#else
#include "libsmp-private-posix.h"
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

#endif
