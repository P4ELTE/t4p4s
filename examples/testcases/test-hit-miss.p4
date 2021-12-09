#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<2> f1;
    bit<2> f2;
    bit<4> padding;
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_MAIN {
    table dummy_table {
        key = {}
        actions = {}
    }

    apply {
	if (dummy_table.apply().hit) {
        	hdr.dummy.f1 = (bit<2>)0;
        } else {
       		hdr.dummy.f1 = (bit<2>)3;
        }

        if (dummy_table.apply().miss) {
        	hdr.dummy.f2 = (bit<2>)3;
        } else {
       		hdr.dummy.f2 = (bit<2>)0;
        }
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
