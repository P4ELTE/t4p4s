// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
    bits8_t    h8;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
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
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        clashing_table_name.apply();
    }
}

CTL_EMIT {
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
        packet.emit(hdr.h8);
    }
}

#define CUSTOM_CTL_EGRESS
CTL_EGRESS {
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

#define CUSTOM_CTL_CHECKSUM

CUSTOM_VERIFY_CHECKSUM {
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

CUSTOM_UPDATE_CHECKSUM {
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

#include "common-boilerplate-post.p4"
