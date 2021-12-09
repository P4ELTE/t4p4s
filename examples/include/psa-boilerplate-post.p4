
#pragma once

// #ifndef USE_CUSTOM_CTL_INGRESS
//     CTL_MAIN {

//         apply {
//             PortId_t EGRESS_DROP_VALUE = (PortId_t)200;
//             if (standard_metadata.egress_port != EGRESS_DROP_VALUE) {
//                 standard_metadata.egress_port = (PortId_t)12345;
//             }
//         }
//     }
// #endif

PARSER2 {
    state start {
        transition accept;
    }
}

#ifndef USE_CUSTOM_CTL_EGRESS
    CTL_EGRESS {
        apply {
        }
    }
#endif

EMIT2 {
    apply {
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
