// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

header custom_t {
    padded8_t  f8;
    padded16_t f16;
    padded4_t  f4;
}

struct headers {
    ethernet_t ethernet;
    custom_t   custom;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.custom);
        transition accept;
    }
}

CTL_MAIN {
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        LOGMSG("Hello world!");

        LOG("Const8    = {}",({8w0}));
        LOG("Const8    = {}",({8w1}));
        LOG("Const8    = {}",({8w123}));
        LOG("Const8    = {}",({8w255}));
        LOG("Const16   = {}",({16w12345}));
        LOG("Const32   = {}",({32w12345}));
        LOG("Const32   = {}",({32w0}));
        LOG("Const32   = {}",({-32w1}));
        LOG("Const32   = {}",({32w0x0FFFFFFF}));
        LOG("Const32   = {}",({32w0xFFFF_FFFF}));
        LOG("Const64   = {}",({64w12345}));
        LOG("Const64   = {}",({64w0xFFFF_FFFF_FFFF_FFFF}));

        bit<32> temp = 12345;
        LOG("Var       = {}",({temp}));

        LOG("SrcAddr   = {}",({hdr.ethernet.srcAddr}));

        LOG("Const8x2  = {} {}",({8w0, 8w0}));
        LOG("Const8x2  = {} {}",({8w1, 8w2}));
        LOG("Const8x4  = {} {} {} {}",({8w0, 8w0, 8w0, 8w0}));
        LOG("Const8x4  = {} {} {} {}",({8w1, 8w2, 8w3, 8w4}));
        LOG("Const8x4  = {} {} {} {}",({8w0x12, 8w0x34, 8w0xab, 8w0xff}));
        LOG("Const16x4 = {} {} {} {}",({16w1, 16w2, 16w3, 16w4}));
        LOG("Const16x4 = {} {} {} {}",({16w0x0123, 16w0x4567, 16w0x89ab, 16w0xcdef}));
        LOG("Const32x4 = {} {} {} {}",({32w0x01234567, 32w0x89abcdef, 32w0xdeadc0de, 32w0xFFFF_FFFF}));

        LOG("Custom    = {}",({hdr.custom}));

        LOG("Ethernet  = {}",({hdr.ethernet}));
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.custom);
    }
}

#include "common-boilerplate-post.p4"
