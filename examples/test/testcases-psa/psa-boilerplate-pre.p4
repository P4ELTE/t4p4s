
#include <core.p4>
#include <bmv2/psa.p4>

#include "../../include/std_headers.p4"

struct empty_metadata_t {
}

#define PARSER parser IngressParserImpl(packet_in packet, \
                                        out headers hdr, \
                                        inout metadata meta, \
                                        in psa_ingress_parser_input_metadata_t istd, \
                                        in empty_metadata_t resubmit_meta, \
                                        in empty_metadata_t recirculate_meta)

#define CTL_EGRESS control egress(inout headers hdr, \
                                  inout metadata meta, \
                                  in    psa_egress_input_metadata_t  istd, \
                                  inout psa_egress_output_metadata_t ostd)

#define CTL_EMIT control IngressDeparserImpl(packet_out buffer, \
                                             out empty_metadata_t clone_i2e_meta, \
                                             out empty_metadata_t resubmit_meta, \
                                             out empty_metadata_t normal_meta, \
                                             inout headers hdr, \
                                             in metadata meta, \
                                             in psa_ingress_output_metadata_t istd)
