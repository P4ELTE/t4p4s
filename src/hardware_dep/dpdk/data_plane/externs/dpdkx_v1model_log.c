// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

#include <stdio.h>

void print_uconst(char** out_fmt_ptr, uint32_t value, int byte_size) {
    *out_fmt_ptr += sprintf(*out_fmt_ptr, T4LIT(%d), value);
    if (value > 9) {
        *out_fmt_ptr += sprintf(*out_fmt_ptr, " = " T4LIT(%x,bytes), value);
    }
    *out_fmt_ptr += byte_size;
}

void print_iconst(char** out_fmt_ptr, int32_t value, int byte_size) {
    *out_fmt_ptr += sprintf(*out_fmt_ptr, T4LIT(%d), value);
    if (value > 9) {
        *out_fmt_ptr += sprintf(*out_fmt_ptr, " = " T4LIT(%x,bytes), value);
    }
    *out_fmt_ptr += byte_size;
}

extern char* sprintf_hdr_general(char* out, int field_count, uint32_t vals[], uint8_t* ptrs[], int offsets[], int sizes[], int vw_sizes[], const char*const fld_short_names[]);

void make_formatter_for_tuple(char** out_fmt, uint8_buffer_t* data) {
    *out_fmt += sprintf(*out_fmt, "(" T4LIT(%s,header) ": ", data->name);

    // TODO implement vw field handling properly
    int fake_vw_sizes[] = { NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT, NO_VW_FIELD_PRESENT };

    uint32_t vals2[data->part_count];
    uint8_t* ptrs2[data->part_count];
    for (int i = 0; i < data->part_count; ++i) {
        uint8_t* ptr = data->buffer + (data->part_bit_offsets[i] / 8);

        if (data->part_bit_sizes[i] > 32) {
            ptrs2[i] = data->buffer;
        } else if (data->part_bit_sizes[i] > 16) {
            vals2[i] = *(uint32_t*)ptr;
        } else if (data->part_bit_sizes[i] > 8) {
            vals2[i] = *(uint16_t*)ptr;
        } else {
            vals2[i] = *ptr;
        }
    }

    *out_fmt = sprintf_hdr_general(*out_fmt, data->part_count, vals2, ptrs2, data->part_bit_offsets, data->part_bit_sizes, fake_vw_sizes, data->part_names);
    *out_fmt += sprintf(*out_fmt, ")");
}

void make_formatter(char* out_fmt, const char* in_fmt, uint8_buffer_t* data) {
    int idx = 0;
    char* param_ptr;
    while ((param_ptr = strstr(in_fmt, "{}")) != NULL) {
        int pre_param_len = param_ptr - in_fmt;

        memcpy(out_fmt, in_fmt, pre_param_len);
        out_fmt += pre_param_len;

        bool is_last_tuple = strstr(in_fmt + pre_param_len + 2, "{}") == NULL;

        if (is_last_tuple && data->part_count - idx > 1) {
            make_formatter_for_tuple(&out_fmt, data);
        } else {
            const char*const param_type = data->part_types[idx];
            void* buf = data->buffer + data->part_bit_offsets[idx];
            int byte_size = data->part_bit_sizes[idx] / 8;
            if (!strcmp(param_type, "u8")) {
                print_uconst(&out_fmt, *(uint8_t*)buf, byte_size);
            } else if (!strcmp(param_type, "i8")) {
                print_iconst(&out_fmt, *(uint8_t*)buf, byte_size);
            } else if (!strcmp(param_type, "u16")) {
                print_uconst(&out_fmt, *(uint16_t*)buf, byte_size);
            } else if (!strcmp(param_type, "i16")) {
                print_iconst(&out_fmt, *(uint16_t*)buf, byte_size);
            } else if (!strcmp(param_type, "u32")) {
                print_uconst(&out_fmt, *(uint32_t*)buf, byte_size);
            } else if (!strcmp(param_type, "i32")) {
                print_iconst(&out_fmt, *(uint32_t*)buf, byte_size);
            } else if (!strcmp(param_type, "buf")) {
                out_fmt += sprintf(out_fmt, T4LIT(%08x,bytes) "%s", *(uint32_t*)buf, byte_size > 4 ? "..." : "");
            } else {
                make_formatter_for_tuple(&out_fmt, data);
            }
        }

        in_fmt += pre_param_len + 2; // skip the {} characters

        ++idx;
    }

    // the remaining format characters don't contain {}, copy them
    strcpy(out_fmt, in_fmt);
}

void EXTERNIMPL0(log_msg)(const char* msg, SHORT_STDPARAMS) {
    #ifdef T4P4S_DEBUG
        debug("    : " T4LIT(Logged,status) ": %s\n", msg);
    #else
        printf("    : " T4LIT(Logged,status) ": %s\n", msg);
    #endif
}

void EXTERNIMPL1(log_msg,u8s)(const char* msg, uint8_buffer_t data, SHORT_STDPARAMS) {
    char fmt[256];
    char text[256];

    make_formatter(fmt, msg, &data);

    uint32_t param = t4p4s2net_4(*(uint32_t*)data.buffer);
    sprintf(text, fmt, param);

    EXTERNIMPL0(log_msg)(text, SHORT_STDPARAMS_IN);
}
