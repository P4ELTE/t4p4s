
#pragma once

#ifndef CUSTOM_CTL_EGRESS
    control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) { apply {} }
#endif

#ifndef CUSTOM_CTL_CHECKSUM
    CTL_VERIFY_CHECKSUM { apply {} }

    CTL_UPDATE_CHECKSUM { apply {} }
#endif

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
