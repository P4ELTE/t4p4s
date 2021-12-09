
#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action nop() {}
    action action1() { hdr.ethernet.srcAddr =  0x111111111111; }
    
    table t1 {
        actions = {
            nop;
            action1;
        }

        key = {
            hdr.ethernet.srcAddr: ternary;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x112233445566 &&& 0xF0000000000F : action1;
        }
    }

    apply {
        t1.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"
