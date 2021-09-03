
#include "psa-boilerplate-pre.p4"

struct metadata {
}

struct data {
    bit<1> first;
    bit<1> second;
    bit<1> third;
}

header empty_t { }

header dummy_t {
    bit<1> f1;
    data   f2;
    bit<4> padding;
}

struct headers {
    empty_t empty;
    dummy_t[134] dummy;
}

PARSER {
    state start {
        packet.extract(hdr.empty);
        packet.extract(hdr.dummy.next);
        packet.extract(hdr.dummy.next);
        packet.extract(hdr.dummy.next);
        packet.extract(hdr.dummy.next);
        transition accept;
    }
}

CTL_EGRESS {
    apply {
       // data d = {~hdr.dummy[1].f2.second, ~hdr.dummy[1].f2.second, ~hdr.dummy[1].f2.second};
       hdr.dummy[1].f1 = (bit<1>)hdr.empty.isValid();
       // hdr.dummy[1].f2 = d;
       hdr.dummy[2].setInvalid();
       hdr.dummy.pop_front(1);
    }
}

CTL_EMIT {
    apply {
       buffer.emit(hdr.dummy);
    }
}

#include "psa-boilerplate-post.p4"
