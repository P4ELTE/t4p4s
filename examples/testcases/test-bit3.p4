#include "common-boilerplate-pre.p4"


struct headers {
    bits8_t    dummy;
    bits32_t   outhdr;
}

struct metadata {}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_MAIN {
    bit<32> time_next = 1;
    bit<32> time_next2;
    bit<32> cTimeUpdate = 0x12345678;

    register<bit<32>>(1) time_next_reg;
    register<bit<32>>(1) time_next_reg2;

    apply {
        time_next_reg.write(0, time_next);
        time_next_reg.read(time_next2, 0);
        time_next2 = time_next2 + cTimeUpdate;
        time_next_reg2.write(0, time_next2);

        hdr.outhdr.setValid();
        time_next_reg2.read(hdr.outhdr.f32, 0);
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.outhdr);
    }
}

#include "common-boilerplate-post.p4"
