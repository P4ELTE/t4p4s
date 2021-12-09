
#pragma once

typedef bit<9> PortId_t;

#include <core.p4>
#include <v1model.p4>
#include "common_headers.p4"


#define PARSER                  parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_VERIFY_CHECKSUM     control verifyChecksum(inout headers hdr, inout metadata meta)
#define CTL_INGRESS             control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_EGRESS              control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_UPDATE_CHECKSUM     control computeChecksum(inout headers hdr, inout metadata meta)
#define CTL_EMIT                control DeparserImpl(packet_out packet, in headers hdr)

#define MARK_TO_DROP()            mark_to_drop(standard_metadata)

#define PortId_size       9
#define PortId_const(txt) 9w ## txt

#define GET_INGRESS_PORT()        standard_metadata.ingress_port
#define GET_EGRESS_PORT()         standard_metadata.egress_port
#define SET_EGRESS_PORT(value)    standard_metadata.egress_port = (PortId_t)value

#define DECLARE_DIGEST(type, name)              /* nothing to declare for v1model */
#define CALL_DIGEST(type, name, port, content)  digest<type>((bit<32>)port, content);

#define CALL_UPDATE_CHECKSUM(condition, data, checksum, algo)  update_checksum(condition, data, checksum, algo);

#define CTL_MAIN CTL_INGRESS
