#include <core.p4>
#include <bmv2/psa.p4>

// In: 000000000000000000000000
// Out: 111111111111111111111000

header dummy_t {
    int<2> f1;
    int<2> f2;
    int<2> f3;
    int<2> f4;
    int<2> f5;
    int<2> f6;
    int<2> f7;
    int<2> f8;
    int<2> f9;
    int<3> f10;
    int<3> padding;
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
    const int tmp_int = 1;
    int<(2*tmp_int)> tmp_int2 = -2s0;

    apply {
       hdr.dummy.f1 = hdr.dummy.f1 + 2s1;
       hdr.dummy.f2 = tmp_int2 - hdr.dummy.f2;
       hdr.dummy.f3 = hdr.dummy.f3 * tmp_int2 + tmp_int2;
       hdr.dummy.f4 = hdr.dummy.f4 | tmp_int2;
       hdr.dummy.f5 = ~(hdr.dummy.f5 & tmp_int2);
       hdr.dummy.f6 = hdr.dummy.f6 ^ tmp_int2;
       hdr.dummy.f7 = (hdr.dummy.f7 - tmp_int) |-| tmp_int;
       hdr.dummy.f8 = (hdr.dummy.f8 + tmp_int) |+| tmp_int;
       hdr.dummy.f9 = (int<2>)((hdr.dummy.f9 - 1)[1:0]);
       hdr.dummy.f10[0:0] = (((hdr.dummy.f10 >> 123) == (int<3>)0)?1w1:1w0);
       hdr.dummy.f10[2:1] = (((hdr.dummy.f10 ++ (int<3>)-1) >> 1) << 2)[3:2];
    }
}


control ingress(inout headers hdr,
                inout metadata meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply { }
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
