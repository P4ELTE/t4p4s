
#include "v1-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits8_t h8;
}

PARSER {
    state start {
        packet.extract(hdr.h8);
        transition accept;
    }
}

CTL_INGRESS {
    action nop()     { }
    action action1() { hdr.h8.f8 = 0x12; }
    action action2() { hdr.h8.f8 = 0x34; }

    table t1 {
        actions = {
            nop;
            action1;
        }

        key = {
            hdr.h8.f8: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0xAA : action1;
        }
    }

    table t2 {
        actions = {
            nop;
            action2;
        }

        key = {
            hdr.h8.f8: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x12 : action2;
        }
    }

    apply {
        switch (t1.apply().action_run) {
            action1: {
                t2.apply();
            }
        }
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.h8);
    }
}

#include "v1-boilerplate-post.p4"
