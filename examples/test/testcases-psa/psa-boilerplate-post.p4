
#if defined(USE_DEFAULT_CTL_INGRESS)
    CTL_INGRESS {
        apply {
            PortId_t EGRESS_DROP_VALUE = (PortId_t)200;
            if (ostd.egress_port != EGRESS_DROP_VALUE) {
                ostd.egress_port = (PortId_t)12345;
            }
        }
    }
#endif


#if defined(USE_DEFAULT_CTL_EGRESS)
    CTL_EGRESS {
        apply {}
    }
#endif

PARSER2 {
    state start {
        transition accept;
    }
}

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
