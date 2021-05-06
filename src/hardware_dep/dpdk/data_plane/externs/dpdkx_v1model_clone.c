// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_v1model.h"
#include "util_packet.h"
#include "util_debug.h"

#include "dpdk_lib.h"
#include "stateful_memory.h"

#include <rte_ip.h>

#ifdef T4P4S_TYPE_enum_CloneType
    void clone(enum_CloneType_t type, uint32_t session, SHORT_STDPARAMS) {
        debug("    : Executing clone\n");
    }

    void clone3(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3\n");
    }

    void clone3__u16(enum_CloneType_t type, uint16_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u16\n");
    }

    void clone3__u32(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u32\n");
    }

    void clone3__u8(enum_CloneType_t type, uint8_t session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u8\n");
    }

    void clone3__u16s(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u16s\n");
    }

    void clone3__u32s(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u32s\n");
    }

    void clone3__u8s(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u8s\n");
    }

    void clone3__u16ss(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS) {
        debug("    : Executing clone3__u16ss\n");
    }
#endif
