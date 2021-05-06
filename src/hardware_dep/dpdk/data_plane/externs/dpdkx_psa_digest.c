// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_model_psa.h"

#include "actions.h"
#include "util_debug.h"

extern ctrl_plane_backend bg;

#define STD_DIGEST_RECEIVER_ID 1024

#ifdef T4P4S_TYPE_mac_learn_digest_t
    void extern_Digest_pack__mac_learn_digest_t(mac_learn_digest_t* mac_learn_digest, Digest_t* out_digest, SHORT_STDPARAMS) {
        // debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(STD_DIGEST_RECEIVER_ID,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(mac_learn_digest,extern) "\n");
        // dbg_bytes(&(mac_learn_digest->srcAddr), 6, "       : " T4LIT(srcAddr,field) "/" T4LIT(48) " = ");
        // debug("       : " T4LIT(ingress_port,field) "/" T4LIT(32) " = " T4LIT(%x) "\n", mac_learn_digest->ingress_port);

        // ctrl_plane_digest digest = create_digest(bg, "mac_learn_digest");

        // add_digest_field(digest, &(mac_learn_digest->srcAddr), 6*8);
        // add_digest_field(digest, &(mac_learn_digest->ingress_port), 4*8);

        // send_digest(bg, digest, STD_DIGEST_RECEIVER_ID);
        // sleep_millis(300);
    }
#endif

#ifdef T4P4S_TYPE_learn_digest_t
    void extern_Digest_pack__learn_digest_t(learn_digest_t* learn_digest, Digest_t* out_digest, SHORT_STDPARAMS) {
        debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Sending digest,outgoing) " to port " T4LIT(1024,port) " using extern " T4LIT(extern_Digest_pack,extern) " for " T4LIT(learn_digest,extern) "\n");
        dbg_print(&(learn_digest->addr), 1, "       : " T4LIT(addr,field));
        debug("       : " T4LIT(dd,field) "/" T4LIT(8) " = " T4LIT(%x) "\n", learn_digest->dd);

        ctrl_plane_digest digest = create_digest(bg, "learn_digest");

        add_digest_field(digest, &(learn_digest->addr), 8);
        add_digest_field(digest, &(learn_digest->dd), 8);

        send_digest(bg, digest, STD_DIGEST_RECEIVER_ID);
        sleep_millis(300);
    }
#endif
