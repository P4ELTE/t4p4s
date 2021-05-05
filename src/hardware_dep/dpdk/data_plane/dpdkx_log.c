// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

void log_msg(const char* msg, SHORT_STDPARAMS) {
    debug("    : " T4LIT(Logged,status) ": %s\n", msg);
}
