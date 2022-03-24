
#pragma once

#ifndef CUSTOM_CTL_EGRESS
    CTL_EGRESS { apply {} }
#endif

#ifdef CUSTOM_CTL_CHECKSUM
    CTL_VERIFY_CHECKSUM { apply { CustomVerifyChecksumCtl.apply(hdr, meta); } }
    CTL_UPDATE_CHECKSUM { apply { CustomUpdateChecksumCtl.apply(hdr, meta); } }
#else
    CTL_VERIFY_CHECKSUM { apply {} }
    CTL_UPDATE_CHECKSUM { apply {} }
#endif

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
