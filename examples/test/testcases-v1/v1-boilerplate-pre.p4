
#define PARSER      parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_INGRESS control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata)
#define CTL_EMIT    control DeparserImpl(packet_out packet, in headers hdr)


#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"
