#include <core.p4>
#include <v1model.p4>
#include "../include/std_headers.p4"


struct Parsed_packet {
    bits8_t    dummy;
    bits32_t   outhdr;
}

struct metadata_t {}

parser parserI(packet_in pkt,
               out Parsed_packet hdr,
               inout metadata_t meta,
               inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.dummy);
        transition accept;
    }
}

control DeparserI(packet_out packet,
                  in Parsed_packet hdr) {
    apply {
        packet.emit(hdr.outhdr);
    }
}

control ingress(inout Parsed_packet hdr,
                 inout metadata_t meta,
                 inout standard_metadata_t standard_metadata)
{
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

control egress(inout Parsed_packet hdr,
               inout metadata_t meta,
               inout standard_metadata_t standard_metadata) {
    apply { }
}

control vc(inout Parsed_packet hdr,
           inout metadata_t meta) {
    apply { }
}

control uc(inout Parsed_packet hdr,
           inout metadata_t meta) {
    apply { }
}

V1Switch(parserI(), vc(), ingress(), egress(), uc(), DeparserI()) main;
