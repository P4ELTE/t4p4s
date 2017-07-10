header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type applepie_t {
    fields {
        baking_time : 32;
        weight : 32;
        ingredients_num : 7;
        yummy_factor : 3;
    }
}

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_APPLEPIE 0x0123

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_APPLEPIE : parse_applepie;
        default: ingress;
    }
}

header applepie_t applepie;

parser parse_applepie {
    extract(applepie);
    return ingress;
}

header_type cooking_metadata_t {
    fields {
        baking_time : 32;
    }
}

metadata cooking_metadata_t cooking_metadata;

action set_baking_time(baking_time) {
    modify_field(cooking_metadata.baking_time, baking_time);
    add_to_field(applepie.yummy_factor, +1);
}

action _drop() {
    drop();
}

table process_applepie {
    reads {
        applepie.weight : exact;
    }
    actions {
        set_baking_time;
        _drop;
    }
    size: 1024;
}

action forward(dmac_val,smac_val,port) {
    modify_field(ethernet.dstAddr, dmac_val);
    modify_field(standard_metadata.egress_spec, port);
    modify_field(ethernet.srcAddr, smac_val);
}

table nextpie {
    reads {
        cooking_metadata.baking_time : exact;
    }
    actions {
        forward;
        _drop;
    }
    size: 512;
}

control ingress {
       apply(process_applepie);
       apply(nextpie);
}

control egress {
} 