// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include <stdint.h>
#include "dpdk_lib.h"
#include "aliases.h"

// uint8_t EXTERNIMPL2(Random,read,u8)(EXTERNTYPE1(Random,u8)* xtrn, SHORT_STDPARAMS) {
//     // TODO temporary implementation
//     return 0;
// }

// uint16_t EXTERNIMPL2(Random,read,u16)(EXTERNTYPE1(Random,u16)* xtrn, SHORT_STDPARAMS) {
//     // TODO temporary implementation
//     return 0;
// }

// uint32_t EXTERNIMPL2(Random,read,u32)(EXTERNTYPE1(Random,u32)* xtrn, SHORT_STDPARAMS) {
//     // TODO temporary implementation
//     return 0;
// }

uint32_t EXTERNIMPL1(Random,read)(SHORT_STDPARAMS) {
    // TODO temporary implementation
    return 0;
}


// TODO this should not be here
uint32_t EXTERNCALL0(Random,read)(SHORT_STDPARAMS) {
    // TODO temporary implementation
    return 0;
}

