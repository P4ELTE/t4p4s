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
    bit<32> time_next1 = 1;
    bit<32> time_next2;
    bit<32> cTimeUpdate = 0x12345678;

    DECLARE_REGISTER(bit<32>, 1, time_next_reg1)
    DECLARE_REGISTER(bit<32>, 1, time_next_reg2)

    apply {
        REGISTER_WRITE(time_next_reg1, 0, time_next1);
        REGISTER_READ(time_next2, time_next_reg1, 0);
        time_next2 = time_next2 + cTimeUpdate;
        REGISTER_WRITE(time_next_reg2, 0, time_next2);

        hdr.outhdr.setValid();
        REGISTER_READ(hdr.outhdr.f32, time_next_reg2, 0);
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.outhdr);
    }
}

#include "common-boilerplate-post.p4"
