// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "v1-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_INGRESS {
    apply {
        log_msg("Const8    = {}",{8w0});
        log_msg("Const8    = {}",{8w1});
        log_msg("Const8    = {}",{8w123});
        log_msg("Const8    = {}",{8w255});
        log_msg("Const16   = {}",{16w12345});
        log_msg("Const32   = {}",{32w12345});
        log_msg("Const32   = {}",{32w0});
        log_msg("Const32   = {}",{-32w1});
        log_msg("Const32   = {}",{32w0x0FFFFFFF});
        log_msg("Const32   = {}",{32w0xFFFF_FFFF});
        log_msg("Const64   = {}",{64w12345});
        log_msg("Const64   = {}",{64w0xFFFF_FFFF_FFFF_FFFF});

        bit<32> temp = 12345;
        log_msg("Var       = {}",{temp});

        log_msg("SrcAddr   = {}",{hdr.ethernet.srcAddr});

        log_msg("Const8x2  = {} {}",{8w0, 8w0});
        log_msg("Const8x2  = {} {}",{8w1, 8w2});
        log_msg("Const8x4  = {} {} {} {}",{8w0, 8w0, 8w0, 8w0});
        log_msg("Const8x4  = {} {} {} {}",{8w1, 8w2, 8w3, 8w4});
        log_msg("Const8x4  = {} {} {} {}",{8w0x12, 8w0x34, 8w0xab, 8w0xff});
        log_msg("Const16x4 = {} {} {} {}",{16w1, 16w2, 16w3, 16w4});
        log_msg("Const16x4 = {} {} {} {}",{16w0x0123, 16w0x4567, 16w0x89ab, 16w0xcdef});
        log_msg("Const32x4 = {} {} {} {}",{32w0x01234567, 32w0x89abcdef, 32w0xdeadc0de, 32w0xFFFF_FFFF});

        log_msg("Ethernet  = {}",{hdr.ethernet});
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "v1-boilerplate-post.p4"
