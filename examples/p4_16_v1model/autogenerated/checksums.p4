header ipv4_t_1 {
    bit<4>  version;
    bit<4>  ihl;
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

header ipv4_t_2 {
    varbit<320> options;
}

header tcp_t_1 {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header tcp_t_2 {
    varbit<320> options;
}

#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<16>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    bit<32>     srcAddr;
    bit<32>     dstAddr;
    @length((bit<32>)ihl * 32w4 * 8 - 160) 
    varbit<320> options;
}

header tcp_t {
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<32>     seqNo;
    bit<32>     ackNo;
    bit<4>      dataOffset;
    bit<4>      res;
    bit<8>      flags;
    bit<16>     window;
    bit<16>     checksum;
    bit<16>     urgentPtr;
    @length((bit<32>)dataOffset * 32w4 * 8 - 160) 
    varbit<320> options;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
    @name("tcp") 
    tcp_t      tcp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ipv4_t_1 tmp_hdr;
    ipv4_t_2 tmp_hdr_0;
    tcp_t_1 tmp_hdr_1;
    tcp_t_2 tmp_hdr_2;
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(tmp_hdr);
        packet.extract(tmp_hdr_0, (bit<32>)((bit<32>)tmp_hdr.ihl * 32w4 * 8 - 160));
        hdr.ipv4.setValid();
        hdr.ipv4.version = tmp_hdr.version;
        hdr.ipv4.ihl = tmp_hdr.ihl;
        hdr.ipv4.diffserv = tmp_hdr.diffserv;
        hdr.ipv4.totalLen = tmp_hdr.totalLen;
        hdr.ipv4.identification = tmp_hdr.identification;
        hdr.ipv4.fragOffset = tmp_hdr.fragOffset;
        hdr.ipv4.ttl = tmp_hdr.ttl;
        hdr.ipv4.protocol = tmp_hdr.protocol;
        hdr.ipv4.hdrChecksum = tmp_hdr.hdrChecksum;
        hdr.ipv4.srcAddr = tmp_hdr.srcAddr;
        hdr.ipv4.dstAddr = tmp_hdr.dstAddr;
        hdr.ipv4.options = tmp_hdr_0.options;
        transition select(hdr.ipv4.protocol) {
            8w0x6: parse_tcp;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract(tmp_hdr_1);
        packet.extract(tmp_hdr_2, (bit<32>)((bit<32>)tmp_hdr.dataOffset * 32w4 * 8 - 160));
        hdr.tcp.setValid();
        hdr.tcp.srcPort = tmp_hdr.srcPort;
        hdr.tcp.dstPort = tmp_hdr.dstPort;
        hdr.tcp.seqNo = tmp_hdr.seqNo;
        hdr.tcp.ackNo = tmp_hdr.ackNo;
        hdr.tcp.dataOffset = tmp_hdr.dataOffset;
        hdr.tcp.res = tmp_hdr.res;
        hdr.tcp.flags = tmp_hdr.flags;
        hdr.tcp.window = tmp_hdr.window;
        hdr.tcp.checksum = tmp_hdr.checksum;
        hdr.tcp.urgentPtr = tmp_hdr.urgentPtr;
        hdr.tcp.options = tmp_hdr_0.options;
        transition accept;
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
    @name(".on_miss") action on_miss() {
    }
    @name(".fib_hit_nexthop") action fib_hit_nexthop(bit<48> dmac, bit<9> port) {
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_port = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w0xff;
    }
    @name(".rewrite_src_mac") action rewrite_src_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name(".ipv4_fib_lpm") table ipv4_fib_lpm {
        actions = {
            on_miss;
            fib_hit_nexthop;
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 512;
    }
    @name(".sendout") table sendout {
        actions = {
            on_miss;
            rewrite_src_mac;
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 512;
    }
    apply {
        ipv4_fib_lpm.apply();
        sendout.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    Checksum16() ipv4_checksum;
    Checksum16() tcp_checksum;
    apply {
        if (hdr.ipv4.isValid() && hdr.ipv4.hdrChecksum == ipv4_checksum.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options })) 
            mark_to_drop();
        if (hdr.tcp.isValid() && hdr.tcp.checksum == tcp_checksum.get({ hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, hdr.ipv4.totalLen, 16w0xffeb, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp.options })) 
            mark_to_drop();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    Checksum16() ipv4_checksum;
    Checksum16() tcp_checksum;
    apply {
        if (hdr.ipv4.isValid()) 
            hdr.ipv4.hdrChecksum = ipv4_checksum.get({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options });
        if (hdr.tcp.isValid()) 
            hdr.tcp.checksum = tcp_checksum.get({ hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, hdr.ipv4.totalLen, 16w0xffeb, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp.options });
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
