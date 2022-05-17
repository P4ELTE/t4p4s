// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddr;
    test_etherType_t etherType;
    test_srcAddr_t   srcAddr2;
}

PARSER {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddr);
        packet.extract(hdr.etherType);
        packet.extract(hdr.srcAddr2);
        transition accept;
    }
}

CTL_MAIN {
    action setInvalid_srcAddr2() {
        hdr.srcAddr2.setInvalid();
    }

    table dmac {
        actions = {
            setInvalid_srcAddr2;
        }

        key = {
        }

        size = 1;

        default_action = setInvalid_srcAddr2;
    }

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        if (hdr.dstAddr.isValid()) {
            packet.emit(hdr.dstAddr);
        }
        if (hdr.srcAddr.isValid()) {
            packet.emit(hdr.srcAddr);
        }
        if (hdr.srcAddr2.isValid()) {
            packet.emit(hdr.srcAddr2);
        }
        if (hdr.etherType.isValid()) {
            packet.emit(hdr.etherType);
        }
    }
}

#include "common-boilerplate-post.p4"
