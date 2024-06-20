// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include <stdbool.h>
#include <stdint.h>

#include "dataplane.h"
#include "util_debug.h"

#include "dpdk_lib_byteorder.h"

#ifdef T4P4S_DEBUG
    #include <assert.h>
    #include "util_debug.h"
#endif

int INCOMPLETE_BYTECOUNT(bitfield_handle_t fd) {
    return (fd.bitcount - 1) / 8;
}
