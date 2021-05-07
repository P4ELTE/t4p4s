// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dpdk_lib.h"

#include <stdio.h>

void make_formatter(char* out_fmt, const char* in_fmt, ...) {
    va_list argp;
    va_start(argp, in_fmt);

    char* param_ptr;
    while ((param_ptr = strstr(in_fmt, "{}")) != NULL) {
        const char* param_fmt = va_arg(argp, const char*);

        int pre_param_len = param_ptr - in_fmt;
        int param_len = strlen(param_fmt);

        memcpy(out_fmt, in_fmt, pre_param_len);
        strcpy(out_fmt + pre_param_len, param_fmt);
        out_fmt += pre_param_len + param_len;
        in_fmt += pre_param_len + 2; // skip the {} characters
    }

    // the remaining format characters don't contain {}, copy them
    strcpy(out_fmt, in_fmt);

    va_end(argp);
}

void print_log_msg(const char* msg) {
    #ifdef T4P4S_DEBUG
        debug("    : " T4LIT(Logged,status) ": %s\n", msg);
    #else
        printf("    : " T4LIT(Logged,status) ": %s\n", msg);
    #endif
}

void log_msg(const char* msg, SHORT_STDPARAMS) {
    debug("    : " T4LIT(Logged,status) ": %s\n", msg);
}

const char* dec_hex_fmt(int size) {
    #ifdef T4P4S_DEBUG
        if(size == 1) {
            return (T4LIT(%d) " (0x" T4LIT(%02x,bytes) ")");
        }else if(size == 2) {
            return (T4LIT(%d) " (0x" T4LIT(%04x,bytes) ")");
        } else {
            return (T4LIT(%d) " (0x" T4LIT(%08x,bytes) ")");
        }
    #else
        if(size == 1) {
            return "%d (0x%02x)";
        }else if(size == 2) {
            return "%d (0x%04x)";
        } else {
            return "%d (0x%08x)";
        }
    #endif
}

void log_msg__u8s(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS) {
    char fmt[256];
    char text[256];

    make_formatter(fmt, *msg, dec_hex_fmt(1), dec_hex_fmt(1), dec_hex_fmt(1), dec_hex_fmt(1));

    uint8_t param1 = ((uint8_t*)data.buffer)[0];
    uint8_t param2 = ((uint8_t*)data.buffer)[1];
    uint8_t param3 = ((uint8_t*)data.buffer)[2];
    uint8_t param4 = ((uint8_t*)data.buffer)[3];
    sprintf(text, fmt, param1, param1, param2, param2, param3, param3, param4, param4);

    print_log_msg(text);
}

void log_msg__u16s(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS) {
    char fmt[256];
    char text[256];

    make_formatter(fmt, *msg, dec_hex_fmt(2), dec_hex_fmt(2), dec_hex_fmt(2), dec_hex_fmt(2));

    uint16_t param1 = ((uint16_t*)data.buffer)[0];
    uint16_t param2 = ((uint16_t*)data.buffer)[1];
    uint16_t param3 = ((uint16_t*)data.buffer)[2];
    uint16_t param4 = ((uint16_t*)data.buffer)[3];
    sprintf(text, fmt, param1, param1, param2, param2, param3, param3, param4, param4);

    print_log_msg(text);
}

void log_msg__u32s(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS) {
    char fmt[256];
    char text[256];

    make_formatter(fmt, *msg, dec_hex_fmt(4), dec_hex_fmt(4), dec_hex_fmt(4), dec_hex_fmt(4));

    uint32_t param1 = ((uint32_t*)data.buffer)[0];
    uint32_t param2 = ((uint32_t*)data.buffer)[1];
    uint32_t param3 = ((uint32_t*)data.buffer)[2];
    uint32_t param4 = ((uint32_t*)data.buffer)[3];
    sprintf(text, fmt, param1, param1, param2, param2, param3, param3, param4, param4);

    print_log_msg(text);
}

// TODO properly implement arbitrary length parameter passing and formatting
void log_msg__bufs(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS) {
    char fmt[256];
    char text[256];

    make_formatter(fmt, *msg, "0x" T4LIT(%08x...,bytes));

    uint32_t param = t4p4s2net_4(*(uint32_t*)data.buffer);
    sprintf(text, fmt, param);

    print_log_msg(text);
}

void log_msg__ethernet_ts(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS) {
    char fmt[256];
    char text[256];

    make_formatter(fmt, *msg, "src=0x" T4LIT(%08x%04x,bytes) " dst=0x" T4LIT(%08x%04x,bytes) " typ=0x" T4LIT(%04x,bytes));

    // TODO make this less unappealing
    uint32_t src1 = t4p4s2net_4(*(uint32_t*)(data.buffer+0));
    uint16_t src2 = t4p4s2net_2(*(uint16_t*)(data.buffer+4));
    uint32_t dst1 = t4p4s2net_4(*(uint32_t*)(data.buffer+6));
    uint16_t dst2 = t4p4s2net_2(*(uint16_t*)(data.buffer+6+4));
    uint16_t typ = t4p4s2net_2(*(uint16_t*)(data.buffer + 12));
    sprintf(text, fmt, src1, src2, dst1, dst2, typ);

    print_log_msg(text);
}
