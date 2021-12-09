// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddr;
    test_etherType_t etherType;
}

PARSER {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddr);
        packet.extract(hdr.etherType);
        transition accept;
    }
}

CTL_MAIN {
    action setInvalid_srcAddr() {
        hdr.srcAddr.setInvalid();
    }

    table dmac {
        actions = {
            setInvalid_srcAddr;
        }

        key = {
        }

        size = 1;

        default_action = setInvalid_srcAddr;
    }

    apply {
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dstAddr);
        if (hdr.srcAddr.isValid()) {
            packet.emit(hdr.srcAddr);
        }
        packet.emit(hdr.etherType);
    }
}

#include "common-boilerplate-post.p4"
