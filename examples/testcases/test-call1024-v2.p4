#include "common-boilerplate-pre.p4"

struct headers {
    ethernet_t ethernet;
}

struct metadata {}

PARSER {
    state start {
        packet.extract(hdr = hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    action myAction(PortId_t port) {
        SET_EGRESS_PORT(port);
    }

    table myTable {
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        actions = {
            myAction;
        }

        const entries = {
            0x000000000000 : myAction((PortId_t)1);
        }
    }

    action myAction2() {
    }

    action myAction2_1() {
        myAction2();
        myAction2();
    }

    action myAction2_2() {
        myAction2_1();
        myAction2_1();
    }

    action myAction2_3() {
        myAction2_2();
        myAction2_2();
    }

    action myAction2_4() {
        myAction2_3();
        myAction2_3();
    }

    action myAction2_5() {
        myAction2_4();
        myAction2_4();
    }

    action myAction2_6() {
        myAction2_5();
        myAction2_5();
    }

    action myAction2_7() {
        myAction2_6();
        myAction2_6();
    }

    action myAction2_8() {
        myAction2_7();
        myAction2_7();
    }

    action myAction2_9() {
        myAction2_8();
        myAction2_8();
    }

    action myAction2_10() {
        myAction2_9();
        myAction2_9();
    }

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        myAction2_10();

        myTable.apply();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"
