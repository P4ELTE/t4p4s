// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "gen_defs.h"

#ifdef T4P4S_FAKERANDOM
    #include <time.h>
#endif


void EXTERNIMPL0(random)(uint32_t* out, uint32_t min, uint32_t max, SHORT_STDPARAMS) {
    #ifdef T4P4S_FAKERANDOM
        *out = 0x12345678;
        bool is_fake = true;
    #else
        *out = ((uint32_t)rand() % (max - min + 1)) + min;
        bool is_fake = false;
    #endif

    debug("    : Generated " T4LIT(%db) "%s random number in the range " T4LIT(%d) ".." T4LIT(%d) ": " T4LIT(%d) " (0x" T4LIT(%x) ")\n",
          32, is_fake ? " fake" : "", min, max, *out, *out);
}


void EXTERNIMPL1(random,u8)(uint8_t* out, uint8_t min, uint8_t max, SHORT_STDPARAMS) {
    uint32_t out32;
    EXTERNIMPL0(random)(&out32, min, max, SHORT_STDPARAMS_IN);
    *out = (uint8_t)out32;
}


void EXTERNIMPL1(random,u16)(uint16_t* out, uint16_t min, uint16_t max, SHORT_STDPARAMS) {
    uint32_t out32;
    EXTERNIMPL0(random)(&out32, min, max, SHORT_STDPARAMS_IN);
    *out = (uint16_t)out32;
}


void EXTERNIMPL1(random,u32)(uint32_t* out32, uint32_t min, uint32_t max, SHORT_STDPARAMS) {
    EXTERNIMPL0(random)(out32, min, max, SHORT_STDPARAMS_IN);
}
