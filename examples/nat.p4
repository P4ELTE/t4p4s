header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        fragOffset : 16;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

field_list ipv4_checksum_list {
        ipv4.version;
	ipv4.ihl;
        ipv4.diffserv;
        ipv4.totalLen;
        ipv4.identification;
        ipv4.fragOffset;
        ipv4.ttl;
        ipv4.protocol;
        ipv4.srcAddr;
        ipv4.dstAddr;
}

field_list_calculation ipv4_checksum {
    input {
        ipv4_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field ipv4.hdrChecksum {
    verify ipv4_checksum if(valid(ipv4));
    update ipv4_checksum if(valid(ipv4));
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}

field_list tcp_checksum_list {
    ipv4.srcAddr;
    ipv4.dstAddr;
    8'0;
    ipv4.protocol;
    ipv4.totalLen;
        /* -20 to complement the missing ipv4 length subtraction */
    16'0xffeb;
    tcp.srcPort;
    tcp.dstPort;
    tcp.seqNo;
    tcp.ackNo;
    tcp.dataOffset;
    tcp.res;
    tcp.flags;
    tcp.window;
    tcp.urgentPtr;
    payload; 
}

field_list_calculation tcp_checksum {
    input {
        tcp_checksum_list;
    }
    algorithm : csum16;
    output_width : 16;
}

calculated_field tcp.checksum {
    verify tcp_checksum if(valid(tcp));
    update tcp_checksum if(valid(tcp));
}

header_type udp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        len : 16;
        checksum : 16;
    }
}

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_IPV4 0x0800

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;

#define IP_PROT_TCP 0x06
#define IP_PROT_UDP 0x11

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol) {
        IP_PROT_TCP : parse_tcp;
        IP_PROT_UDP : parse_udp;
        default : ingress;
    }
}

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
    return ingress;
}

header udp_t udp;

parser parse_udp {
    extract(udp);
    return ingress;
}

table own_pool {
    reads {
        ipv4.dstAddr : exact;
    }
    actions {
	_no_op;
    }
}

action _no_op() {
    no_op();
}

table nat_ul_tcp {
    reads {
        ipv4.srcAddr : exact;
        tcp.srcPort : exact;
    }
    actions {
        natTcp_learn;
        encodeTcp_src;
    }
}

table nat_ul_udp {
    reads {
        ipv4.srcAddr : exact;
        udp.srcPort : exact;
    }
    actions {
        natUdp_learn;
        encodeUdp_src;
    }
}

#define NAT_TCP_LEARN_RECEIVER 1025
#define NAT_UDP_LEARN_RECEIVER 1026

field_list natTcp_learn_digest {
    ipv4.srcAddr;
    tcp.srcPort;
}

field_list natUdp_learn_digest {
    ipv4.srcAddr;
    udp.srcPort;
}

action natTcp_learn() {
    generate_digest(NAT_TCP_LEARN_RECEIVER, natTcp_learn_digest);
    //resubmit();
}

action natUdp_learn() {
    generate_digest(NAT_UDP_LEARN_RECEIVER, natUdp_learn_digest);
    //resubmit();
}

action encodeTcp_src(ipAddr, tcpPort) {
    modify_field(ipv4.srcAddr, ipAddr);
    modify_field(tcp.srcPort, tcpPort);
}

action encodeUdp_src(ipAddr, udpPort) {
    modify_field(ipv4.srcAddr, ipAddr);
    modify_field(udp.srcPort, udpPort);
}

table nat_dl_tcp {
    reads {
        ipv4.dstAddr : exact;
        tcp.dstPort : exact;
    }
    actions {
        resolve_dstTcp;
        _drop;
    }
}

table nat_dl_udp {
    reads {
        ipv4.dstAddr : exact;
        udp.dstPort : exact;
    }
    actions {
        resolve_dstUdp;
        _drop;
    }
}

action resolve_dstTcp(ipAddr, tcpPort) {
    modify_field(ipv4.dstAddr, ipAddr);
    modify_field(tcp.dstPort, tcpPort);
}

action resolve_dstUdp(ipAddr, udpPort) {
    modify_field(ipv4.dstAddr, ipAddr);
    modify_field(udp.dstPort, udpPort);
}

action _drop() {
    drop();
}

control ingress {
    apply(own_pool) {
        hit {
            if (valid(tcp)) {
                apply(nat_dl_tcp);
            }
            if (valid(udp)) {
                apply(nat_dl_udp);
            }
        }
        miss {
            if (valid(tcp)) {
                apply(nat_ul_tcp);
            }
            if (valid(udp)) {
                apply(nat_ul_udp);
            }
        }
    }
}

control egress {
}
