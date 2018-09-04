#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header hdr1_t {
    bit<8> byte1;
}

header test_dstAddr_t {
    bit<48> dstAddr;
}

header test_srcAddr_t {
    bit<48> srcAddr;
}

header test_etherType_t {
    bit<16> etherType;
}

