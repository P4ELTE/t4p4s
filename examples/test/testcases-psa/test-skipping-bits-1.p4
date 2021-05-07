
#include "psa-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits16_t inhdr;
}

PARSER {
    state start {
        packet.extract<bits8_t>(_);
        packet.extract(hdr.inhdr);
        transition accept;
    }
}

CTL_EGRESS {
    apply {
        if (hdr.inhdr.f16 == 0x0FF) {
            hdr.inhdr.setInvalid();
        } else {
            hdr.inhdr.f16 = hdr.inhdr.f16 + 1;
        }
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.inhdr);
    }
}

#include "psa-boilerplate-post.p4"
