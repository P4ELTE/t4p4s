// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

header varbit_t {
    varbit<96> f1;
}

struct metadata {
}

struct headers {
    bits32_t len;
    varbit_t orig;
    varbit_t copy;
    varbit_t copy2;
}

PARSER {
    state start {
        packet.extract(hdr.len);
        packet.extract(hdr.orig, hdr.len.f32 * 8);
        transition accept;
    }
}

CTL_MAIN {
    apply {
        hdr.copy = { f1 = hdr.orig.f1 };

        hdr.copy2.setValid();
        hdr.copy2.f1 = hdr.orig.f1;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.orig);
        packet.emit(hdr.copy);
        packet.emit(hdr.copy2);
    }
}

#include "common-boilerplate-post.p4"
