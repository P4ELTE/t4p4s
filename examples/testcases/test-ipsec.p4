// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
    @name(".ipv4")
    ipv4_t     ipv4;
}

PARSER {
    @name(".start") state start {
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

extern void ipsec_encapsulate();

CTL_MAIN {
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        log_msg("diffserv    = {}", {hdr.ipv4.diffserv});
        log_msg("totalLen    = {}", {hdr.ipv4.totalLen});
        log_msg("identification    = {}", {hdr.ipv4.identification});
        log_msg("flags    = {}", {hdr.ipv4.flags});
        log_msg("fragOffset    = {}", {hdr.ipv4.fragOffset});
        log_msg("ttl    = {}", {hdr.ipv4.ttl});
        log_msg("protocol    = {}", {hdr.ipv4.protocol});
        log_msg("hdrChecksum    = {}", {hdr.ipv4.hdrChecksum});
        log_msg("srcAddr    = {}", {hdr.ipv4.srcAddr});
        log_msg("dstAddr    = {}", {hdr.ipv4.dstAddr});

        ipsec_encapsulate();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

#include "common-boilerplate-post.p4"
