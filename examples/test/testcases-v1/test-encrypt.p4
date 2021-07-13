// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "v1-boilerplate-pre.p4"


struct metadata {
}

header offset_t {
    bit<8> offset;
}
struct headers {
    offset_t offset;
}

PARSER {
    state start {
        packet.extract(hdr.offset);
        transition accept;
    }
}

extern void encrypt<T>(in T offset);

CTL_INGRESS {
    apply {
        hdr.offset.offset = hdr.offset.offset + 8w1;
        encrypt({hdr.offset.offset});
        hdr.offset.offset = hdr.offset.offset - 8w1;
    }
}

CTL_EMIT {
    apply {
    }
}


#include "v1-boilerplate-post.p4"
