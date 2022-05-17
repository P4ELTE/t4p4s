// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    test_dstAddr_t   dstAddr;
    test_srcAddr_t   srcAddrIn;
    test_srcAddr_t   srcAddrAdded;
    test_etherType_t etherType;
}

PARSER {
    state start {
        packet.extract(hdr.dstAddr);
        packet.extract(hdr.srcAddrIn);
        packet.extract(hdr.etherType);
        transition accept;
    }
}

CTL_MAIN {
    action setValid_srcAddrAdded() {
        hdr.srcAddrAdded.setValid();
        hdr.srcAddrAdded.srcAddr = hdr.srcAddrIn.srcAddr;
    }

    table dmac {
        actions = {
            setValid_srcAddrAdded;
        }

        key = {
        }

        size = 1;

        default_action = setValid_srcAddrAdded;
    }

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        dmac.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dstAddr);
        packet.emit(hdr.srcAddrIn);
        packet.emit(hdr.srcAddrAdded);
        packet.emit(hdr.etherType);
    }
}

#include "common-boilerplate-post.p4"
