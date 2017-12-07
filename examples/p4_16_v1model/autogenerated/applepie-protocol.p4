#include <core.p4>
#include <v1model.p4>

struct cooking_metadata_t {
    bit<32> baking_time;
}

header applepie_t {
    bit<32> baking_time;
    bit<32> weight;
    bit<7>  ingredients_num;
    bit<3>  yummy_factor;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name("cooking_metadata") 
    cooking_metadata_t cooking_metadata;
}

struct headers {
    @name("applepie") 
    applepie_t applepie;
    @name("ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_applepie") state parse_applepie {
        packet.extract(hdr.applepie);
        transition accept;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x123: parse_applepie;
            default: accept;
        }
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".forward") action forward(bit<48> dmac_val, bit<48> smac_val, bit<9> port) {
        hdr.ethernet.dstAddr = dmac_val;
        standard_metadata.egress_spec = port;
        hdr.ethernet.srcAddr = smac_val;
    }
    @name("._drop") action _drop() {
        mark_to_drop();
    }
    @name(".set_baking_time") action set_baking_time(bit<32> baking_time) {
        meta.cooking_metadata.baking_time = baking_time;
        hdr.applepie.yummy_factor = hdr.applepie.yummy_factor + 3w1;
    }
    @name(".nextpie") table nextpie {
        actions = {
            forward;
            _drop;
        }
        key = {
            meta.cooking_metadata.baking_time: exact;
        }
        size = 512;
    }
    @name(".process_applepie") table process_applepie {
        actions = {
            set_baking_time;
            _drop;
        }
        key = {
            hdr.applepie.weight: exact;
        }
        size = 1024;
    }
    apply {
        process_applepie.apply();
        nextpie.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.applepie);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
