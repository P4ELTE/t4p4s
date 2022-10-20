
#pragma once

#include <core.p4>
#include <psa.p4>
#include "common_headers.p4"

struct empty_metadata_t {}

#define CTLIMPL_INGRESS(name) control name(inout headers hdr, \
                                           inout metadata meta, \
                                           in    psa_ingress_input_metadata_t  istd, \
                                           inout psa_ingress_output_metadata_t standard_metadata)

#define PARSER parser IngressParserImpl(packet_in packet, \
                                        out headers hdr, \
                                        inout metadata meta, \
                                        in psa_ingress_parser_input_metadata_t standard_metadata, \
                                        in empty_metadata_t resubmit_meta, \
                                        in empty_metadata_t recirculate_meta)

#define CTL_INGRESS            CTLIMPL_INGRESS(ingress)
#define CUSTOM_VERIFY_CHECKSUM CTLIMPL_INGRESS(CustomVerifyChecksumCtl)
#define CUSTOM_UPDATE_CHECKSUM CTLIMPL_INGRESS(CustomUpdateChecksumCtl)

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
#define GET_EGRESS_PORT_SIMPLE()  standard_metadata.egress_port
#define SET_EGRESS_PORT(value)    standard_metadata.egress_port = (PortId_t)(value)

#define DECLARE_DIGEST(type, name)              Digest<type>() name;
#define CALL_DIGEST(type, name, port, content)  name.pack(content)

#define DECLARE_RANDOM(type, varname, min, max)     Random<type>(min, max) varname;
#define RANDOM_READ(out, type, varname, min, max)   out = varname.read();

#define DECLARE_REGISTER_IDX(type, idxtype, amount, varname)    Register<type, idxtype>(amount) varname;
#define DECLARE_REGISTER(type, amount, varname)                 DECLARE_REGISTER_IDX(type, bit<32>, amount, varname)
#define REGISTER_WRITE(varname, idx, in)                        varname.write(idx, in)
#define REGISTER_READ(out, varname, idx)                        out = varname.read(idx)

#define METER_GREEN  PSA_MeterColor_t.GREEN
#define METER_YELLOW PSA_MeterColor_t.YELLOW
#define METER_RED    PSA_MeterColor_t.RED

// here, low/up refers to lowercase/uppercase
#define MeterColor_t(v1type)                                        PSA_MeterColor_t
#define MeterColor_value(v1value, psa_value)                        PSA_MeterColor_t.psa_value
#define DECLARE_METER(amount, elemtype, lowtype, uptype, varname)   Meter<elemtype>(amount, PSA_MeterType_t.uptype) varname
#define METER_EXECUTE(out, varname, idx)                            out = varname.execute(idx)
#define METER_EXECUTE_COLOR(out, varname, idx, color)               out = varname.execute(idx, color)

#define HASH(part1, part2, part3, part4, part5)            /* TODO */

#define LOGMSG(msg)         /* skipping logging */
#define LOG(fmt, content)   /* skipping logging */

#define CTL_VERIFY_CHECKSUM_APPLY() CustomVerifyChecksumCtl.apply(hdr, meta, istd, standard_metadata)
#define CTL_UPDATE_CHECKSUM_APPLY() CustomUpdateChecksumCtl.apply(hdr, meta, istd, standard_metadata)
#define DECLARE_CHECKSUM(type, psa_algo, varname)  Checksum<type>(PSA_HashAlgorithm_t.psa_algo) varname;
#define CALL_UPDATE_CHECKSUM(varname, condition, data, out, v1_algo)  checksum_var.update(data)

#define CTL_MAIN CTL_INGRESS
