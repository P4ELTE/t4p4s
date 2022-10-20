// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include <stdint.h>
#include "dpdk_lib.h"
#include "aliases.h"
#include "stateful_memory_type.h"

#ifdef externdef_Random_u8
    uint8_t EXTERNIMPL2(Random,read,u8)(EXTERNTYPE1(Random,u8)* xtrn, uint8_t idx, SHORT_STDPARAMS) {
        // TODO temporary implementation
        return 0;
    }
#endif

#ifdef externdef_Random_u16
    uint16_t EXTERNIMPL2(Random,read,u16)(EXTERNTYPE1(Random,u16)* xtrn, uint16_t idx, SHORT_STDPARAMS) {
        // TODO temporary implementation
        return 0;
    }
#endif

#ifdef externdef_Random_u32
    uint32_t EXTERNIMPL2(Random,read,u32)(EXTERNTYPE1(Random,u32)* xtrn, uint32_t idx, SHORT_STDPARAMS) {
        // TODO temporary implementation
        return 0;
    }
#endif

uint32_t EXTERNIMPL1(Random,read)(SHORT_STDPARAMS) {
    // TODO temporary implementation
    return 0;
}


uint32_t EXTERNCALL0(Random,read)(SHORT_STDPARAMS) {
    // TODO temporary implementation
    return 0;
}

