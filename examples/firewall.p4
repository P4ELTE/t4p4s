/* -*- P4_16 -*- */

#include "common-boilerplate-pre.p4"

const bit<16> TYPE_IPV4  = 0x0800;
const bit<16> TYPE_ARP   = 0x0806;
const bit<8>  PROTO_ICMP = 1;
const bit<8>  PROTO_TCP = 6;
const bit<8>  PROTO_UDP = 17;

// ARP RELATED CONSTS
const bit<16> ARP_HTYPE = 0x0001;    // Ethernet Hardware type is 1
const bit<16> ARP_PTYPE = TYPE_IPV4; // Protocol used for ARP is IPV4
const bit<8>  ARP_HLEN  = 6;         // Ethernet address size is 6 bytes
const bit<8>  ARP_PLEN  = 4;         // IP address size is 4 bytes
const bit<16> ARP_REQ = 1;           // Operation 1 is request
const bit<16> ARP_REPLY = 2;         // Operation 2 is reply

struct metadata {
    /* empty */
}

struct headers {
    ethernet_t   ethernet;
    arp_t        arp;
    ipv4_t       ipv4;
    tcp_t        tcp;
    udp_t        udp;
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
        transition select(hdr.arp.oper) {
          ARP_REQ: accept;
          default: accept;
      }
    }


    state parse_ipv4 {
      packet.extract(hdr.ipv4);
      transition select(hdr.ipv4.protocol) {
        PROTO_ICMP: parse_icmp;
        PROTO_TCP: parse_tcp;
        PROTO_UDP: parse_udp;

        default: accept;
      }
    }

    state parse_tcp {
      packet.extract(hdr.tcp);
      transition accept;
    }

    state parse_udp {
      packet.extract(hdr.udp);
      transition accept;
    }

    state parse_icmp {
      packet.extract(hdr.icmp);
      transition accept;
    }
}


CTL_MAIN {
    action drop() {
        MARK_TO_DROP();
    }

    table eth_dstMac_filter {
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }


    table eth_srcMac_filter {
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table eth_proto_filter {
        key = {
            hdr.ethernet.etherType: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table ip_proto_filter {
        key = {
            hdr.ipv4.protocol: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table ip_dstIP_filter {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table ip_srcIP_filter {
        key = {
            hdr.ipv4.srcAddr: lpm;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table tcp_srcPort_filter {
        key = {
            hdr.tcp.srcPort: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table tcp_dstPort_filter {
        key = {
            hdr.tcp.dstPort: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table udp_srcPort_filter {
        key = {
            hdr.udp.srcPort: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    table udp_dstPort_filter {
        key = {
            hdr.udp.dstPort: exact;
        }
        actions = {
            drop;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    apply {
        bit<8> dropped = 0;
        if (hdr.ethernet.isValid()){
          if (eth_srcMac_filter.apply().hit || eth_dstMac_filter.apply().hit || eth_proto_filter.apply().hit)
            dropped = 1;
          if (hdr.ipv4.isValid() && dropped == 0){
            if (ip_srcIP_filter.apply().hit || ip_dstIP_filter.apply().hit || ip_proto_filter.apply().hit)
              dropped = 1;
            if (hdr.tcp.isValid() && dropped == 0){
              if (tcp_srcPort_filter.apply().hit || tcp_dstPort_filter.apply().hit)
                dropped = 1;
            }
            else if (hdr.udp.isValid() && dropped == 0){
              if(udp_srcPort_filter.apply().hit && udp_dstPort_filter.apply().hit)
                dropped = 1;
            }
          }
          if (dropped != 1) {
            bit<PortId_size> in_port = (bit<PortId_size>)GET_EGRESS_PORT();
            bit<PortId_size> out_port = (in_port+1)%2;
            SET_EGRESS_PORT(out_port);
          }
        }
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.arp);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
        packet.emit(hdr.icmp);
    }
}

#include "common-boilerplate-post.p4"
