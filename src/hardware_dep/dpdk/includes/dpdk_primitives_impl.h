// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"


bitfield_handle_t dst(header_instance_e hdesc, field_instance_e fld);

header_instance_e to_hdr(field_instance_e fld);

// Extract operations

uint32_t GET32_META_IMPL(bitfield_handle_t fd);
uint32_t GET32_FLD_IMPL(bitfield_handle_t fd);
void     GET_BUF_IMPL(void* dst, bitfield_handle_t fd);

// Modify operations

void SET32_IMPL(bitfield_handle_t dst_fd, uint32_t value32);

void SET_BUF_IMPL(bitfield_handle_t dst, void* src, int srclen);
void SET32_BUF_IMPL(bitfield_handle_t dst, void* src, int srclen);
void SET32_BITS_IMPL(bitfield_handle_t dst, uint32_t value32);
