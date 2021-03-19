#include <core.p4>
#include <psa.p4>

#include "../include/std_headers.p4"

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
        hdr.outhdr.f1.f1 = 0;
        hdr.outhdr.f3.f3 = 0;
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

    state got1_1       { hdr.outhdr.f1.f1  = 1; transition look3;  }
    state got3_1       { hdr.outhdr.f3.f3  = 1; transition look8;  }
    state got8_1       { hdr.outhdr.f8.f8  = 1; transition look9;  }
    state got9_1       { hdr.outhdr.f9.f9  = 1; transition look16; }
    state got16_1      { hdr.outhdr.f16.f16 = 1; transition look17; }
    state got17_1      { hdr.outhdr.f17.f17 = 1; transition look32; }
    state got32_1      { hdr.outhdr.f32.f32 = 1; transition done;   }

    state got1_default  { hdr.outhdr.f1.f1  = packet.lookahead<bits1_t>().f1;   transition look3;  }
    state got3_default  { hdr.outhdr.f3.f3  = packet.lookahead<bits3_t>().f3;   transition look8;  }
    state got8_default  { hdr.outhdr.f8.f8  = packet.lookahead<bits8_t>().f8;   transition look16; }
    state got9_default  { hdr.outhdr.f9.f9  = packet.lookahead<bits9_t>().f9;   transition look16; }
    state got16_default { hdr.outhdr.f16.f16 = packet.lookahead<bits16_t>().f16; transition look17; }
    state got17_default { hdr.outhdr.f17.f17 = packet.lookahead<bits17_t>().f17; transition look32; }
    state got32_default { hdr.outhdr.f32.f32 = packet.lookahead<bits32_t>().f32; transition done;   }
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
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control IngressDeparserImpl(packet_out buffer,
                            out empty_metadata_t clone_i2e_meta,
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
                           out empty_metadata_t clone_e2e_meta,
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
