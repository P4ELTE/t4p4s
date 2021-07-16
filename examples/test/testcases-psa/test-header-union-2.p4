#include <core.p4>
#include <bmv2/psa.p4>

// In: 00000000000000010000001000000011
// Out: 00000011

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

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    options_t[10] option_stack;
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
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
    apply { }
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
        option_A_t tmp = { 8w3 };
        if (hdr.option_stack[0].isValid() && hdr.option_stack[1].isValid() && hdr.option_stack[2].isValid() && !hdr.option_stack[3].isValid()) {
            buffer.emit(tmp);
        }
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
