
#include "common-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
	bit<32> addr;
}

struct headers {
    ethernet_t ethernet;
    dummy_t    h1;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.h1);
        transition accept;
    }
}

CTL_MAIN {
    action nop() {}
    action action1(bit<32> data) { hdr.h1.addr = data; }
    action action2() {}
    action action3(PortId_t port) { SET_EGRESS_PORT(port); }

    table t1 {
        actions = {
            nop;
            action1;
            action2;
        }

        key = {
            hdr.ethernet.srcAddr: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x001234567890 : action1(0x0000000000000001);
            _              : action2();
        }
    }

    table t2 {
        actions = {
            nop;
            action2;
            action3;
        }

        key = {
            hdr.ethernet.dstAddr: exact;
            hdr.ethernet.srcAddr: exact;
        }

        size = 1;

        default_action = nop;

        const entries = {
            (0x12345678, 0x9ABCDEF0) : action2;
            (100, 200)               : action2;
            (300, 400)               : action3((PortId_t)500);
            (_, _)                   : action3((PortId_t)123);

            #ifdef T4P4S_TEST_1
                    (0xD15EA5E, 0XDEAD_BEEF): action1;
            #else
                    (0xDEAD_10CC, 0xBAAAAAAD): action2;
            #endif
        }
    }

    apply {
        t1.apply();
        t2.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.h1);
    }
}

#include "common-boilerplate-post.p4"
