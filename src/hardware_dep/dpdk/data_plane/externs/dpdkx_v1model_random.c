// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "gen_defs.h"

#ifdef T4P4S_FAKERANDOM
    #include <time.h>
#endif

void random__u8(uint8_t* out, uint8_t min, uint8_t max, SHORT_STDPARAMS) {
    #ifdef T4P4S_FAKERANDOM
        *out = 42;
        bool is_fake = true;
    #else
        *out = ((uint8_t)rand() % (max - min + 1)) + min;
        bool is_fake = false;
    #endif

    debug("    : Generated " T4LIT(%dB) "%s random number in the range " T4LIT(%d) ".." T4LIT(%d) ": " T4LIT(%d) " (0x" T4LIT(%x) ")\n",
          8, is_fake ? " fake" : "", min, max, *out, *out);
}

void random__u16(uint16_t* out, uint16_t min, uint16_t max, SHORT_STDPARAMS) {
    #ifdef T4P4S_FAKERANDOM
        *out = 0x1234;
        bool is_fake = true;
    #else
        *out = ((uint16_t)rand() % (max - min + 1)) + min;
        bool is_fake = false;
    #endif

    debug("    : Generated " T4LIT(%dB) "%s random number in the range " T4LIT(%d) ".." T4LIT(%d) ": " T4LIT(%d) " (0x" T4LIT(%x) ")\n",
          16, is_fake ? " fake" : "", min, max, *out, *out);
}

void random__u32(uint32_t* out, uint32_t min, uint32_t max, SHORT_STDPARAMS) {
    #ifdef T4P4S_FAKERANDOM
        *out = 0x12345678;
        bool is_fake = true;
    #else
        *out = ((uint32_t)rand() % (max - min + 1)) + min;
        bool is_fake = false;
    #endif

    debug("    : Generated " T4LIT(%dB) "%s random number in the range " T4LIT(%d) ".." T4LIT(%d) ": " T4LIT(%d) " (0x" T4LIT(%x) ")\n",
          32, is_fake ? " fake" : "", min, max, *out, *out);
}

void random_fun(uint32_t* out, int min, int max, SHORT_STDPARAMS) {
    random__u32(out, min, max, SHORT_STDPARAMS_IN);
}
