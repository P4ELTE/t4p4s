// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "v1-boilerplate-pre.p4"


struct metadata {
}

header stk_t {
    padded8_t value;
}

struct headers {
    bits8_t  sum;
    stk_t[2] val;
}

PARSER {
    state start {
        packet.extract(hdr.val.next);

        hdr.sum.setValid();
        hdr.sum.f8 = hdr.val.last.value.f8;

        transition select(hdr.val.last.value.f8) {
            8w0:     accept;
            default: next;
        }
    }

    state next {
        packet.extract(hdr.val.next);

        hdr.sum.f8 = hdr.sum.f8 + hdr.val.last.value.f8;

        transition accept;
    }
}

CTL_INGRESS {
    apply {
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.sum);
        packet.emit(hdr.val);
    }
}


#include "v1-boilerplate-post.p4"
