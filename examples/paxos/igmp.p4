#include "includes/headers.p4"
#include "includes/parser.p4"

#define MEMBERSHIP_QUERY 0x11
#define MEMBERSHIP_REPORT 0x16

header_type igmp_metadata_t {
    fields {
        high_bits : 48;
        low_bits : 48;

    }
}

metadata igmp_metadata_t igmp_meta;

action handle_query(ip_multicast_group) {
    modify_field(igmp.msgtype, MEMBERSHIP_REPORT);
    modify_field(igmp.max_resp, 0);
    modify_field(igmp.grp_addr, ip_multicast_group);
    // Update destination MAC using IP Group Address
    modify_field(igmp_meta.low_bits, ip_multicast_group & 0x007FFFFF);
    modify_field(igmp_meta.high_bits, 0x01005E000000);
    modify_field(ethernet.dstAddr, igmp_meta.high_bits | igmp_meta.low_bits);
}

table igmp_tbl {
    reads {
        igmp.msgtype : exact;
    }
    actions {
        handle_query;
    }
    size : 8;
}
