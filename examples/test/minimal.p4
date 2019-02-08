/* -*- P4_16 -*- */

#include <core.p4>
#include <v1model.p4>

header ethernet_header {
	bit<48> dstAddr;
	bit<48> srcAddr;
	bit<16> etherType;
}

struct parsed_packet {
	ethernet_header ethernet;
}

struct metadata {
}

parser MyParser(packet_in pkt, out parsed_packet hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	state start {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			0x0800: accept;
			default: accept;
		}
	}
}

control MyDeparser(packet_out pkt, in parsed_packet hdr) {
	apply { pkt.emit(hdr); }
}
register<bit<32>>(1) test_register;

control MyIngress(inout parsed_packet hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
  action _drop() {
		mark_to_drop();
  }
	action forward(bit<9> port) {
		standard_metadata.egress_spec = port;
	}
	table route {
		key = {
			standard_metadata.ingress_port: exact;
		}
		actions = {
			forward;
			_drop;
		}
		default_action = _drop;
	}

	apply { 
    bit<32> test_val;
    test_register.read(test_val, 0);
    route.apply();
	}
}

control MyEgress(inout parsed_packet hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	apply { }
}

control MyVerify(inout parsed_packet hdr, inout metadata meta) {
	apply { } 
}

control MyCalculate(inout parsed_packet hdr, inout metadata meta) {
	apply { }
}

V1Switch(
	MyParser(),
	MyVerify(),
	MyIngress(),
	MyEgress(),
	MyCalculate(),
	MyDeparser()
) main;

