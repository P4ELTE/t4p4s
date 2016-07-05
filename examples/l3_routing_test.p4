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

/*
#define IPV4_LPM_TABLE_SIZE                    16384
#define REWRITE_MAC_TABLE_SIZE                 32768
*/
action on_miss() {
}

action fib_hit_nexthop(dmac, port) {
    modify_field(ethernet.dstAddr, dmac);
    modify_field(standard_metadata.egress_port, port);
    add_to_field(ipv4.ttl, -1);
}


table ipv4_fib_lpm {
    reads {
        ipv4.dstAddr : lpm;
    }
    actions {
        on_miss;
        fib_hit_nexthop;
    }
    size : 512;
}


action rewrite_src_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
}

table sendout {
    reads {
	standard_metadata.egress_port : exact;
    }
    actions {
        on_miss;
        rewrite_src_mac;
    }
    size : 512;
}

control ingress {
        /* fib lookup, set dst mac and standard_metadata.egress_port */
        apply(ipv4_fib_lpm);

	/* set smac from standard_metadata.egress_port */
	apply(sendout);
}

control egress {
}
