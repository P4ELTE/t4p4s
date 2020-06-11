// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include "actions.h"
#include "util.h"

extern ctrl_plane_backend bg;

#define STD_DIGEST_RECEIVER_ID 1024

void extern_Digest_pack(learn_digest_t* learn_digest) {
    debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(1024,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(learn_digest,extern) "\n");
    dbg_bytes(&(learn_digest->addr), 1, "       : " T4LIT(addr,field) "/" T4LIT(8) " = ");
    debug("       : " T4LIT(dd,field) "/" T4LIT(8) " = " T4LIT(%x) "\n", learn_digest->dd);

    ctrl_plane_digest digest = create_digest(bg, "learn_digest");

    add_digest_field(digest, &(learn_digest->addr), 8);
    add_digest_field(digest, &(learn_digest->dd), 8);

    send_digest(bg, digest, STD_DIGEST_RECEIVER_ID);
    sleep_millis(300);
}
