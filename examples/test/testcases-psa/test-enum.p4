#include <core.p4>
#include <bmv2/psa.p4>

// In: 0000000000000000
// Out: 1111111110000000

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

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state parse_ethernet {
        packet.extract(hdr.dummy);
        transition accept;
    }
    state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
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
        buffer.emit(hdr.dummy);
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
