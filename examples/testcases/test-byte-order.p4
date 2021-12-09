
#include "common-boilerplate-pre.p4"

struct metadata {
    bit<32> addr1;
}

header dummy_t {
    bit<8>  version;
    bit<32> addr1;
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
    const bit<32> addr2 = 0x12345678;
          bit<32> addr3 = 0;

    action action1(bit<32> data) { hdr.dummy.addr1 = data       + 32w1; }
    action action2()             { meta.addr1      = 0x12345678 + 32w1; }
    action action3()             { meta.addr1      = addr2      + 32w1; }
    action action4(bit<32> data) { addr3           = data       + 32w1; }
    action action5(bit<32> data) { meta.addr1      = data       + 32w1; }

    table t1 {
        actions = {
            action1;
            action2;
            action3;
            action4;
            action5;
        }

        key = {
            hdr.dummy.version: exact;
            hdr.dummy.addr1:   exact;
        }

        size = 3;

        const entries = {
            (0x00, 0x12345678) : action1(0x12345678);
            (0x01, 0x12345678) : action2();
            (0x02, 0x12345678) : action1(addr2);
            (0x03, 0x12345678) : action3();
            (0x04, 0x12345678) : action4(0x12345678);
            (0x05, 0x12345678) : action5(0x12345668);
        }
    }

    table t2 {
        actions = {
            action1;
        }
        key = {
            meta.addr1: exact;
        }
        size = 2;
        const entries = {
            0x12345669 : action1(0x12345678);
        }
    }

    table t3 {
        actions = {
            action1;
        }
        key = {
            addr3: exact;
        }
        size = 2;
        const entries = {
            0x12345678 : action1(0x12345678);
        }
    }

    apply {
        t1.apply();
        t2.apply();
        t3.apply();

        if (hdr.dummy.version==0x01 || hdr.dummy.version==0x03) {
            hdr.dummy.addr1 = meta.addr1;
        }
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
