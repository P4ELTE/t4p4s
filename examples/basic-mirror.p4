
#include "common-boilerplate-pre.p4"

const bit<16> TYPE_IPV4 = 0x800;

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

struct metadata {
    /* empty */
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

#define CUSTOM_CTL_CHECKSUM 1

CUSTOM_VERIFY_CHECKSUM {
    apply {  }
}

CUSTOM_UPDATE_CHECKSUM {
    DECLARE_CHECKSUM(bit<32>, CRC16, checksum_var)
    apply {
        CALL_UPDATE_CHECKSUM(
            checksum_var,
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
            csum16);
    }
}

CTL_MAIN {
    apply {
        if (hdr.ipv4.isValid()) {
            ip4Addr_t tmp = hdr.ipv4.dstAddr;
            hdr.ipv4.dstAddr = hdr.ipv4.srcAddr;
            hdr.ipv4.srcAddr = tmp;

            macAddr_t tmp2 = hdr.ethernet.dstAddr;
            hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
            hdr.ethernet.srcAddr = tmp2;

            SET_EGRESS_PORT(GET_INGRESS_PORT());

            CTL_VERIFY_CHECKSUM_APPLY();
            CTL_UPDATE_CHECKSUM_APPLY();
        }
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

#include "common-boilerplate-post.p4"
