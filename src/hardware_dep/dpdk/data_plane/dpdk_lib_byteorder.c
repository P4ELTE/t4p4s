// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include <rte_byteorder.h>

#include "dpdk_lib_byteorder.h"

int padded_bytecount(bitfield_handle_t fd) {
    if (fd.bytecount == 1)	       return 8;
    else if (fd.bytecount == 2)    return 16;
    else                           return 32;
}

int padded_bitcount(bitfield_handle_t fd) { return 8 * padded_bytecount(fd); }

uint8_t  topbits_1(uint8_t  data, int bits) { return data >> (8 - bits);  }
uint16_t topbits_2(uint16_t data, int bits) { return data >> (16 - bits); }
uint32_t topbits_4(uint32_t data, int bits) { return data >> (32 - bits); }

uint8_t  net2t4p4s_1(uint8_t  data) { return data; }
uint16_t net2t4p4s_2(uint16_t data) { return rte_be_to_cpu_16(data); }
uint32_t net2t4p4s_4(uint32_t data) { return rte_be_to_cpu_32(data); }

uint8_t  t4p4s2net_1(uint8_t  data) { return data; }
uint16_t t4p4s2net_2(uint16_t data) { return rte_cpu_to_be_16(data); }
uint32_t t4p4s2net_4(uint32_t data) { return rte_cpu_to_be_32(data); }


uint32_t net2t4p4s(bitfield_handle_t fd, uint32_t data) {
    if (fd.bytecount == 1)    return net2t4p4s_1(data);
    if (fd.bytecount == 2)    return net2t4p4s_2(data);
    return net2t4p4s_4(data);
}

uint32_t t4p4s2net(bitfield_handle_t fd, uint32_t data) {
    if (fd.bytecount == 1)    return t4p4s2net_1(data);
    if (fd.bytecount == 2)    return t4p4s2net_2(data);
    return t4p4s2net_4(data);
}
