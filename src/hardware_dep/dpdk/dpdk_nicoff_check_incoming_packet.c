// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"
#include "dpdk_nicoff.h"
#include "util_debug.h"

extern int get_packet_idx(LCPARAMS);
extern bool is_final_section(const char*const text);
extern bool starts_with_fmt_char(const char*const text);


bool check_nybble(char nybble, int idx, int segment_idx, const char*const text, LCPARAMS) {
    const char*const valid_nybble_chars = "0123456789abcdefABCDEF";
    if (strchr(valid_nybble_chars, nybble) == NULL) {
        debug("   " T4LIT(!!,error) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) ": " T4LIT(invalid character '%c',error) " found at position " T4LIT(#%d) " in test case header segment " T4LIT(#%d) ": " T4LIT(%.*s,bytes) T4LIT(%c,error) T4LIT(%s,bytes) "\n",
              get_packet_idx(LCPARAMS_IN), rte_lcore_id(), nybble, idx, segment_idx, idx-1, text, text[idx-1], text + idx);
        return false;
    }
    return true;
}

bool is_valid_fake_packet_segment(const char* text, int segment_idx, LCPARAMS) {
    const char* ptr = text;
    int idx = 1;
    while (*ptr != '\0') {
        if (starts_with_fmt_char(ptr)) {
            ++ptr;
            ++idx;
            continue;
        }

        bool is_ok_nybble1 = check_nybble(ptr[0], idx, segment_idx, text, LCPARAMS_IN);
        if (!is_ok_nybble1)   return false;

        while (ptr[1] == '\0') {
            debug("   " T4LIT(!!,error) " " T4LIT(Packet #%d,packet) "@" T4LIT(core%d,core) ": test case header segment " T4LIT(#%d) " at position " T4LIT(#%d) " " T4LIT(ends in unfinished byte,error) ": " T4LIT(%.*s,bytes) T4LIT(%c,error)"\n",
                  get_packet_idx(LCPARAMS_IN), rte_lcore_id(), segment_idx, idx, idx-1, text, text[idx-1]);
            return false;
        }

        bool is_ok_nybble2 = check_nybble(ptr[1], idx + 1, segment_idx, text, LCPARAMS_IN);
        if (!is_ok_nybble2)   return false;

        ptr += 2;
        idx += 2;
    }

    return true;
}

bool is_valid_fake_packet(const char* texts[MAX_SECTION_COUNT], LCPARAMS) {
    int segment_idx = 1;
    for (; !is_final_section(*texts); ++texts) {
        const char* text = *texts;
        if (!is_valid_fake_packet_segment(text, segment_idx, LCPARAMS_IN)) {
            return false;
        }
        ++segment_idx;
    }

    return true;
}
