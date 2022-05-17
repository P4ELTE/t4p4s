
#include "common-boilerplate-pre.p4"

struct metadata {
}

header dummy_t {
    bit<1> f1;
    bit<1> f2;
    bit<2> f3;
    bit<2> f4;
    bit<2> f5;

    padded1_t f1b;
    padded1_t f2b;
    padded2_t f3b;
    padded2_t f4b;
    padded2_t f5b;
}

struct headers {
    dummy_t dummy;
}

bit<1> max1(in bit<1> left, in bit<1> right) {
   if (left > right)
      return left;
   return right;
}

bit<2> max2(in bit<2> left, in bit<2> right) {
   if (left > right)
      return left;
   return right;
}

bit<1> max1b(in bit<1> left, in bit<1> right) {
   return (left > right) ? left : right;
}

bit<2> max2b(in bit<2> left, in bit<2> right) {
   return (left > right) ? left : right;
}

PARSER {
    state start {
        packet.extract(hdr.dummy);
        transition accept;
    }
}


CTL_MAIN {
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        hdr.dummy.f1 = max1(hdr.dummy.f1, 1w1);
        hdr.dummy.f2 = max1(1w1, hdr.dummy.f2);
        hdr.dummy.f3 = max2(2w3, 2w0);
        hdr.dummy.f4 = max2(2w1, 2w2);
        hdr.dummy.f5 = max2(hdr.dummy.f4, 2w3);

        hdr.dummy.f1b.f1 = max1(hdr.dummy.f1b.f1, 1w1);
        hdr.dummy.f2b.f1 = max1(1w1, hdr.dummy.f2b.f1);
        hdr.dummy.f3b.f2 = max2(2w3, 2w0);
        hdr.dummy.f4b.f2 = max2(2w1, 2w2);
        hdr.dummy.f5b.f2 = max2(hdr.dummy.f4b.f2, 2w3);
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
