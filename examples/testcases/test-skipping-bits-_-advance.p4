
#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits8_t dummy;
}

PARSER {
    state start {
        packet.extract<bits8_t>(_);
        packet.advance(2*8);
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_MAIN {
    apply {
       hdr.dummy.f8 = hdr.dummy.f8 + 1;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
