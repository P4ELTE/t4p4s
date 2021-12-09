#include "common-boilerplate-pre.p4"

enum Suits { Clubs, Diamonds, Hearths, Spades }

enum bit<2> Choice {
  A      = 0x3,
  AA     = 0x3,
  B      = 0x1
}

header dummy_t {
    Choice f1;
    Choice f2;
    bit<2> f3;
    bit<1> f4;
    Choice f5;
    bit<7> padding;
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
    Choice tmp = Choice.A;
    Suits tmp2 = Suits.Clubs;
    
    apply {
       hdr.dummy.f1 = tmp;
       hdr.dummy.f2 = (Choice)2w0x3;
       hdr.dummy.f3 = hdr.dummy.f3 + (bit<2>)Choice.AA;
       hdr.dummy.f4 = (tmp2==Suits.Diamonds)?1w0:1w1;
       if (hdr.dummy.f5!=Choice.A && hdr.dummy.f5!=Choice.AA && hdr.dummy.f5!=Choice.B) {
          hdr.dummy.f5 = Choice.AA;
       }
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy);
    }
}

#include "common-boilerplate-post.p4"
