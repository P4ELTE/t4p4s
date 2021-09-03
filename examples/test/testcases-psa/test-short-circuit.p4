
#include "psa-boilerplate-pre.p4"

header dummy_t {
    bit<2> f1;
    bit<6> padding;
}

header dummy2_t  {
	bit<6> neverknown;
	bit<2> padding;
}

struct metadata {}

struct headers {
    dummy_t  dummy;
    dummy2_t dummy2;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}

CTL_EGRESS {
    apply {
	   hdr.dummy.f1 = 1;
	   bit<6> possibly_zero = hdr.dummy.padding;

	   if (hdr.dummy.f1 == 0 && ((6w1/possibly_zero)==0) && 6w1==hdr.dummy2.neverknown) {
			// NEVER RUNS
			hdr.dummy.f1 = 2;
	   } else {
			if (hdr.dummy.f1 == 1 || ((6w1/possibly_zero)==0)) {
                // ALWAYS RUNS
				hdr.dummy.f1 = 0;
			}
	   }

	   if (hdr.dummy2.isValid() && 6w1==hdr.dummy2.neverknown) {
			// NEVER RUNS
			hdr.dummy.f1 = 3;
	   }
	}
}


CTL_EMIT {
    apply {
        buffer.emit(hdr.dummy);
    }
}

#include "psa-boilerplate-post.p4"
