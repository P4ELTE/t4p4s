header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        versionIhl : 8;
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

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        nhgroup : 32;
    }
}

metadata routing_metadata_t routing_metadata;

action set_nhop(portmask) {
    modify_field(routing_metadata.nhgroup, ipv4.srcAddr, portmask);
    add_to_field(ipv4.ttl, 0xff);
}

action _drop() {
    drop();
}

table ipv4_lpm {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
        set_nhop;
        _drop;
    }
    size: 1024;
}

action forward(dmac_val,smac_val,port) {
    modify_field(ethernet.dstAddr, dmac_val);
    modify_field(standard_metadata.egress_port, port);
    modify_field(ethernet.srcAddr, smac_val);
}

table nexthops {
    reads {
        routing_metadata.nhgroup : exact;
    }
    actions {
        forward;
        _drop;
    }
    size: 512;
}

control ingress {
       apply(ipv4_lpm);
       apply(nexthops);
}

control egress {
}
