
#if defined(USE_DEFAULT_CTL_INGRESS)
    control ingress(inout headers hdr,
                    inout metadata meta,
                    in    psa_ingress_input_metadata_t  istd,
                    inout psa_ingress_output_metadata_t ostd)
    {
        apply {
            PortId_t EGRESS_DROP_VALUE = (PortId_t)200;
            if (ostd.egress_port != EGRESS_DROP_VALUE) {
                ostd.egress_port = (PortId_t)12345;
            }
        }
    }
#endif


#if defined(USE_DEFAULT_CTL_EGRESS)
    control egress(inout headers hdr, \
                                  inout metadata meta, \
                                  in    psa_egress_input_metadata_t  istd, \
                                  inout psa_egress_output_metadata_t ostd)
    {
        apply {}
    }
#endif

parser EgressParserImpl(packet_in buffer,
                        out headers hdr,
                        inout metadata meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_metadata_t clone_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers hdr,
                           in metadata meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
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
