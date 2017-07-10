#define ENABLE_PAXOS 1
#include "includes/headers.p4"
#include "includes/parser.p4"
#include "includes/paxos_headers.p4"
#include "includes/paxos_parser.p4"
#include "igmp.p4"

register instance_register {
    width : INSTANCE_SIZE;
    instance_count : 1;
}

action _drop() {
    drop();
}

action _nop() {

}

action forward(port) {
    modify_field(standard_metadata.egress_spec, port);
}

table forward_tbl {
    reads {
        standard_metadata.ingress_port : exact;
    }
    actions {
        forward;
        _drop;
    }
    size : 48;
}

//  This function read num_inst stored in the register and copy it to
//  the current packet. Then it increased the num_inst by 1, and write
//  it back to the register
action increase_instance() {
    register_read(paxos.inst, instance_register, 0);
    add_to_field(paxos.inst, 1);
    register_write(instance_register, 0, paxos.inst);
    modify_field(udp.dstPort, PAXOS_ACCEPTOR);
    modify_field(udp.checksum, 0);
}

table sequence_tbl {
    reads   { paxos.msgtype : exact; }
    actions { increase_instance; _nop; }
    size : 1;
}


control ingress {
    if (valid(ipv4)) {
        apply(forward_tbl);
        apply(igmp_tbl);
    }
    if (valid(paxos))
        apply(sequence_tbl);    
}