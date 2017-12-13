// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIB_H
#define LIB_H

#include "compiler.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <linux/version.h>

#define MIN(a,b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b;})
#define MAX(a,b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b;})
#define IS_POWER_OF_TWO(x) ({__typeof__(x) _x = (x); _x && !(_x & (_x - 1));})
#define DIV_ROUND_UP(a,b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); (_a + _b - 1) / _b;})

#define print_errno(message) fprintf(stderr, "[%s (%s:%i)] %s: %s (%i)\n", __func__, __FILE__, __LINE__, (message), strerror(errno), errno)

#define CHECK_LINUX_VERSION(a,b,c) (LINUX_VERSION_CODE >= KERNEL_VERSION(a,b,c))

static always_inline void timespec_diff(const struct timespec* const start, const struct timespec* const end, struct timespec* const diff)
{
    diff->tv_sec = end->tv_sec - start->tv_sec;
    diff->tv_nsec = end->tv_nsec - start->tv_nsec;

    if (diff->tv_nsec < 0)
    {
        diff->tv_sec--;
        diff->tv_nsec += 1000000000;
    }
}

#endif
