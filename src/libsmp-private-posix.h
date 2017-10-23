/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef LIBSMP_PRIVATE_POSIX_H
#define LIBSMP_PRIVATE_POSIX_H

#ifdef __cplusplus
extern "C" {
#endif

static inline int smp_fd_is_valid(int fd)
{
    return (fd >= 0) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif
