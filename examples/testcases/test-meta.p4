#include "common-boilerplate-pre.p4"

header in1_t {
    bit<7> padding;
    bool f1;
}

header set_after_meta_cond_t {
    bit<7> padding;
    bool f1;
}

header meta_set_in_action_t {
    bit<7> padding;
    bool f1;
}

header bit1_set_in_action_t {
    bit<7> padding;
    bit<1> f1;
}

struct metadata {
    bit<1> send;
}

struct headers {
    in1_t                 in1;
    set_after_meta_cond_t out1;
    meta_set_in_action_t  out2;
    bit1_set_in_action_t  out3;
}

PARSER {
    state start {
        packet.extract(hdr.in1);
        packet.extract(hdr.out1);
        packet.extract(hdr.out2);
        packet.extract(hdr.out3);
        transition accept;
    }
}

CTL_MAIN {
    action set_f1() {
        meta.send = 1;
    }

    apply {
        set_f1();

        if (meta.send == 1) {
            hdr.out1.f1 = true;
        }

        hdr.out2.f1 = (bool)meta.send;
        hdr.out3.f1 = meta.send;

        hdr.in1.setInvalid();

        SET_EGRESS_PORT(GET_INGRESS_PORT());
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.out1);
        packet.emit(hdr.out2);
        packet.emit(hdr.out3);
    }
}

#include "common-boilerplate-post.p4"
