
#include "common-boilerplate-pre.p4"

enum bit<8> operation_e { LOOKUP_CONST_LPM4_ENTRY = 0, ADD_ENTRY = 1, LOOKUP_LPM4_ENTRY = 2 }

header op_t { operation_e op; }

struct metadata {
    bit<32>    content;
}

struct headers {
    op_t       op;
    bits8_t    size;

    pbits1_t   h1;
    pbits2_t   h2;
    pbits3_t   h3;
    pbits4_t   h4;
    pbits5_t   h5;
    pbits6_t   h6;
    pbits7_t   h7;
    bits8_t    h8;
    pbits9_t   h9;
    pbits10_t  h10;
    pbits11_t  h11;
    pbits12_t  h12;
    pbits13_t  h13;
    pbits14_t  h14;
    pbits15_t  h15;
    bits16_t   h16;
    pbits17_t  h17;
    pbits18_t  h18;
    pbits19_t  h19;
    pbits20_t  h20;
    pbits21_t  h21;
    pbits22_t  h22;
    pbits23_t  h23;
    pbits24_t  h24;
    pbits25_t  h25;
    pbits26_t  h26;
    pbits27_t  h27;
    pbits28_t  h28;
    pbits29_t  h29;
    pbits30_t  h30;
    pbits31_t  h31;

    bits32_t   ipv4;
    bits48_t   mac;

    bits8_t    result;
}

PARSER {
    state start {
        packet.extract(hdr.op);
        packet.extract(hdr.size);
        transition select(hdr.size.f8) {
          1:  parse1;
          2:  parse2;
          3:  parse3;
          4:  parse4;
          5:  parse5;
          6:  parse6;
          7:  parse7;
          8:  parse8;
          9:  parse9;
          10: parse10;
          11: parse11;
          12: parse12;
          13: parse13;
          14: parse14;
          15: parse15;
          16: parse16;
          17: parse17;
          18: parse18;
          19: parse19;
          20: parse20;
          21: parse21;
          22: parse22;
          23: parse23;
          24: parse24;
          25: parse25;
          26: parse26;
          27: parse27;
          28: parse28;
          29: parse29;
          30: parse30;
          31: parse31;

          32: parse_ipv4;
          48: parse_mac;
          default: reject;
        }
    }

    state parse1  { packet.extract(hdr.h1); transition accept; }
    state parse2  { packet.extract(hdr.h2); transition accept; }
    state parse3  { packet.extract(hdr.h3); transition accept; }
    state parse4  { packet.extract(hdr.h4); transition accept; }
    state parse5  { packet.extract(hdr.h5); transition accept; }
    state parse6  { packet.extract(hdr.h6); transition accept; }
    state parse7  { packet.extract(hdr.h7); transition accept; }
    state parse8  { packet.extract(hdr.h8); transition accept; }
    state parse9  { packet.extract(hdr.h9); transition accept; }
    state parse10 { packet.extract(hdr.h10); transition accept; }
    state parse11 { packet.extract(hdr.h11); transition accept; }
    state parse12 { packet.extract(hdr.h12); transition accept; }
    state parse13 { packet.extract(hdr.h13); transition accept; }
    state parse14 { packet.extract(hdr.h14); transition accept; }
    state parse15 { packet.extract(hdr.h15); transition accept; }
    state parse16 { packet.extract(hdr.h16); transition accept; }
    state parse17 { packet.extract(hdr.h17); transition accept; }
    state parse18 { packet.extract(hdr.h18); transition accept; }
    state parse19 { packet.extract(hdr.h19); transition accept; }
    state parse20 { packet.extract(hdr.h20); transition accept; }
    state parse21 { packet.extract(hdr.h21); transition accept; }
    state parse22 { packet.extract(hdr.h22); transition accept; }
    state parse23 { packet.extract(hdr.h23); transition accept; }
    state parse24 { packet.extract(hdr.h24); transition accept; }
    state parse25 { packet.extract(hdr.h25); transition accept; }
    state parse26 { packet.extract(hdr.h26); transition accept; }
    state parse27 { packet.extract(hdr.h27); transition accept; }
    state parse28 { packet.extract(hdr.h28); transition accept; }
    state parse29 { packet.extract(hdr.h29); transition accept; }
    state parse30 { packet.extract(hdr.h30); transition accept; }
    state parse31 { packet.extract(hdr.h31); transition accept; }

    state parse_ipv4 { packet.extract(hdr.ipv4); transition accept; }
    state parse_mac  { packet.extract(hdr.mac);  transition accept; }
}

