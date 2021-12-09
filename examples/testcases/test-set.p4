
#include "common-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
  bit<1> f1;
  bit<8> f2;
  bit<4> f3;
  bit<2> f4;
  bit<2> f5;
  bit<7> padding;
}

struct headers {
    dummy_t dummy;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition select(hdr.dummy.f1) {
            1w1 : stage1;
            _   : reject;
        }
    }

    state stage1 {
        hdr.dummy.f1 = hdr.dummy.f1 + 1;
        transition select(hdr.dummy.f2) {
            8w0x0A &&& 8w0x0F : stage2;
            _                 : reject;
        }
    }

    state stage2 {
        hdr.dummy.f2 = 8w0xFF;
        transition select(hdr.dummy.f3) {
            4w5..4w8 : stage3;
            _        : reject;
        }
    }

    state stage3 {
        hdr.dummy.f3 = 4w0xF;
        transition select(hdr.dummy.f4, hdr.dummy.f5) {
            (2w0, 2w1) : stage4;
            (2w0, 2w0) : reject;
            (_, _)     : reject;
        }
    }

    state stage4 {
        hdr.dummy.f4 = 2w3;
        hdr.dummy.f5 = 2w3;
        transition accept;
    }
}

CTL_MAIN {
    apply {}
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
