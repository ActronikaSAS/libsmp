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

#ifdef __cplusplus
}
#endif

#endif