#define lpm_table_bits(bits, good) \
    table lpm##bits## { \
        key = { hdr.h##bits##.f##bits## : lpm; } \
        actions = { set_result; } \
        size = 4; \
        const entries = { (##good##): set_result(1); } \
        default_action = set_result(0xFF); \
    }

CTL_MAIN {
    action set_result(bit<8> value) {
        hdr.result.f8 = value;
    }

    // Note: it would be more elegant to have a table for each bit size,
    //       but 32 LPM tables would require about 6GB of memory.
    table lpm32 {
        key = { meta.content : lpm; }
        actions = { set_result; }
        size = 4;
        const entries = { (0x0001): set_result(1); }
        default_action = set_result(0xFF);
    }

    table lpm_ipv4 {
        key = { hdr.ipv4.f32 : lpm; }
        actions = { set_result; }
        size = 4;

        const entries = {
            // "good" entries
            (0x00112233): set_result(1);
            (0x44556677): set_result(2);
            (0x32000a02): set_result(3);
        }

        default_action = set_result(0xFF);
    }

    bool tmp_bool = true;

    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());

        hdr.result.setValid();

        if (hdr.size.f8 == 32) { lpm_ipv4.apply(); hdr.ipv4.setInvalid(); }
        // else if (hdr.size.f8 == 48) { lpm_mac.apply();  hdr.mac.setInvalid(); }
        else {
            if (hdr.size.f8 == 1)  { meta.content = (bit<32>)hdr.h1.f1; hdr.h1.setInvalid(); }
            if (hdr.size.f8 == 2)  { meta.content = (bit<32>)hdr.h2.f2; hdr.h2.setInvalid(); }
            if (hdr.size.f8 == 3)  { meta.content = (bit<32>)hdr.h3.f3; hdr.h3.setInvalid(); }
            if (hdr.size.f8 == 4)  { meta.content = (bit<32>)hdr.h4.f4; hdr.h4.setInvalid(); }
            if (hdr.size.f8 == 5)  { meta.content = (bit<32>)hdr.h5.f5; hdr.h5.setInvalid(); }
            if (hdr.size.f8 == 6)  { meta.content = (bit<32>)hdr.h6.f6; hdr.h6.setInvalid(); }
            if (hdr.size.f8 == 7)  { meta.content = (bit<32>)hdr.h7.f7; hdr.h7.setInvalid(); }
            if (hdr.size.f8 == 8)  { meta.content = (bit<32>)hdr.h8.f8; hdr.h8.setInvalid(); }
            if (hdr.size.f8 == 9)  { meta.content = (bit<32>)hdr.h9.f9; hdr.h9.setInvalid(); }
            if (hdr.size.f8 == 10) { meta.content = (bit<32>)hdr.h10.f10; hdr.h10.setInvalid(); }
            if (hdr.size.f8 == 11) { meta.content = (bit<32>)hdr.h11.f11; hdr.h11.setInvalid(); }
            if (hdr.size.f8 == 12) { meta.content = (bit<32>)hdr.h12.f12; hdr.h12.setInvalid(); }
            if (hdr.size.f8 == 13) { meta.content = (bit<32>)hdr.h13.f13; hdr.h13.setInvalid(); }
            if (hdr.size.f8 == 14) { meta.content = (bit<32>)hdr.h14.f14; hdr.h14.setInvalid(); }
            if (hdr.size.f8 == 15) { meta.content = (bit<32>)hdr.h15.f15; hdr.h15.setInvalid(); }
            if (hdr.size.f8 == 16) { meta.content = (bit<32>)hdr.h16.f16; hdr.h16.setInvalid(); }
            if (hdr.size.f8 == 17) { meta.content = (bit<32>)hdr.h17.f17; hdr.h17.setInvalid(); }
            if (hdr.size.f8 == 18) { meta.content = (bit<32>)hdr.h18.f18; hdr.h18.setInvalid(); }
            if (hdr.size.f8 == 19) { meta.content = (bit<32>)hdr.h19.f19; hdr.h19.setInvalid(); }
            if (hdr.size.f8 == 20) { meta.content = (bit<32>)hdr.h20.f20; hdr.h20.setInvalid(); }
            if (hdr.size.f8 == 21) { meta.content = (bit<32>)hdr.h21.f21; hdr.h21.setInvalid(); }
            if (hdr.size.f8 == 22) { meta.content = (bit<32>)hdr.h22.f22; hdr.h22.setInvalid(); }
            if (hdr.size.f8 == 23) { meta.content = (bit<32>)hdr.h23.f23; hdr.h23.setInvalid(); }
            if (hdr.size.f8 == 24) { meta.content = (bit<32>)hdr.h24.f24; hdr.h24.setInvalid(); }
            if (hdr.size.f8 == 25) { meta.content = (bit<32>)hdr.h25.f25; hdr.h25.setInvalid(); }
            if (hdr.size.f8 == 26) { meta.content = (bit<32>)hdr.h26.f26; hdr.h26.setInvalid(); }
            if (hdr.size.f8 == 27) { meta.content = (bit<32>)hdr.h27.f27; hdr.h27.setInvalid(); }
            if (hdr.size.f8 == 28) { meta.content = (bit<32>)hdr.h28.f28; hdr.h28.setInvalid(); }
            if (hdr.size.f8 == 29) { meta.content = (bit<32>)hdr.h29.f29; hdr.h29.setInvalid(); }
            if (hdr.size.f8 == 30) { meta.content = (bit<32>)hdr.h30.f30; hdr.h30.setInvalid(); }
            if (hdr.size.f8 == 31) { meta.content = (bit<32>)hdr.h31.f31; hdr.h31.setInvalid(); }

            lpm32.apply();
        }
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.result);
    }
}

#include "common-boilerplate-post.p4"
