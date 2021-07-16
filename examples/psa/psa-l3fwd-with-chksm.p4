#include <core.p4>
#include <bmv2/psa.p4>

struct routing_metadata_t {
    bit<32> nhgroup;
}

header arp_t {
    bit<16> hardware_type;
    bit<16> protocol_type;
    bit<8>  HLEN;
    bit<8>  PLEN;
    bit<16> OPER;
    bit<48> sender_ha;
    bit<32> sender_ip;
    bit<48> target_ha;
    bit<32> target_ip;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<8>  versionIhl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<16> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct empty_metadata_t {
}

struct metadata {
    routing_metadata_t routing_metadata;
}

struct headers {
    arp_t      arp;
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

error {
    BadIPv4HeaderChecksum
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta)
{
    InternetChecksum() ck;
    state parse_arp {
        packet.extract(hdr.arp);
        transition accept;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            16w0x806: parse_arp;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        ck.clear();
        ck.add({hdr.ipv4.versionIhl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr});
        verify(ck.get() == hdr.ipv4.hdrChecksum,
               error.BadIPv4HeaderChecksum);
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
    }
}

control ingress(inout headers hdr,
                inout metadata meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    action set_nhop(bit<32> nhgroup) {
        meta.routing_metadata.nhgroup = nhgroup;
    }
    action _drop() {
        ostd.drop=true;
    }
    action _nop() {
    }
    action forward(bit<48> dmac_val, bit<48> smac_val, PortId_t port) {
        hdr.ethernet.dstAddr = dmac_val;
        ostd.egress_port = port;
        // istd.ingress_port = port;
        hdr.ethernet.srcAddr = smac_val;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 8w1;
    }
    table ipv4_lpm {
        actions = {
            set_nhop;
            _drop;
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 1024;
    }
    table macfwd {
        actions = {
            _nop;
            _drop;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 256;
    }
    table nexthops {
        actions = {
            forward;
            _drop;
        }
        key = {
            meta.routing_metadata.nhgroup: exact;
        }
        size = 512;
    }
    apply {
        if (macfwd.apply().hit) {
            ipv4_lpm.apply();
            nexthops.apply();
        }
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
    InternetChecksum() ck;
    apply {
        ck.clear();
        ck.add({hdr.ipv4.versionIhl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr});
        hdr.ipv4.hdrChecksum = ck.get();
        buffer.emit(hdr.ethernet);
        buffer.emit(hdr.arp);
        buffer.emit(hdr.ipv4);
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
