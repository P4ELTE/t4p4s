// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include "actions.h"
#include "util_debug.h"

extern ctrl_plane_backend bg;

bool encountered_digest_without_control_plane = false;

#define STD_DIGEST_RECEIVER_ID 1024

void EXTERNIMPL2(Digest,pack,u8s)(EXTERNTYPE0(Digest)* xdigest, uint8_buffer_t buf, SHORT_STDPARAMS) {
    #ifdef T4P4S_NO_CONTROL_PLANE
        debug(" " T4LIT(!!!!,warning) " " T4LIT(Ignoring digest,warning) " to port " T4LIT(1024,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(%s,extern) " because " T4LIT(the control plane was disabled,warning) "\n", buf.name);
        encountered_digest_without_control_plane = true;
        return;
    #endif

    debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(1024,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(%s,extern) "\n", buf.name);

    ctrl_plane_digest digest = create_digest(bg, "learn_digest");

    for (int i = 0; i < buf.part_count; ++i) {
        int offset = buf.part_bit_offsets[i] / 8;
        int size = buf.part_bit_sizes[i];
        int bytesize = size / 8;
        const char*const part_name = buf.part_names[i];
        uint8_t* ptr = buf.buffer + offset;

        add_digest_field(digest, ptr, size);
        if (size <= 32) {
            uint32_t value;
            if (bytesize == 1)        value = *(uint8_t*)ptr;
            else if (bytesize == 2)   value = *(uint16_t*)ptr;
            else                      value = *(uint32_t*)ptr;

            debug("       : " T4LIT(%s,field) "/" T4LIT(%db) " = " T4LIT(%x) "\n", part_name, size, value);
        } else {
            dbg_bytes(ptr, bytesize, "       : " T4LIT(%s,field) " = ", part_name);
        }
    }

    send_digest(bg, digest, STD_DIGEST_RECEIVER_ID);
    sleep_millis(300);
}
