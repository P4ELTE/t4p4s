#include <core.p4>
#include <psa.p4>

// In: 00000000000000000000000000000000
// Out: 1111100000000000

struct data {
	bit<1> first;
        bit<1> second;
        bit<1> third;
        bit<1> fourth;
}

header empty_t { }

header dummy_t {
    bit<1> f1;
    data f2;
    bit<3> padding;
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    empty_t empty;
    dummy_t dummy;
    dummy_t dummy2;
    dummy_t dummy3;
    dummy_t dummy4;
}
parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state parse_ethernet {
        packet.extract(hdr.empty);
        packet.extract(hdr.dummy);
        packet.extract(hdr.dummy2);
        packet.extract(hdr.dummy3);
        packet.extract(hdr.dummy4);
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
    apply {
       //Version 1: working
       //data d = {~hdr.dummy.f2.first, ~hdr.dummy.f2.second, ~hdr.dummy.f2.third, ~hdr.dummy.f2.fourth};
       //Version 2: working
       //data d = {1,1,1,1};
       //Version 3: C compile error
       data d = {~hdr.dummy.f2.fourth, ~hdr.dummy.f2.third, ~hdr.dummy.f2.second, ~hdr.dummy.f2.first};
       hdr.dummy.f1 = (bit<1>)hdr.empty.isValid();
       hdr.dummy.f2 = d;
       hdr.dummy3.setInvalid();
       hdr.dummy4.setInvalid();
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
       buffer.emit(hdr.dummy2);
       buffer.emit(hdr.dummy3);
       buffer.emit(hdr.dummy4);
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
