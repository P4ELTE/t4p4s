
#include "psa-boilerplate-pre.p4"

struct metadata {
}

#ifdef TEST_CONST_ENTRIES
    #define ENABLED_BITS TEST_CONST_ENTRIES
#else
    #define ENABLED_BITS 0xFFFFFFFF
#endif

header outhdr_t {
    #if (ENABLED_BITS & (1 << 0)) != 0
        padded1_t  p1;
    #endif
    #if (ENABLED_BITS & (1 << 1)) != 0
        padded2_t  p2;
    #endif
    #if (ENABLED_BITS & (1 << 2)) != 0
        padded3_t  p3;
    #endif
    #if (ENABLED_BITS & (1 << 3)) != 0
        padded4_t  p4;
    #endif
    #if (ENABLED_BITS & (1 << 4)) != 0
        padded5_t  p5;
    #endif
    #if (ENABLED_BITS & (1 << 5)) != 0
        padded6_t  p6;
    #endif
    #if (ENABLED_BITS & (1 << 6)) != 0
        padded7_t  p7;
    #endif
    #if (ENABLED_BITS & (1 << 7)) != 0
        padded8_t  p8;
    #endif
    #if (ENABLED_BITS & (1 << 8)) != 0
        padded9_t  p9;
    #endif
    #if (ENABLED_BITS & (1 << 9)) != 0
        padded10_t p10;
    #endif
    #if (ENABLED_BITS & (1 << 10)) != 0
        padded11_t p11;
    #endif
    #if (ENABLED_BITS & (1 << 11)) != 0
        padded12_t p12;
    #endif
    #if (ENABLED_BITS & (1 << 12)) != 0
        padded13_t p13;
    #endif
    #if (ENABLED_BITS & (1 << 13)) != 0
        padded14_t p14;
    #endif
    #if (ENABLED_BITS & (1 << 14)) != 0
        padded15_t p15;
    #endif
    #if (ENABLED_BITS & (1 << 15)) != 0
        padded16_t p16;
    #endif
    #if (ENABLED_BITS & (1 << 16)) != 0
        padded17_t p17;
    #endif
    #if (ENABLED_BITS & (1 << 17)) != 0
        padded18_t p18;
    #endif
    #if (ENABLED_BITS & (1 << 18)) != 0
        padded19_t p19;
    #endif
    #if (ENABLED_BITS & (1 << 19)) != 0
        padded20_t p20;
    #endif
    #if (ENABLED_BITS & (1 << 20)) != 0
        padded21_t p21;
    #endif
    #if (ENABLED_BITS & (1 << 21)) != 0
        padded22_t p22;
    #endif
    #if (ENABLED_BITS & (1 << 22)) != 0
        padded23_t p23;
    #endif
    #if (ENABLED_BITS & (1 << 23)) != 0
        padded24_t p24;
    #endif
    #if (ENABLED_BITS & (1 << 24)) != 0
        padded25_t p25;
    #endif
    #if (ENABLED_BITS & (1 << 25)) != 0
        padded26_t p26;
    #endif
    #if (ENABLED_BITS & (1 << 26)) != 0
        padded27_t p27;
    #endif
    #if (ENABLED_BITS & (1 << 27)) != 0
        padded28_t p28;
    #endif
    #if (ENABLED_BITS & (1 << 28)) != 0
        padded29_t p29;
    #endif
    #if (ENABLED_BITS & (1 << 29)) != 0
        padded30_t p30;
    #endif
    #if (ENABLED_BITS & (1 << 30)) != 0
        padded31_t p31;
    #endif
    #if (ENABLED_BITS & (1 << 31)) != 0
        padded32_t p32;
    #endif
}

struct headers {
    outhdr_t outhdr;
}

