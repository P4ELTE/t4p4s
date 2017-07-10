#ifndef _PAXOS_PARSER_P4_
#define _PAXOS_PARSER_P4_

// Parser for paxos packet headers

parser parse_paxos {
    extract(paxos);
    return ingress;
}

#endif