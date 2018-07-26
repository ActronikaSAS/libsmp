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

#ifndef LIBSMP_PRIVATE_AVR_H
#define LIBSMP_PRIVATE_AVR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ENOENT
#define ENOENT      2       /* No such file or directory */
#endif
#ifndef E2BIG
#define E2BIG       7       /* Argument list too long */
#endif
#ifndef EBADF
#define EBADF       9       /* Bad file number */
#endif
#ifndef EAGAIN
#define EAGAIN      11      /* Try again */
#endif
#ifndef ENOMEM
#define ENOMEM      12      /* Out of memory */
#endif
#ifndef EFAULT
#define EFAULT      14      /* Bad address */
#endif
#ifndef EBUSY
#define EBUSY       16      /* Device or resource busy */
#endif
#ifndef EINVAL
#define EINVAL      22      /* Invalid argument */
#endif
#ifndef ENOSYS
#define ENOSYS      38      /* Invalid system call number */
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN  /* Operation would block */
#endif
#ifndef EBADMSG
#define EBADMSG     74      /* Not a data message */
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT 110    /* Connection timed out */
#endif

#ifdef __cplusplus
}
#endif

#endif
