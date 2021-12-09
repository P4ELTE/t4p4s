#include "common-boilerplate-pre.p4"

header dummy_t {
    bit<1> f11;
    bit<1> f12;
    bit<3> f14;
    bit<3> padding;
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
    bit tmp_bit = 1;
    bit<2> tmp_bit2 = 2w3;
    
    apply {
       
       //note: saturating arithmetic is recognized by T4P4S but treated as regular arithmetic
       hdr.dummy.f11 = (hdr.dummy.f11 |-| tmp_bit) + tmp_bit;
       hdr.dummy.f12 = (hdr.dummy.f12 + tmp_bit) |+| tmp_bit;
       
       // note: shifting by 123 does not make sense, as it is longer than f14 (the spec defines this special case)
       hdr.dummy.f14[0:0] = (bit<1>)((hdr.dummy.f14 >> 123) == (bit<3>)0);

       // note: currently the partial update of a field is not supported
       hdr.dummy.f14[0:0] = (bit<1>)((hdr.dummy.f14 >> 1) == (bit<3>)0);
       hdr.dummy.f14[2:1] = (((hdr.dummy.f14 ++ (bit<3>)7) >> 1) << 2)[3:2];
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
