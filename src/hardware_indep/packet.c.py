
#{ INLINING bool is_packet_dropped(packet_descriptor_t* pd) {
#[      return get_egress_port(pd) == EGRESS_DROP_VALUE;
#} }
