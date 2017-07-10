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

field_list ipv4_checksum_list {
        ipv4.versionIhl;
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

action _drop() {
    drop();
}

action _nop() {
}

// -- L2 --

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
        mcast_hash : 16;
        lf_field_list: 32;
        fwd_mode : 8;
    }
}

metadata intrinsic_metadata_t intrinsic_metadata;

#define MAC_LEARN_RECEIVER 1024
#define BCAST_PORT 100

field_list mac_learn_digest {
    ethernet.srcAddr;
    standard_metadata.ingress_port;
}

action mac_learn() {
    generate_digest(MAC_LEARN_RECEIVER, mac_learn_digest);
}


table smac {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {mac_learn; _nop;}
    size : 512;
}

action broadcast() {
    modify_field(standard_metadata.egress_port, BCAST_PORT);
}

action forward(port) {
    modify_field(standard_metadata.egress_port, port);
}

table dmac {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {forward; broadcast;}
    size : 512;
}

// -- L3 --

header_type routing_metadata_t {
    fields {
        nhgroup : 32;
    }
}

metadata routing_metadata_t routing_metadata;

action set_nhop(nhgroup) {
    modify_field(routing_metadata.nhgroup, nhgroup);
    add_to_field(ipv4.ttl, 0xff);
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

action l3forward(dmac_val) {
    modify_field(ethernet.dstAddr, dmac_val);
}

table ipv4_forward {
    reads {
        routing_metadata.nhgroup : exact;
    }
    actions {
        l3forward;
        _drop;
    }
    size: 512;
}

action rewrite_mac(smac) {
    modify_field(ethernet.srcAddr, smac);
}

table send_frame {
    reads {
        standard_metadata.egress_port: exact;
    }
    actions {
       rewrite_mac;
       _nop;
    }
}

#define L2FWD 0

action set_fwd_mode(mode) {
    modify_field(intrinsic_metadata.fwd_mode, mode);
}

table dmac_classifier {
	reads {
		ethernet.dstAddr: exact;
	}
	actions {
		set_fwd_mode;
	}
}


control ingress {
    apply(smac);
    apply(dmac_classifier) {
        miss { // L3FWD
            apply(ipv4_lpm);
            apply(ipv4_forward);
        }
    }
    apply(dmac);
}

control egress {
    if (intrinsic_metadata.fwd_mode!=L2FWD) {
    	apply(send_frame);
    }
}
