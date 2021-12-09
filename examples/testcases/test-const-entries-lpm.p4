
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
    action action2() { hdr.ethernet.srcAddr =  0x999999999999; }
    action action3() { hdr.ethernet.srcAddr =  0x555555555555; }
    
    table t1 {
        actions = {
            nop;
            action1;
            action2;
            action3;
        }

        key = {
            hdr.ethernet.srcAddr: lpm;
        }

        size = 1;

        default_action = nop;

        const entries = {
            0x112233445500 &&& 0xFFF : action1;
            0x113300000000 &&& 0xFFF : action2;
            0x113300000000 &&& 0xF   : action3;
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
