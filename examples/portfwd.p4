header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 14;
    }
}

header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

action forward(port) {
    modify_field(standard_metadata.egress_port, port);
}

action forward_rewrite(port, mac) {
    modify_field(standard_metadata.egress_port, port);
    modify_field(ethernet.srcAddr, mac);
}

table t_fwd {
    reads {
        standard_metadata.ingress_port : exact;
    }
    actions {forward; forward_rewrite;}
    size : 2048;
}

control ingress {
    apply(t_fwd);
}

control egress {
}
