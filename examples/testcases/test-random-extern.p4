/* -*- P4_16 -*- */
#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    bits8_t inhdr;

    bits8_t outhdr8;
    // bits8_t outhdr8_2;

    bits16_t outhdr16;
    // bits16_t outhdr16_2;

    bits32_t outhdr32;
    // bits32_t outhdr32_2;
}

PARSER {
    state start {
        packet.extract(hdr.inhdr);
        transition accept;
    }
}

CTL_MAIN {
    action random8() {
        hdr.outhdr8.setValid();
        // hdr.outhdr8_2.setValid();

        bit<8> rand_val;
        random<bit<8>>(rand_val, 0, 254);
        hdr.outhdr8.f8 = rand_val;

        // random<bit<8>>(hdr.outhdr8_2.f8, 0, 254);
    }

    action random16() {
        hdr.outhdr16.setValid();
        // hdr.outhdr16_2.setValid();

        bit<16> rand_val;
        random<bit<16>>(rand_val, 0, 254);
        hdr.outhdr16.f16 = rand_val;

        // random<bit<16>>(hdr.outhdr16_2.f16, 0, 254);
    }

    action random32() {
        hdr.outhdr32.setValid();
        // hdr.outhdr32_2.setValid();

        bit<32> rand_val;
        random<bit<32>>(rand_val, 0, 254);
        hdr.outhdr32.f32 = rand_val;

        // random<bit<32>>(hdr.outhdr32_2.f32, 0, 254);
    }

    apply {
        standard_metadata.egress_port = 9w123;

        if (hdr.inhdr.f8 == 8)     random8();
        if (hdr.inhdr.f8 == 16)    random16();
        if (hdr.inhdr.f8 == 32)    random32();
    }
}

CTL_EMIT {
    apply {
        if (hdr.inhdr.f8 == 8)     packet.emit(hdr.outhdr8);
        if (hdr.inhdr.f8 == 16)    packet.emit(hdr.outhdr16);
        if (hdr.inhdr.f8 == 32)    packet.emit(hdr.outhdr32);
    }
}

#include "common-boilerplate-post.p4"
