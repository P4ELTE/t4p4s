#include <core.p4>
#include <v1model.p4>


header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16>         etherType;
}

struct Parsed_packet {
    Ethernet_h    ethernet;
}

struct metadata_t {
}

parser parserI(packet_in pkt,
               out Parsed_packet hdr,
               inout metadata_t meta,
               inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control DeparserI(packet_out packet,
                  in Parsed_packet hdr) {
    apply { packet.emit(hdr.ethernet); }
}

control ingress(inout Parsed_packet hdr,
                 inout metadata_t meta,
                 inout standard_metadata_t standard_metadata)
{

    bit<32> time_next;
    bit<32> cTimeUpdate = 20000;

    register<bit<32>>(1) time_next_reg;

    apply {
           time_next_reg.read(time_next, 0);
           time_next = time_next + cTimeUpdate;
           time_next_reg.write(0, time_next);
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


V1Switch(parserI(),
    vc(),
    ingress(),
    egress(),
    uc(),
    DeparserI()) main;

