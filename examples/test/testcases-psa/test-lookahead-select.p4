
#include "psa-boilerplate-pre.p4"

struct metadata {
}

header outhdr_t {
    padded1_t  f1;
    padded3_t  f3;
    padded7_t  f7;
    padded8_t  f8;
    padded9_t  f9;
    padded16_t f16;
    padded17_t f17;
    padded32_t f32;
}

struct headers {
    outhdr_t outhdr;
}

PARSER {
    state start {
        hdr.outhdr.setValid();
        hdr.outhdr.f1.f1 = 0;
        hdr.outhdr.f3.f3 = 0;
        hdr.outhdr.f7.f7 = 0;
        hdr.outhdr.f8.f8 = 0;
        hdr.outhdr.f9.f9 = 0;
        hdr.outhdr.f16.f16 = 0;
        hdr.outhdr.f17.f17 = 0;
        hdr.outhdr.f32.f32 = 0;

        transition look1;
    }

    state done {
        packet.advance(32);
        transition accept;
    }

    state look1 {
        transition select(packet.lookahead<bit<1>>()) {
            1w1:     got1_1;
            default: got1_default;
        }
    }

    state look3 {
        transition select(packet.lookahead<bit<3>>()) {
            3w1:     got3_1;
            default: got3_default;
        }
    }

    state look7 {
        transition select(packet.lookahead<bit<7>>()) {
            7w1:     got7_1;
            default: got7_default;
        }
    }

    state look8 {
        transition select(packet.lookahead<bit<8>>()) {
            8w1:     got8_1;
            default: got8_default;
        }
    }

    state look9 {
        transition select(packet.lookahead<bit<9>>()) {
            9w1:     got9_1;
            default: got9_default;
        }
    }

    state look16 {
        transition select(packet.lookahead<bit<16>>()) {
            16w1:    got16_1;
            default: got16_default;
        }
    }

    state look17 {
        transition select(packet.lookahead<bit<17>>()) {
            17w1:    got17_1;
            default: got17_default;
        }
    }

    state look32 {
        transition select(packet.lookahead<bit<32>>()) {
            32w1:    got32_1;
            default: got32_default;
        }
    }

    state got1_1       { hdr.outhdr.f1.f1   = 1; transition look3;  }
    state got3_1       { hdr.outhdr.f3.f3   = 1; transition look7;  }
    state got7_1       { hdr.outhdr.f7.f7   = 1; transition look8;  }
    state got8_1       { hdr.outhdr.f8.f8   = 1; transition look9;  }
    state got9_1       { hdr.outhdr.f9.f9   = 1; transition look16; }
    state got16_1      { hdr.outhdr.f16.f16 = 1; transition look17; }
    state got17_1      { hdr.outhdr.f17.f17 = 1; transition look32; }
    state got32_1      { hdr.outhdr.f32.f32 = 1; transition done;   }

    state got1_default  { hdr.outhdr.f1.f1   = packet.lookahead<bits1_t>().f1;   transition look3;  }
    state got3_default  { hdr.outhdr.f3.f3   = packet.lookahead<bits3_t>().f3;   transition look7;  }
    state got7_default  { hdr.outhdr.f7.f7   = packet.lookahead<bits7_t>().f7;   transition look8;  }
    state got8_default  { hdr.outhdr.f8.f8   = packet.lookahead<bits8_t>().f8;   transition look9;  }
    state got9_default  { hdr.outhdr.f9.f9   = packet.lookahead<bits9_t>().f9;   transition look16; }
    state got16_default { hdr.outhdr.f16.f16 = packet.lookahead<bits16_t>().f16; transition look17; }
    state got17_default { hdr.outhdr.f17.f17 = packet.lookahead<bits17_t>().f17; transition look32; }
    state got32_default { hdr.outhdr.f32.f32 = packet.lookahead<bits32_t>().f32; transition done;   }
}

CTL_EGRESS {
    apply {}
}

CTL_EMIT {
    apply {
        buffer.emit(hdr.outhdr);
    }
}

#include "psa-boilerplate-post.p4"
