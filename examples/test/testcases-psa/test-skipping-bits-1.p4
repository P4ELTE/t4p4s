#include <core.p4>
#include <psa.p4>
#include "../../include/std_headers.p4"

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    bits16_t inhdr;
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state start {
        packet.extract<bits8_t>(_);
        packet.extract(hdr.inhdr);
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {
        if (hdr.inhdr.f16 == 0x0FF) {
            hdr.inhdr.setInvalid();
        } else {
            hdr.inhdr.f16 = hdr.inhdr.f16 + 1;
        }
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
        buffer.emit(hdr.inhdr);
    }
}

#include "psa-testcase-dummy-pipeline.p4"
