#include <core.p4>
#include <bmv2/psa.p4>

// In: 00000000
// Out: 11110000

struct metadata {
	bit<32> addr1;
}

header dummy_t {
    bit<8> version;
	bit<32> addr1;
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
	const bit<32> addr2 = 0x12345678;
	bit<32> addr3 = 0;

    action action1(bit<32> data) { hdr.dummy.addr1 = data + 32w1; }
    action action2() {meta.addr1 = 0x12345678 + 32w1; }
    action action3() {meta.addr1 = addr2 + 32w1; }
    action action4(bit<32> data) { addr3 = data + 32w1; }
    action action5(bit<32> data) { meta.addr1 = data + 32w1; }

    table t1 {
        actions = {
            action1;
            action2;
            action3;
            action4;
            action5;
        }

        key = {
            hdr.dummy.version: exact;
            hdr.dummy.addr1: exact;
        }

        size = 3;

        const entries = {
            (0x00, 0x12345678) : action1(0x12345678);
            (0x01, 0x12345678) : action2();
            (0x02, 0x12345678) : action1(addr2);
            (0x03, 0x12345678) : action3();
            (0x04, 0x12345678) : action4(0x12345678);
            (0x05, 0x12345678) : action5(0x12345668);
        }
    }

    table t2 {
		actions = {
			action1;
		}
		key = {
			meta.addr1: exact;
		}
		size = 2;
		const entries = {
			0x12345669 : action1(0x12345678);
		}
    }

    table t3 {
		actions = {
			action1;
		}
		key = {
			addr3: exact;
		}
		size = 2;
		const entries = {
			0x12345678 : action1(0x12345678);
		}
    }

    apply {
		t1.apply();
		t2.apply();
		t3.apply();

        if (hdr.dummy.version==0x01 || hdr.dummy.version==0x03) {
			hdr.dummy.addr1 = meta.addr1;
        }
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
