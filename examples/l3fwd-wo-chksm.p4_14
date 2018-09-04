header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type arp_t {
    fields {
	hardware_type : 16;
	protocol_type : 16;
	HLEN          : 8;   /* hardware address length */
	PLEN          : 8;   /* protocol address length */
	OPER          : 16; 
	sender_ha     : 48;  /* ha = hardware address */ 
	sender_ip     : 32;
	target_ha     : 48;
	target_ip     : 32;
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
#define ETHERTYPE_ARP  0x0806

header ethernet_t ethernet;
header arp_t arp;
header ipv4_t ipv4;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        ETHERTYPE_ARP  : parse_arp;
        default: ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

parser parse_arp {
    extract(arp);
    return ingress;
}

header_type routing_metadata_t {
    fields {
        nhgroup : 32;
    }
}

metadata routing_metadata_t routing_metadata;

action set_nhop(nhgroup) {
    modify_field(routing_metadata.nhgroup, nhgroup);
}

action _drop() {
    drop();
}

action _nop() {
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
    subtract_from_field(ipv4.ttl, 1);
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

table macfwd {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        _nop;
        _drop;
    }
    size: 256;
}

control ingress {
       apply(macfwd) {
          hit {
               apply(ipv4_lpm);
               apply(nexthops);
          }
       }
}

control egress {
}
