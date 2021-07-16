#include <core.p4>
#include <bmv2/psa.p4>

// In:  00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
// Out: 00000001 00000010 00000100 00001000 00010000 00100000 01000000 10000000

header dummy_t {
    bit<1> f7;
    bit<1> f6;
    bit<1> f5;
    bit<1> f4;
    bit<1> f3;
    bit<1> f2;
    bit<1> f1;
    bit<1> f0;
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    dummy_t dummy0;
    dummy_t dummy1;
    dummy_t dummy2;
    dummy_t dummy3;
    dummy_t dummy4;
    dummy_t dummy5;
    dummy_t dummy6;
    dummy_t dummy7;
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state parse_ethernet {
        packet.extract(hdr.dummy0);
        packet.extract(hdr.dummy1);
        packet.extract(hdr.dummy2);
        packet.extract(hdr.dummy3);
        packet.extract(hdr.dummy4);
        packet.extract(hdr.dummy5);
        packet.extract(hdr.dummy6);
        packet.extract(hdr.dummy7);
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
    bit<1> one = 1;

    apply {
       hdr.dummy0.f0  = hdr.dummy0.f0 + one + 1w0;

       // tests if modifying non-byte-aligned fields works properly
       hdr.dummy1.f1 = hdr.dummy1.f1 + one + 1w0;
       hdr.dummy2.f2 = hdr.dummy2.f2 + one + 1w0;
       hdr.dummy3.f3 = hdr.dummy3.f3 + one + 1w0;
       hdr.dummy4.f4 = hdr.dummy4.f4 + one + 1w0;
       hdr.dummy5.f5 = hdr.dummy5.f5 + one + 1w0;
       hdr.dummy6.f6 = hdr.dummy6.f6 + one + 1w0;
       hdr.dummy7.f7 = hdr.dummy7.f7 + one + 1w0;
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
        buffer.emit(hdr.dummy0);
        buffer.emit(hdr.dummy1);
        buffer.emit(hdr.dummy2);
        buffer.emit(hdr.dummy3);
        buffer.emit(hdr.dummy4);
        buffer.emit(hdr.dummy5);
        buffer.emit(hdr.dummy6);
        buffer.emit(hdr.dummy7);
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