PARSER {
    state start {
        hdr.outhdr.setValid();

        #if (ENABLED_BITS & (1 << 0)) != 0
            hdr.outhdr.p1.f1  = packet.lookahead<bits1_t>().f1;   hdr.outhdr.p1.pad1 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 1)) != 0
            hdr.outhdr.p2.f2  = packet.lookahead<bits2_t>().f2;   hdr.outhdr.p2.pad2 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 2)) != 0
            hdr.outhdr.p3.f3  = packet.lookahead<bits3_t>().f3;   hdr.outhdr.p3.pad3 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 3)) != 0
            hdr.outhdr.p4.f4  = packet.lookahead<bits4_t>().f4;   hdr.outhdr.p4.pad4 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 4)) != 0
            hdr.outhdr.p5.f5  = packet.lookahead<bits5_t>().f5;   hdr.outhdr.p5.pad5 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 5)) != 0
            hdr.outhdr.p6.f6  = packet.lookahead<bits6_t>().f6;   hdr.outhdr.p6.pad6 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 6)) != 0
            hdr.outhdr.p7.f7  = packet.lookahead<bits7_t>().f7;   hdr.outhdr.p7.pad7 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 7)) != 0
            hdr.outhdr.p8.f8  = packet.lookahead<bits8_t>().f8;
        #endif
        #if (ENABLED_BITS & (1 << 8)) != 0
            hdr.outhdr.p9.f9  = packet.lookahead<bits9_t>().f9;   hdr.outhdr.p9.pad9 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 9)) != 0
            hdr.outhdr.p10.f10  = packet.lookahead<bits10_t>().f10;   hdr.outhdr.p10.pad10 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 10)) != 0
            hdr.outhdr.p11.f11  = packet.lookahead<bits11_t>().f11;   hdr.outhdr.p11.pad11 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 11)) != 0
            hdr.outhdr.p12.f12  = packet.lookahead<bits12_t>().f12;   hdr.outhdr.p12.pad12 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 12)) != 0
            hdr.outhdr.p13.f13  = packet.lookahead<bits13_t>().f13;   hdr.outhdr.p13.pad13 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 13)) != 0
            hdr.outhdr.p14.f14  = packet.lookahead<bits14_t>().f14;   hdr.outhdr.p14.pad14 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 14)) != 0
            hdr.outhdr.p15.f15  = packet.lookahead<bits15_t>().f15;   hdr.outhdr.p15.pad15 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 15)) != 0
            hdr.outhdr.p16.f16  = packet.lookahead<bits16_t>().f16;
        #endif
        #if (ENABLED_BITS & (1 << 16)) != 0
            hdr.outhdr.p17.f17  = packet.lookahead<bits17_t>().f17;   hdr.outhdr.p17.pad17 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 17)) != 0
            hdr.outhdr.p18.f18  = packet.lookahead<bits18_t>().f18;   hdr.outhdr.p18.pad18 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 18)) != 0
            hdr.outhdr.p19.f19  = packet.lookahead<bits19_t>().f19;   hdr.outhdr.p19.pad19 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 19)) != 0
            hdr.outhdr.p20.f20  = packet.lookahead<bits20_t>().f20;   hdr.outhdr.p20.pad20 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 20)) != 0
            hdr.outhdr.p21.f21  = packet.lookahead<bits21_t>().f21;   hdr.outhdr.p21.pad21 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 21)) != 0
            hdr.outhdr.p22.f22  = packet.lookahead<bits22_t>().f22;   hdr.outhdr.p22.pad22 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 22)) != 0
            hdr.outhdr.p23.f23  = packet.lookahead<bits23_t>().f23;   hdr.outhdr.p23.pad23 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 23)) != 0
            hdr.outhdr.p24.f24  = packet.lookahead<bits24_t>().f24;   hdr.outhdr.p24.pad24 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 24)) != 0
            hdr.outhdr.p25.f25  = packet.lookahead<bits25_t>().f25;   hdr.outhdr.p25.pad25 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 25)) != 0
            hdr.outhdr.p26.f26  = packet.lookahead<bits26_t>().f26;   hdr.outhdr.p26.pad26 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 26)) != 0
            hdr.outhdr.p27.f27  = packet.lookahead<bits27_t>().f27;   hdr.outhdr.p27.pad27 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 27)) != 0
            hdr.outhdr.p28.f28  = packet.lookahead<bits28_t>().f28;   hdr.outhdr.p28.pad28 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 28)) != 0
            hdr.outhdr.p29.f29  = packet.lookahead<bits29_t>().f29;   hdr.outhdr.p29.pad29 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 29)) != 0
            hdr.outhdr.p30.f30  = packet.lookahead<bits30_t>().f30;   hdr.outhdr.p30.pad30 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 30)) != 0
            hdr.outhdr.p31.f31  = packet.lookahead<bits31_t>().f31;   hdr.outhdr.p31.pad31 = 0;
        #endif
        #if (ENABLED_BITS & (1 << 31)) != 0
            hdr.outhdr.p32.f32  = packet.lookahead<bits32_t>().f32;
        #endif

        packet.advance(32);
        transition accept;
    }
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
