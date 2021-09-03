// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#pragma once

#import "util_packet_bitfield.h"

int padded_bytecount(bitfield_handle_t fd);
int padded_bitcount(bitfield_handle_t fd);


uint8_t topbits_1(uint8_t data, int bits);
uint16_t topbits_2(uint16_t data, int bits);
uint32_t topbits_4(uint32_t data, int bits);

uint8_t net2t4p4s_1(uint8_t data);
uint16_t net2t4p4s_2(uint16_t data);
uint32_t net2t4p4s_4(uint32_t data);

uint8_t t4p4s2net_1(uint8_t data);
uint16_t t4p4s2net_2(uint16_t data);
uint32_t t4p4s2net_4(uint32_t data);

uint32_t net2t4p4s(bitfield_handle_t fd, uint32_t data);
uint32_t t4p4s2net(bitfield_handle_t fd, uint32_t data);
