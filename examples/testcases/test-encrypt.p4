// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

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

CTL_MAIN {
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

#include "common-boilerplate-post.p4"
