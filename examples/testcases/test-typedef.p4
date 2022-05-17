#include "common-boilerplate-pre.p4"

typedef bit<1>  one_bit_t;
typedef bit<8>  one_byte_t;
typedef bit<16> two_byte_t;

header dummy_t {
    bit<7>     padding;
    one_bit_t  f0;

    one_byte_t byte_field1;
    two_byte_t byte_field2;
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_MAIN {
    one_bit_t one = 1;

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        hdr.dummy.f0          = hdr.dummy.f0 + one + 1w0;
        hdr.dummy.byte_field1 = 0xFF;
        hdr.dummy.byte_field2 = 0xACDC;
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
