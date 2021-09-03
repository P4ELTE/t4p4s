
#include "psa-boilerplate-pre.p4"

struct metadata {
}

header varbit_t {
    varbit<8> f2;
}

struct headers {
    bits8_t  f8;
    varbit_t orig;
    varbit_t copy;
}

PARSER {
    state start {
        packet.extract(hdr.f8);
        transition select(hdr.f8.f8) {
            8w0: short;
            8w1: long;
            default: reject;
        }
    }

    state short {
        packet.extract(hdr.orig, 2);
        transition accept;
    }

    state long {
        packet.extract(hdr.orig, 7);
        transition accept;
    }
}

CTL_EGRESS {
    apply {
        hdr.copy.setValid();
        hdr.copy = hdr.orig;

        hdr.f8.setInvalid();
        hdr.orig.setInvalid();
    }
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.copy);
    }
}

#include "psa-boilerplate-post.p4"
