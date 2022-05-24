// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once

// Function based set field

typedef enum {
    ENDIAN_CONVERT_ALWAYS,
    ENDIAN_CONVERT_AS_NEEDED,
    ENDIAN_KEEP,
    ENDIAN_NET, // the source is an array of bytes, not an int
} endian_strategy_t;

typedef enum {
    SRCDST_BUF,
    SRCDST_PKT,
    SRCDST_HANDLE,
    SRCDST_32,
} srcdst_type_t;

typedef struct {
    srcdst_type_t srcdst_type;

    union {
        packet_descriptor_t* pd;
        struct {
            void* buf;
            int width;
        };
        bitfield_handle_t fd;
        uint32_t value32;
    };
} srcdst_t;

srcdst_t dst_buf(void* buf, int width);
srcdst_t dst_pkt(packet_descriptor_t* pd);
srcdst_t dst_handle(bitfield_handle_t fd);

srcdst_t src_buf(void* buf, int width);
srcdst_t src_pkt(packet_descriptor_t* pd);
srcdst_t src_32(uint32_t value32);
srcdst_t src_handle(bitfield_handle_t fd);

// Extract operations

uint32_t GET32(srcdst_t desc, field_instance_e fld);
uint32_t GET32_def(srcdst_t src, field_instance_e fld, uint32_t default_value);
void     GET_BUF(void* dst, srcdst_t src, field_instance_e fld);

// Modify operations

void MODIFY(srcdst_t desc, field_instance_e fld, srcdst_t src, endian_strategy_t strategy);

void set_fld(packet_descriptor_t* pd, field_instance_e fld, uint32_t value32);
void set_fld_buf(packet_descriptor_t* pd, field_instance_e fld, uint8_t* buf);

// Extract statement

void transfer_to_egress(packet_descriptor_t* pd);

int get_egress_port(packet_descriptor_t* pd);
int get_ingress_port(packet_descriptor_t* pd);
void mark_to_drop(SHORT_STDPARAMS);

// Helpers

bitfield_handle_t get_handle_fld(packet_descriptor_t* pd, field_instance_e fld, const char* operation_txt);
bitfield_handle_t get_handle_buf(void* buf, int width, field_instance_e fld);
bitfield_handle_t get_handle_32(uint32_t value32, field_instance_e fld);
