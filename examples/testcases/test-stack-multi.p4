
#include "common-boilerplate-pre.p4"

struct metadata {
}

header data_t {
    bit<8> f8;
    bit<8> cont;
}

struct headers {
    data_t[5] multi;
    bits8_t   result;
}

PARSER {
    state start {
        packet.extract(hdr.result);
        transition multi_extract;
    }

    state multi_extract {
        packet.extract(hdr.multi.next);

        hdr.result.f8 = hdr.multi.last.f8;

        transition select(hdr.multi.last.cont) {
            8w1     : multi_extract;
            default : accept;
        }
    }
}

CTL_MAIN {
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        if (hdr.multi[0].isValid())  hdr.multi[0].setInvalid();
        if (hdr.multi[1].isValid())  hdr.multi[1].setInvalid();
        if (hdr.multi[2].isValid())  hdr.multi[2].setInvalid();
        if (hdr.multi[3].isValid())  hdr.multi[3].setInvalid();
        if (hdr.multi[4].isValid())  hdr.multi[4].setInvalid();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.result);
    }
}

#include "common-boilerplate-post.p4"
