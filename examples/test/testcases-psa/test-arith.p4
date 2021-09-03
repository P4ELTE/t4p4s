
#include "psa-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits32_t in1;
    bits32_t in2;
    bits32_t result;
}

PARSER {
    state start {
        packet.extract(hdr.in1);
        packet.extract(hdr.in2);
        transition accept;
    }
}

CTL_EGRESS {
    apply {
        hdr.result.setValid();
        hdr.result.f32 = hdr.in1.f32 + hdr.in2.f32;

        hdr.in1.setInvalid();
        hdr.in2.setInvalid();
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.result);
    }
}

#include "psa-boilerplate-post.p4"
