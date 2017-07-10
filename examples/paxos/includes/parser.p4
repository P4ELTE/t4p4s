#ifndef _PARSER_P4_
#define _PARSER_P4_

// Parser for ethernet, ipv4, and udp headers

#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IPV4 0x0800
#define UDP_PROTOCOL 0x11
#define IGMP_PROTOCOL 0x2
#define ETHERTYPE_IPV6 0x86dd
#define PAXOS_COORDINATOR 0x8888
#define PAXOS_ACCEPTOR 0x8889

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_ARP : parse_arp;
        ETHERTYPE_IPV4 : parse_ipv4; 
        ETHERTYPE_IPV6 : parse_ipv6;
        default : ingress;
    }
}

parser parse_arp {
    extract(arp);
    return ingress;
}

parser parse_ipv4 {
    extract(ipv4);
    return select(latest.protocol) {
        UDP_PROTOCOL : parse_udp;
        IGMP_PROTOCOL : parse_igmp;
        default : ingress;
    }
}

parser parse_ipv6 {
    extract(ipv6);
    return ingress;
}

parser parse_udp {
    extract(udp);
    return select(udp.dstPort) {
#if ENABLE_PAXOS
        PAXOS_COORDINATOR : parse_paxos;
        PAXOS_ACCEPTOR : parse_paxos;
#endif
        default: ingress;
    }
}


parser parse_igmp {
    extract(igmp);
    return ingress;
}

#endif