
#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<8> f1;
    bit<8> f2;
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


control CNLookup4(in    bit<8> in_data,
                  out   bit<8> out_arg
                 ) {
    action def_action() {
        out_arg = 0x11;
    }

    action action_hit() {
        out_arg = 0x22;
    }

    table table_aa {
        key = {
            in_data : exact;
        }
        actions = {
            def_action;
            action_hit;
        }

        default_action = def_action;

        const entries = {
            (0xAA): action_hit;
        }
    }

    apply {
        out_arg = 0x33;
        if (!table_aa.apply().hit) {
            out_arg = 0x44;
        }
    }
}

CTL_MAIN {
    CNLookup4() lookup4;
    bit<8> out_data = 0;

    apply {
        lookup4.apply(hdr.dummy.f1, out_data);
        hdr.dummy.f2 = out_data;

        standard_metadata.egress_port = 123;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
