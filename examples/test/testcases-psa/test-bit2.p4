#include <core.p4>
#include <bmv2/psa.p4>

// In: 00000000
// Out: 11111000

header dummy_t {
    bit<1> f11;
    bit<1> f12;
    bit<3> f14;
    bit<3> padding;
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}
parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state parse_ethernet {
        packet.extract(hdr.dummy);
        transition accept;
    }
    state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    bit tmp_bit = 1;
    bit<2> tmp_bit2 = 2w3;

    apply {

       //note: saturating arithmetic is recognized by T4P4S but treated as regular arithmetic
       hdr.dummy.f11 = (hdr.dummy.f11 |-| tmp_bit) + tmp_bit;
       hdr.dummy.f12 = (hdr.dummy.f12 + tmp_bit) |+| tmp_bit;

       // note: shifting by 123 does not make sense, as it is longer than f14 (the spec defines this special case)
       hdr.dummy.f14[0:0] = (bit<1>)((hdr.dummy.f14 >> 123) == (bit<3>)0);

       // note: currently the partial update of a field is not supported
       hdr.dummy.f14[0:0] = (bit<1>)((hdr.dummy.f14 >> 1) == (bit<3>)0);
       hdr.dummy.f14[2:1] = (((hdr.dummy.f14 ++ (bit<3>)7) >> 1) << 2)[3:2];
    }
}


control ingress(inout headers hdr,
                inout metadata meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        ostd.egress_port = (PortId_t)12345;
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers hdr,
                        inout metadata meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control IngressDeparserImpl(packet_out buffer,
                            out empty_metadata_t clone_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers hdr,
                            in metadata meta,
                            in psa_ingress_output_metadata_t istd)
{
    apply {
        buffer.emit(hdr.dummy);
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_metadata_t clone_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers hdr,
                           in metadata meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
