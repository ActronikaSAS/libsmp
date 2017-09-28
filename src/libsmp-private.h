/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef LIBSMP_PRIVATE_H
#define LIBSMP_PRIVATE_H

#define return_val_if_fail(expr, val) \
    do { \
        if (!(expr)) \
            return val; \
    } while (0);

#endif
