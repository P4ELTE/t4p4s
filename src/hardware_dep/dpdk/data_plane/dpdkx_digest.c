// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include "actions.h"
#include "util_debug.h"

extern ctrl_plane_backend bg;

#define STD_DIGEST_RECEIVER_ID 1024

void extern_Digest_pack(mac_learn_digest_t* mac_learn_digest) {
    debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(1024,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(mac_learn_digest,extern) "\n");
    dbg_bytes(&(mac_learn_digest->srcAddr), 6, "       : " T4LIT(srcAddr,field) "/" T4LIT(48) " = ");
    debug("       : " T4LIT(ingress_port,field) "/" T4LIT(32) " = " T4LIT(%x) "\n", mac_learn_digest->ingress_port);

    ctrl_plane_digest digest = create_digest(bg, "mac_learn_digest");

    add_digest_field(digest, &(mac_learn_digest->srcAddr), 6*8);
    add_digest_field(digest, &(mac_learn_digest->ingress_port), 4*8);

    send_digest(bg, digest, STD_DIGEST_RECEIVER_ID);
    sleep_millis(300);
}
