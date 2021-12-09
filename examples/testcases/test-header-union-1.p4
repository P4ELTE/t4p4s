#include "common-boilerplate-pre.p4"

header A {
  bit<8> a;
}
header B {
  bit<8> b;
}
header_union AB {
  A a;
  B b;
}

struct metadata {
}

struct headers {
    AB dummy;
    A dummy2;
    A dummy3;
}
PARSER {
    state start {
        packet.extract(hdr.dummy2);
        packet.extract(hdr.dummy3);
        transition accept;
    }
}

CTL_MAIN {
    apply {
        AB u;
        A my_a = { 8w1 };
        u.a = my_a;
        hdr.dummy2 = u.a;

        AB u2;
        A my_a2 = { 8w1 };
        my_a2.setInvalid();
        u.a = my_a;
        hdr.dummy3 = u.a;    
    }
}


CTL_EMIT {
    apply {
        packet.emit(hdr.dummy2);
        packet.emit(hdr.dummy3);
    }
}

#include "common-boilerplate-post.p4"
