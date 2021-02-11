
#include <v1model.p4>

header dummy_t {
    bit<8> f1;
    bit<8> f2;
}

struct empty_metadata_t {
}

struct metadata {
}

struct header_t {
    dummy_t dummy;
}

parser CNIngressParser(packet_in pkt, out header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        pkt.extract(hdr.dummy);
        transition accept;
    }
}


control CNLookup4(in    bit<8> in_data,
                  out   bit<8> out_arg
                 ) {
    action def_action() {
        out_arg = 0x11;
    }

    action action_hit() {
        out_arg = 0x22;
    }

    table table_aa {
        key = {
            in_data : exact;
        }
        actions = {
            def_action;
            action_hit;
        }

        default_action = def_action;

        const entries = {
            (0xAA): action_hit;
        }
    }

    apply {
        out_arg = 0x33;
        if (!table_aa.apply().hit) {
            out_arg = 0x44;
        }
    }
}

control CNIngress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    CNLookup4() lookup4;
    bit<8> out_data = 0;

    apply {
        lookup4.apply(hdr.dummy.f1, out_data);
        hdr.dummy.f2 = out_data;

        standard_metadata.egress_port = 123;
    }
}

control DeparserImpl(packet_out packet, in header_t hdr) {
    apply {
        packet.emit(hdr.dummy);
    }
}

control EmptyEgress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
control EmptyChecksum(inout header_t hdr, inout metadata meta) { apply {} }
control EmptyComputeChecksum(inout header_t hdr, inout metadata meta) { apply {} }

V1Switch(CNIngressParser(), EmptyChecksum(), CNIngress(), EmptyEgress(), EmptyComputeChecksum(), DeparserImpl()) main;
