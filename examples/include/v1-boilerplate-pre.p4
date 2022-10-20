
#pragma once

typedef bit<9> PortId_t;

#include <core.p4>
#include <v1model.p4>
#include "common_headers.p4"


#define CTLIMPL_CHECKSUM(name)     control name(inout headers hdr, inout metadata meta)

#define PARSER                  parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_VERIFY_CHECKSUM     CTLIMPL_CHECKSUM(verifyChecksum)
#define CUSTOM_VERIFY_CHECKSUM  CTLIMPL_CHECKSUM(CustomVerifyChecksumCtl)
#define CTL_INGRESS             control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_EGRESS              control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_UPDATE_CHECKSUM     CTLIMPL_CHECKSUM(computeChecksum)
#define CUSTOM_UPDATE_CHECKSUM  CTLIMPL_CHECKSUM(CustomUpdateChecksumCtl)
#define CTL_EMIT                control DeparserImpl(packet_out packet, in headers hdr)

#define MARK_TO_DROP()          mark_to_drop(standard_metadata)

#define PortId_size       9
#define PortId_const(txt) 9w ## txt

#define GET_INGRESS_PORT()        standard_metadata.ingress_port
#define GET_EGRESS_PORT()         standard_metadata.egress_port
#define GET_EGRESS_PORT_SIMPLE()  standard_metadata.egress_port
#define SET_EGRESS_PORT(value)    standard_metadata.egress_port = (PortId_t)(value);

#define DECLARE_DIGEST(type, name)              /* nothing to declare for v1model */
#define CALL_DIGEST(type, name, port, content)  digest<type>((bit<32>)port, content);

#define DECLARE_RANDOM(type, varname, min, max)     /* nothing to declare for v1model */
#define RANDOM_READ(out, type, varname, min, max)   random<type>(out, min, max);

#define DECLARE_REGISTER(type, amount, varname)     register<type>(amount) varname;
#define REGISTER_WRITE(varname, idx, in)            varname.write(idx, in)
#define REGISTER_READ(out, varname, idx)            varname.read(out, idx)

#define METER_GREEN  V1MODEL_METER_COLOR_GREEN
#define METER_YELLOW V1MODEL_METER_COLOR_YELLOW
#define METER_RED    V1MODEL_METER_COLOR_RED

// here, low/up refers to lowercase/uppercase
#define MeterColor_t(v1type)                                        v1type
#define MeterColor_value(v1value, psa_value)                        v1value
#define DECLARE_METER(amount, elemtype, lowtype, uptype, varname)   meter(amount, MeterType.lowtype) varname
#define METER_EXECUTE(out, varname, idx)                            varname.execute_meter(idx, out)

#define LOGMSG(msg)         log_msg(msg)
#define LOG(fmt, content)   log_msg(fmt, content)

#define HASH(part1, part2, part3, part4, part5)            hash(part1, part2, part3, part4, part5)

#define CTL_VERIFY_CHECKSUM_APPLY() /* nothing to do */
#define CTL_UPDATE_CHECKSUM_APPLY() /* nothing to do */
#define DECLARE_CHECKSUM(type, psa_algo, varname)  /* nothing to do */
#define CALL_UPDATE_CHECKSUM(varname, condition, data, out, v1_algo)  update_checksum(condition, data, out, HashAlgorithm.v1_algo)

#define CTL_MAIN CTL_INGRESS
