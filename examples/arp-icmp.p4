
#include "common-boilerplate-pre.p4"

const bit<16> TYPE_IPV4  = 0x0800;
const bit<16> TYPE_ARP   = 0x0806;
const bit<8>  PROTO_ICMP = 1;

// ARP RELATED CONSTS
const bit<16> ARP_HTYPE = 0x0001;    // Ethernet Hardware type is 1
const bit<16> ARP_PTYPE = TYPE_IPV4; // Protocol used for ARP is IPV4
const bit<8>  ARP_HLEN  = 6;         // Ethernet address size is 6 bytes
const bit<8>  ARP_PLEN  = 4;         // IP address size is 4 bytes
const bit<16> ARP_REQ = 1;           // Operation 1 is request
const bit<16> ARP_REPLY = 2;         // Operation 2 is reply


/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

struct metadata {
    /* empty */
}

struct headers {
    ethernet_t   ethernet;
    arp_t        arp;
    arp_ipv4_t   arp_ipv4;
    ipv4_t       ipv4;
    icmp_t       icmp;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_ARP: parse_arp;
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_arp {
        packet.extract(hdr.arp);
        packet.extract(hdr.arp_ipv4);
        transition select(hdr.arp.oper) {
            ARP_REQ: accept;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            PROTO_ICMP: parse_icmp;
            default: accept;
        }
    }

    state parse_icmp {
        bit<32> versionihl = (bit<32>)hdr.ipv4.version << 4 + (bit<32>)hdr.ipv4.ihl;
        bit<32> n = (bit<32>) (hdr.ipv4.totalLen) - (bit<32>)(versionihl << 2);

        packet.extract(hdr.icmp, 8*n-64);
        transition accept;
    }
}

CTL_MAIN {
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action arp_reply(macAddr_t request_mac) {
        //update operation code from request to reply
        hdr.arp.oper = ARP_REPLY;

        //reply's dst_mac is the request's src mac
        hdr.arp_ipv4.tha = hdr.arp_ipv4.sha;

        //reply's dst_ip is the request's src ip
        hdr.arp_ipv4.sha = request_mac;

        //reply's src ip is the request's dst ip
        hdr.arp_ipv4.spa = hdr.arp_ipv4.tpa;

        //update ethernet header
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = request_mac;

        //send it back to the same port
        standard_metadata.egress_port = standard_metadata.ingress_port;
    }


    action icmp_reply() {
        //set ICMP type to Echo reply
        hdr.icmp.type = 0;

        //for checksum calculation this field should be zero
        hdr.icmp.checksum = 0;

        //swap the source and destination IP addresses
    	bit<32> tmp_ip = hdr.ipv4.srcAddr;
        hdr.ipv4.srcAddr = hdr.ipv4.dstAddr;
        hdr.ipv4.dstAddr = tmp_ip;

        //swap the source and destination MAC addresses
        bit<48> tmp_mac = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_mac;

        //send it back to the same port
        standard_metadata.egress_port = standard_metadata.ingress_port;
    }


    // ARP table implements an ARP responder
    table arp_exact {
        key = {
            hdr.arp_ipv4.tpa: exact;
        }
        actions = {
            arp_reply;
            drop;
        }
        size = 1024;
        default_action = drop;
    }

    // replies to ICMP echo requests if the destination IP matches
    table icmp_responder {
        key = {
            hdr.ethernet.dstAddr: exact;
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            icmp_reply;
            drop;
        }
        size = 1024;
        default_action = drop();
    }

    apply {
        if (hdr.arp.isValid()){
            arp_exact.apply();
        } else if (hdr.icmp.isValid()) {
          icmp_responder.apply();
        } else {
          drop();
        }
    }
}

#define CUSTOM_CTL_CHECKSUM 1

CTL_VERIFY_CHECKSUM {
    apply {  }
}

CTL_UPDATE_CHECKSUM {
    apply {
        CALL_UPDATE_CHECKSUM(hdr.icmp.isValid(),
            ({
                hdr.icmp.type,
                hdr.icmp.code,
                16w0,
                hdr.icmp.identifier,
                hdr.icmp.sequence_number,
                hdr.icmp.padding
            }),
            hdr.icmp.checksum,
            HashAlgorithm.csum16);

        CALL_UPDATE_CHECKSUM(
            hdr.ipv4.isValid(),
            ({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr
            }),
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16);
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.arp);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.icmp);
    }
}

#include "common-boilerplate-post.p4"
