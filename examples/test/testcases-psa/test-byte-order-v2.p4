#include <core.p4>
#include <bmv2/psa.p4>

// In: 00000000
// Out: 11110000

struct metadata {
	bit<28> addr1;
}

header dummy_t {
	bit<28> addr1;
	bit<4> padding;
}

struct empty_metadata_t {
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
	const bit<28> addr2 = 0x1234568;
	bit<28> addr3 = 1;

    action action1(bit<28> data) { hdr.dummy.addr1 = data + 28w1; }
    action action2() {meta.addr1 = 0x1234678 + 28w1; }
    action action3() {meta.addr1 = addr2 + 28w1; }
    action action4(bit<28> data) { addr3 = data + 28w1; }
    action action5(bit<28> data) { meta.addr1 = data + 28w1; }

    table t1 {
        actions = {
            action1;
            action2;
            action3;
            action4;
            action5;
        }

        key = {
            hdr.dummy.addr1: exact;
        }

        size = 3;

        const entries = {
            (0x1234567) : action1(0x1234568);
            (0x1234561) : action2();
            (0x1234562) : action1(addr2);
            (0x1234563) : action3();
            (0x1234564) : action4(0x1234567);
            (0x1234565) : action5(0x1234568);
        }
    }

    apply {
		t1.apply();
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
