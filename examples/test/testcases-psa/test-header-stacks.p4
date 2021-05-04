#include <core.p4>
#include <psa.p4>

struct data {
    bit<1> first;
    bit<1> second;
    bit<1> third;
}

header empty_t { }

header dummy_t {
    bit<1> f1;
    data f2;
    bit<4> padding;
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    empty_t empty;
    dummy_t[134] dummy;
}
parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state start {
        packet.extract(hdr.empty);
        packet.extract(hdr.dummy.next);
        packet.extract(hdr.dummy.next);
        packet.extract(hdr.dummy.next);
        packet.extract(hdr.dummy.next);
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {
       // data d = {~hdr.dummy[1].f2.second, ~hdr.dummy[1].f2.second, ~hdr.dummy[1].f2.second};
       hdr.dummy[1].f1 = (bit<1>)hdr.empty.isValid();
       // hdr.dummy[1].f2 = d;
       hdr.dummy[2].setInvalid();
       hdr.dummy.pop_front(1);
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

#include "psa-testcase-dummy-pipeline.p4"
