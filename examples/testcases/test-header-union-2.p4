#include "common-boilerplate-pre.p4"

header option_A_t {
  bit<8> a;
}
header option_B_t {
  bit<8> b;
}
header option_C_t {
  bit<8> c;
}
header_union options_t {
  option_A_t a;
  option_B_t b;
  option_C_t c;
}

struct metadata {
}

struct headers {
    options_t[10] option_stack;
}

PARSER {
    state parse_option_a {
        packet.extract(hdr.option_stack.next.a);
        transition start;
    }
    state parse_option_b {
        packet.extract(hdr.option_stack.next.b);
        transition start;
    }
    state parse_option_c {
        packet.extract(hdr.option_stack.next.c);
        transition start;
    }
    state end {
        transition accept;
    }
    state start {
        transition select(packet.lookahead<bit<2>>()) {
            2w0x0 : parse_option_a;
            2w0x1 : parse_option_b;
            2w0x2 : parse_option_c;
            2w0x3 : end;
        }
    }
}

CTL_MAIN {
    apply {
    }
}


CTL_EMIT {
    apply {
        option_A_t tmp = { 8w3 };
        if (hdr.option_stack[0].isValid() && hdr.option_stack[1].isValid() && hdr.option_stack[2].isValid() && !hdr.option_stack[3].isValid()) {
            packet.emit(tmp);
        }
    }
}

#include "common-boilerplate-post.p4"
