#include <core.p4>
#include <psa.p4>

#include "../include/std_headers.p4"

header outhdr_t {
    bit<(8-1)>   align1;    bit<1>  f1;
    bit<(8-2)>   align2;    bit<2>  f2;
    bit<(8-3)>   align3;    bit<3>  f3;
    bit<(8-4)>   align4;    bit<4>  f4;
    bit<(8-5)>   align5;    bit<5>  f5;
    bit<(8-6)>   align6;    bit<6>  f6;
    bit<(8-7)>   align7;    bit<7>  f7;
                            bit<8>  f8;
    bit<(16-9)>  align9;    bit<9>  f9;
    bit<(16-10)> align10;   bit<10> f10;
    bit<(16-11)> align11;   bit<11> f11;
    bit<(16-12)> align12;   bit<12> f12;
    bit<(16-13)> align13;   bit<13> f13;
    bit<(16-14)> align14;   bit<14> f14;
    bit<(16-15)> align15;   bit<15> f15;
                            bit<16> f16;
    bit<(32-17)> align17;   bit<17> f17;
    bit<(32-18)> align18;   bit<18> f18;
    bit<(32-19)> align19;   bit<19> f19;
    bit<(32-20)> align20;   bit<20> f20;
    bit<(32-21)> align21;   bit<21> f21;
    bit<(32-22)> align22;   bit<22> f22;
    bit<(32-23)> align23;   bit<23> f23;
    bit<(32-24)> align24;   bit<24> f24;
    bit<(32-25)> align25;   bit<25> f25;
    bit<(32-26)> align26;   bit<26> f26;
    bit<(32-27)> align27;   bit<27> f27;
    bit<(32-28)> align28;   bit<28> f28;
    bit<(32-29)> align29;   bit<29> f29;
    bit<(32-30)> align30;   bit<30> f30;
    bit<(32-31)> align31;   bit<31> f31;
                            bit<32> f32;
}


struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    outhdr_t outhdr;
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state start {
        hdr.outhdr.setValid();

        hdr.outhdr.f1 = packet.lookahead<bits1_t>().f1;    hdr.outhdr.align1 = 0;
        hdr.outhdr.f2 = packet.lookahead<bits2_t>().f2;    hdr.outhdr.align2 = 0;
        hdr.outhdr.f3 = packet.lookahead<bits3_t>().f3;    hdr.outhdr.align3 = 0;
        hdr.outhdr.f4 = packet.lookahead<bits4_t>().f4;    hdr.outhdr.align4 = 0;
        hdr.outhdr.f5 = packet.lookahead<bits5_t>().f5;    hdr.outhdr.align5 = 0;
        hdr.outhdr.f6 = packet.lookahead<bits6_t>().f6;    hdr.outhdr.align6 = 0;
        hdr.outhdr.f7 = packet.lookahead<bits7_t>().f7;    hdr.outhdr.align7 = 0;
        hdr.outhdr.f8 = packet.lookahead<bits8_t>().f8;    // hdr.outhdr.align8 = 0;
        hdr.outhdr.f9 = packet.lookahead<bits9_t>().f9;    hdr.outhdr.align9 = 0;
        hdr.outhdr.f10 = packet.lookahead<bits10_t>().f10; hdr.outhdr.align10 = 0;
        hdr.outhdr.f11 = packet.lookahead<bits11_t>().f11; hdr.outhdr.align11 = 0;
        hdr.outhdr.f12 = packet.lookahead<bits12_t>().f12; hdr.outhdr.align12 = 0;
        hdr.outhdr.f13 = packet.lookahead<bits13_t>().f13; hdr.outhdr.align13 = 0;
        hdr.outhdr.f14 = packet.lookahead<bits14_t>().f14; hdr.outhdr.align14 = 0;
        hdr.outhdr.f15 = packet.lookahead<bits15_t>().f15; hdr.outhdr.align15 = 0;
        hdr.outhdr.f16 = packet.lookahead<bits16_t>().f16; // hdr.outhdr.align16 = 0;
        hdr.outhdr.f17 = packet.lookahead<bits17_t>().f17; hdr.outhdr.align17 = 0;
        hdr.outhdr.f18 = packet.lookahead<bits18_t>().f18; hdr.outhdr.align18 = 0;
        hdr.outhdr.f19 = packet.lookahead<bits19_t>().f19; hdr.outhdr.align19 = 0;
        hdr.outhdr.f20 = packet.lookahead<bits20_t>().f20; hdr.outhdr.align20 = 0;
        hdr.outhdr.f21 = packet.lookahead<bits21_t>().f21; hdr.outhdr.align21 = 0;
        hdr.outhdr.f22 = packet.lookahead<bits22_t>().f22; hdr.outhdr.align22 = 0;
        hdr.outhdr.f23 = packet.lookahead<bits23_t>().f23; hdr.outhdr.align23 = 0;
        hdr.outhdr.f24 = packet.lookahead<bits24_t>().f24; hdr.outhdr.align24 = 0;
        hdr.outhdr.f25 = packet.lookahead<bits25_t>().f25; hdr.outhdr.align25 = 0;
        hdr.outhdr.f26 = packet.lookahead<bits26_t>().f26; hdr.outhdr.align26 = 0;
        hdr.outhdr.f27 = packet.lookahead<bits27_t>().f27; hdr.outhdr.align27 = 0;
        hdr.outhdr.f28 = packet.lookahead<bits28_t>().f28; hdr.outhdr.align28 = 0;
        hdr.outhdr.f29 = packet.lookahead<bits29_t>().f29; hdr.outhdr.align29 = 0;
        hdr.outhdr.f30 = packet.lookahead<bits30_t>().f30; hdr.outhdr.align30 = 0;
        hdr.outhdr.f31 = packet.lookahead<bits31_t>().f31; hdr.outhdr.align31 = 0;
        hdr.outhdr.f32 = packet.lookahead<bits32_t>().f32; // hdr.outhdr.align32 = 0;

        packet.advance(32);
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {
    }
}


control ingress(inout headers hdr,
                inout metadata meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        ostd.egress_port = (PortId_t)12345;
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers hdr,
                        inout metadata meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t cllook_01_1_i2e_meta,
                        in empty_metadata_t cllook_01_1_e2e_meta)
{
    state start {
        transition accept;
    }
}

control IngressDeparserImpl(packet_out buffer,
                            out empty_metadata_t cllook_01_1_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers hdr,
                            in metadata meta,
                            in psa_ingress_output_metadata_t istd)
{
    apply {
        buffer.emit(hdr.outhdr);
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_metadata_t cllook_01_1_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers hdr,
                           in metadata meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
