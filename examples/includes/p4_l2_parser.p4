
header ethernet_t ethernet;

parser start {
    return parse_ethernet;
}

#define ETHERTYPE_IPV4 0x0800

// note: optimising compiler can discard early and not parse all headers?
//       -------- "" ----------- reduce number of copy steps?
// note: if not ingress at end, can that hinder optimisation?
//       what if, say, we want to process gtp packets differently (send to different entry point)?
// note: accessing metadata can complicate this further
// note: can encapsulation/decapsulation appear at the parsing level?
// note: can recursion appear? (without cloning?)
//       avoid infinite recursion?
// note: must implementation directly follow this description, or does this only provide semantics? is optimisation possible?
// note: use case for optimisations?
// note: use the same table with a sidetracked header?
// note: deparse can give it back / can we discard the original after deparse?
// use case for cloning+sending: multicast, broadcast?
parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_IPV4 : parse_ipv4;
        default: ingress;
    }
}

header ipv4_t ipv4;

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

parser_exception p4_pe_default {
    return ingress;
}

parser_exception p4_pe_checksum {
    parser_drop;
}

parser_exception p4_pe_custom {
    parser_drop;
}

