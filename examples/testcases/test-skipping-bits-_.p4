
#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits16_t inhdr;
    bits8_t  always_present;
}

PARSER {
    state start {
        packet.extract<bits8_t>(_);
        packet.extract(hdr.inhdr);
        packet.extract(hdr.always_present);
        packet.extract<bits8_t>(_);
        transition accept;
    }
}

CTL_MAIN {
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
        packet.emit(hdr.inhdr);
        packet.emit(hdr.always_present);
    }
}

#include "common-boilerplate-post.p4"
