#define ETHERTYPE_IPV4 0x0800

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
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header ethernet_t ethernet;
header ipv4_t ipv4;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return parse_ipv4;
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

action alma(dstAddr) {
    modify_field(standard_metadata.egress_port, 22);
    modify_field(ipv4.srcAddr, dstAddr);
}

action korte() {
    modify_field(standard_metadata.egress_port, 33);
}

table table1 {
    reads {
        ipv4.dstAddr : lpm;
        ipv4.srcAddr : exact;
        ethernet.dstAddr: exact;
    }
    actions {
        alma;
        korte;
    }
    size : 512;
}

control ingress {
    apply(table1);
}

control egress {
}
