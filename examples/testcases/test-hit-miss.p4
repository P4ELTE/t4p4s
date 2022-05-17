#include "common-boilerplate-pre.p4"

header output_t {
    padded_bool_t f1;
    padded_bool_t f2;
}

struct metadata {
}

struct headers {
    bits8_t  input;
    output_t output;
}

PARSER {
    state start {
        packet.extract(hdr.input);
        transition accept;
    }
}

CTL_MAIN {
    action dummy() {}

    table dummy_table {
        key = {hdr.input.f8: exact;}
        actions = {dummy;}
        const entries = {
            0 : dummy();
        }
    }

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        hdr.output.setValid();
        hdr.output.f1.b = dummy_table.apply().hit;
        hdr.output.f2.b = dummy_table.apply().miss;
        hdr.input.setInvalid();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.output);
    }
}

#include "common-boilerplate-post.p4"
