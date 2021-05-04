/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

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

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.inhdr);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}

control ingress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
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

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        if (hdr.inhdr.f8 == 8)     packet.emit(hdr.outhdr8);
        if (hdr.inhdr.f8 == 16)    packet.emit(hdr.outhdr16);
        if (hdr.inhdr.f8 == 32)    packet.emit(hdr.outhdr32);
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               inout standard_metadata_t standard_metadata) {
    apply { }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
     apply { }
}

V1Switch(MyParser(),MyVerifyChecksum(),ingress(),egress(),MyComputeChecksum(),MyDeparser()) main;
