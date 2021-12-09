
#pragma once

#include <core.p4>
#include <psa.p4>
#include "common_headers.p4"

struct empty_metadata_t {}

#define PARSER parser IngressParserImpl(packet_in packet, \
                                        out headers hdr, \
                                        inout metadata meta, \
                                        in psa_ingress_parser_input_metadata_t standard_metadata, \
                                        in empty_metadata_t resubmit_meta, \
                                        in empty_metadata_t recirculate_meta)

#define CTL_INGRESS control ingress(inout headers hdr, \
                                    inout metadata meta, \
                                    in    psa_ingress_input_metadata_t  istd, \
                                    inout psa_ingress_output_metadata_t standard_metadata)

#define CTL_EMIT control IngressDeparserImpl(packet_out packet, \
                                             out empty_metadata_t clone_i2e_meta, \
                                             out empty_metadata_t resubmit_meta, \
                                             out empty_metadata_t normal_meta, \
                                             inout headers hdr, \
                                             in metadata meta, \
                                             in psa_ingress_output_metadata_t standard_metadata)

#define PARSER2 parser EgressParserImpl(packet_in packet, \
                        out headers hdr, \
                        inout metadata meta, \
                        in psa_egress_parser_input_metadata_t istd, \
                        in empty_metadata_t normal_meta, \
                        in empty_metadata_t clone_i2e_meta, \
                        in empty_metadata_t clone_e2e_meta)

#define CTL_EGRESS control egress(inout headers hdr, \
                                  inout metadata meta, \
                                  in    psa_egress_input_metadata_t  istd, \
                                  inout psa_egress_output_metadata_t standard_metadata)

#define EMIT2 control EgressDeparserImpl(packet_out packet, \
                           out empty_metadata_t clone_e2e_meta, \
                           out empty_metadata_t recirculate_meta, \
                           inout headers hdr, \
                           in metadata meta, \
                           in psa_egress_output_metadata_t standard_metadata, \
                           in psa_egress_deparser_input_metadata_t edstd)


#define MARK_TO_DROP()            standard_metadata.drop = true

#define PortId_size       32
#define PortId_const(txt) 32w ## txt

#define GET_INGRESS_PORT()        istd.ingress_port
#define GET_EGRESS_PORT()         ((PortId_t)standard_metadata.egress_port)
#define SET_EGRESS_PORT(value)    standard_metadata.egress_port = (PortId_t)(value)

#define DECLARE_DIGEST(type, name)              Digest<type>() name;
#define CALL_DIGEST(type, name, port, content)  name.pack(content)


#define CTL_MAIN CTL_INGRESS
