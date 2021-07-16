#include <core.p4>
#include <bmv2/psa.p4>

// In1: 0000000000000011
// Out1: 00000011
// In2: 1000000000000011
// Out2: 00000011

header dummy_t {
    bit<1> f1;
    bit<3> padding;
    varbit<4> f2;
}

header real_t {
    bit<2> f1;
    bit<6> padding;
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    dummy_t dummy;
    real_t real;
}
parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state short {
	packet.extract(hdr.dummy, 6);
	packet.extract(hdr.real);
        transition accept;
    }
    state long {
	packet.extract(hdr.dummy, 8);
	packet.extract(hdr.real);
        transition accept;
    }
    state parse_ethernet {
        transition select(packet.lookahead<bit<1>>()) {
		1w0: short;
		1w1: long;
	}
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
        hdr.dummy.setInvalid();
        buffer.emit(hdr.real);
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
