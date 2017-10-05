
#include "includes/p4_l2_features.h"
#include "includes/p4_l2_headers.p4"
#include "includes/p4_l2_parser.p4"

#define MAC_LEARN_RECEIVER  1024 // Must be 1024 for L2 learning


header_type intrinsic_metadata_t {
  fields {
    lf_field_list : 32;
  }
}

metadata intrinsic_metadata_t intrinsic_metadata;



/* dmac table and actions */

#define MAC_TABLE_SIZE  1024
action dmac_hit(port) {
    modify_field(standard_metadata.egress_spec, port);
}

action dmac_miss() {
    modify_field(standard_metadata.egress_spec, CPU_PORT);
}

table dmac {
    reads {
        ethernet.dstAddr        : exact;
    }
    actions {
        dmac_hit;
        dmac_miss;
    }
    // note: what is min_size good for? is allocation dynamic?
    size : MAC_TABLE_SIZE;
    support_timeout: false;
}

field_list mac_learn_digest {
    ethernet.srcAddr;
    standard_metadata.ingress_port;
}

/* smac table and actions */
action smac_hit() {
}

action smac_miss() {
    /* send to CPU for learning */
    // note: has to see tables dmac and smac?
    //       put controller in metadata?
    //       is it possible to specify controller here?
    //       probably just punt it up to the control plane
    generate_digest(MAC_LEARN_RECEIVER, mac_learn_digest);
}

table smac {
    reads {
        ethernet.srcAddr        : exact;
    }
    actions {
      smac_hit;
      smac_miss;
    }
    size : MAC_TABLE_SIZE;
    // note: can set timeout amount?
    support_timeout: false;
}

// ingress processing
control ingress {
    /* Add router-mac check here for doing route-lookups */
    apply(dmac);    /* L2 forwarding */

    /* perform smac lookup,
     * send notification unknown-sa (smac_miss) to CPU
     * packet is forwarded based on dmac lookup result
     */
    apply(smac);
}

control egress {
    /* no processing on egress - forwarding decision is already made */
}
