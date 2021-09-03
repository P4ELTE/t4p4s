// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    hdr1_t     h1;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action clashing_action_name() {
    }

    table clashing_table_name {
        actions = {
            clashing_action_name;
        }

        key = {
        }

        size = 1;

        default_action = clashing_action_name;
    }

    apply {
        clashing_table_name.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    action clashing_action_name() {
    }

    table clashing_table_name {
        actions = {
            clashing_action_name;
        }

        key = {
        }

        size = 1;

        default_action = clashing_action_name;
    }

    apply {
        clashing_table_name.apply();

        packet.emit(hdr.ethernet);
        packet.emit(hdr.h1);
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action clashing_action_name() {
    }

    table clashing_table_name {
        actions = {
            clashing_action_name;
        }

        key = {
        }

        size = 1;

        default_action = clashing_action_name;
    }

    apply {
        clashing_table_name.apply();
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    action clashing_action_name() {
    }

    table clashing_table_name {
        actions = {
            clashing_action_name;
        }

        key = {
        }

        size = 1;

        default_action = clashing_action_name;
    }

    apply {
        clashing_table_name.apply();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    action clashing_action_name() {
    }

    table clashing_table_name {
        actions = {
            clashing_action_name;
        }

        key = {
        }

        size = 1;

        default_action = clashing_action_name;
    }

    apply {
        clashing_table_name.apply();
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
