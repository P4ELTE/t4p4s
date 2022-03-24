/* -*- P4_16 -*- */
#include "common-boilerplate-pre.p4"

/* CONSTANTS */

const bit<16> TYPE_IPV4 = 0x0800;
const bit<8>  TYPE_TCP  = 6;

#define BLOOM_FILTER_ENTRIES 65535
#4096
#define BLOOM_FILTER_BIT_WIDTH 32


struct metadata {
    /* empty */
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    tcp_t        tcp;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            TYPE_TCP: tcp;
            default: accept;
        }
    }

    state tcp {
       packet.extract(hdr.tcp);
       transition accept;
    }
}

CTL_MAIN {
    DECLARE_REGISTER(bit<BLOOM_FILTER_BIT_WIDTH>, 1, bloom_filter_1)
    DECLARE_REGISTER(bit<BLOOM_FILTER_BIT_WIDTH>, 1, bloom_filter_2)
    bit<32> reg_pos_one; bit<32> reg_pos_two;
    bit<32> reg_val_one; bit<32> reg_val_two;
    bit<32> direction;

    action drop() {
        MARK_TO_DROP();
        exit;
    }

    action compute_hashes(ip4Addr_t ipAddr1, ip4Addr_t ipAddr2, bit<16> port1, bit<16> port2){
        // Note: this counters "reg_pos_one may be uninitialized"
        reg_pos_one = 0;
        reg_pos_two = 0;

        //Get register position
        HASH(reg_pos_one, HashAlgorithm.crc32, (bit<32>)0, ({ipAddr1,
                                                            ipAddr2,
                                                            port1,
                                                            port2,
                                                            hdr.ipv4.protocol}),
                                                            (bit<32>)BLOOM_FILTER_ENTRIES);
 
        HASH(reg_pos_two, HashAlgorithm.crc32, (bit<32>)0, ({ipAddr1,
                                                            ipAddr2,
                                                            port1,
                                                            port2,
                                                            hdr.ipv4.protocol}),
                                                            (bit<32>)BLOOM_FILTER_ENTRIES);
        if(reg_pos_one > 65535){
            reg_pos_one = reg_pos_one >> 16;
        }
        if(reg_pos_two > 65535){
            reg_pos_two = reg_pos_two >> 16;
        }
    }

/*    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        SET_EGRESS_PORT(port);
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }
*/
    
    action set_direction(bit<1> dir) {
        direction = (bit<32>)dir;
    }

    table check_ports {
        key = {
            GET_INGRESS_PORT(): exact;
            GET_EGRESS_PORT_SIMPLE(): exact;
        }
        actions = {
            set_direction;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }
    
    apply {
        // Note: extra trickery is needed because "Cannot declare variables with type int"
        bit<PortId_size> in_port = (bit<PortId_size>)GET_INGRESS_PORT();
        bit<PortId_size> new_egress_port = (bit<PortId_size>)(((int)in_port+1)%2);
        SET_EGRESS_PORT(new_egress_port);

        if (hdr.ipv4.isValid()){
            if (hdr.tcp.isValid()){
                //compute_hashes(hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.tcp.srcPort, hdr.tcp.dstPort);
                direction = 0; // default
                if (check_ports.apply().hit) {
                    // test and set the bloom filter
                    if (direction == 0) {
                        compute_hashes(hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.tcp.srcPort, hdr.tcp.dstPort);
                        // Packet comes from internal network
                        // If there is a syn we update the bloom filter and add the entry
                        if (hdr.tcp.flags.syn == 1){
                            @atomic{
                             REGISTER_WRITE(bloom_filter_1, 1, reg_pos_one);
                             REGISTER_WRITE(bloom_filter_2, 1, reg_pos_two);
                            }
                        }
                    } else{ // Packet comes from external network
                        compute_hashes(hdr.ipv4.dstAddr, hdr.ipv4.srcAddr, hdr.tcp.dstPort, hdr.tcp.srcPort);
                        @atomic{
                         REGISTER_READ(reg_val_one, bloom_filter_1, (int)reg_pos_one);
                         REGISTER_READ(reg_val_two, bloom_filter_2, (int)reg_pos_two);
                        }
                        // only allow flow to pass if both entries are set
                        if (reg_val_one != 1 || reg_val_two != 1){
                            drop();
                        }
                    }
                } else { // Packet comes from external network, but port is not added to table
                        compute_hashes(hdr.ipv4.dstAddr, hdr.ipv4.srcAddr, hdr.tcp.dstPort, hdr.tcp.srcPort);
                        @atomic{
                         REGISTER_READ(reg_val_one, bloom_filter_1, (int)reg_pos_one);
                         REGISTER_READ(reg_val_two, bloom_filter_2, (int)reg_pos_two);
                        }
                        // only allow flow to pass if both entries are set
                        if (reg_val_one != 1 || reg_val_two != 1){
                            drop();
                        }
                }
            }
        }
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

#include "common-boilerplate-post.p4"
