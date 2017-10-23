/* libsmp
 * Copyright (C) 2017 Actronika SAS
 *     Author: Aurélien Zanelli <aurelien.zanelli@actronika.com>
 */

#ifndef LIBSMP_PRIVATE_AVR_H
#define LIBSMP_PRIVATE_AVR_H

#ifdef __cplusplus
extern "C" {
#endif

#define ENOENT      2       /* No such file or directory */
#define E2BIG       7       /* Argument list too long */
#define EBADF       9       /* Bad file number */
#define EAGAIN      11      /* Try again */
#define ENOMEM      12      /* Out of memory */
#define EFAULT      14      /* Bad address */
#define EBUSY       16      /* Device or resource busy */
#define EINVAL      22      /* Invalid argument */
#define ENOSYS      38      /* Invalid system call number */
#define EWOULDBLOCK EAGAIN  /* Operation would block */
#define EBADMSG     74      /* Not a data message */

static inline int smp_fd_is_valid(int fd)
{
    return (fd >= 0) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif
