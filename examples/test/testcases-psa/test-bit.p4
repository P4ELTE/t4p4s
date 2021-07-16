#include <core.p4>
#include <bmv2/psa.p4>

// In: 0000000000000000
// Out: 1111111111111100

header dummy_t {
    bit<1> f1;
    bit<1> f2;
    bit<1> f3;
    bit<1> f4;
    bit<1> f5;
    bit<1> f6;
    bit<1> f7;
    bit<2> f8;
    bit<2> f9;
    bit<2> f10;
    bit<1> f13;
    bit<2> padding;
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
       hdr.dummy.f1 = hdr.dummy.f1 + (bit<1>)true;
       hdr.dummy.f2 = tmp_bit - hdr.dummy.f2;
       hdr.dummy.f3 = hdr.dummy.f3 * tmp_bit + tmp_bit;
       hdr.dummy.f4 = (bit<1>)(hdr.dummy.f4 == 0);
       hdr.dummy.f5 = (bit<1>)(hdr.dummy.f5 != 1);
       hdr.dummy.f6 = (bit<1>)(hdr.dummy.f6 > 0 || hdr.dummy.f6 >= 0);
       hdr.dummy.f7 = (bit<1>)(hdr.dummy.f7 < 1 || hdr.dummy.f7 <= 1);
       hdr.dummy.f8 = hdr.dummy.f8 | tmp_bit2;
       hdr.dummy.f9 = ~(hdr.dummy.f9 & tmp_bit2);
       hdr.dummy.f10 = hdr.dummy.f10 ^ tmp_bit2;
       hdr.dummy.f13 = (hdr.dummy.f13 ++ (hdr.dummy.f13 + 1))[0:0];
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
