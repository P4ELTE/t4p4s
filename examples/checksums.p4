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
        options: *;
    }
    length : ihl * 4;
    max_length : 60;
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
        ipv4.options;
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
        options : *;
    }
    length : dataOffset * 4;
    max_length : 60;
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
        tcp.options;
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

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol) {
        IP_PROT_TCP : parse_tcp;
        default : ingress;
    }
}

header tcp_t tcp;

parser parse_tcp {
    extract(tcp);
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
