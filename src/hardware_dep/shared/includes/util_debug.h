// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#pragma once


#include <stdint.h>
#include <stdio.h>
#include <rte_common.h>
#include <pthread.h>
#include "gen_light.h"

#define T4COLOR(color)    "\e[" color "m"

#ifndef T4LIGHT_off
    #define T4LIGHT_off
#endif

#ifndef T4LIGHT_default
    #define T4LIT(txt,...)      #txt
    #define T4LIGHT_default
#else
    #define T4LIT(txt,...)      "\e[" T4LIGHT_##__VA_ARGS__ "m" #txt "\e[" T4LIGHT_off "m"
#endif

#define T4LIGHT_ T4LIGHT_default


#ifdef T4P4S_DEBUG
    extern void dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count);
    extern pthread_mutex_t dbg_mutex;

    #define __SHORTFILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #define dbg_bytes(bytes, byte_count, MSG, ...)   \
        { \
            pthread_mutex_lock(&dbg_mutex); \
            debug_printf(MSG T4COLOR(T4LIGHT_bytes), ##__VA_ARGS__); \
            dbg_fprint_bytes(stderr, bytes, byte_count); \
            fprintf(stderr, T4COLOR(T4LIGHT_off) "\n"); \
            pthread_mutex_unlock(&dbg_mutex); \
        }

    #define dbg_print(bytes, bit_count, MSG, ...) \
        { \
            char fmt[256]; \
            if (bit_count <= 32) { \
                sprintf(fmt, "%s/" T4LIT(%db) " = " T4LIT(%d) " = " T4LIT(0x%x) " = ", MSG, bit_count, *(bytes), *(bytes)); \
            } else { \
                sprintf(fmt, "%s/" T4LIT(%db) " = ", MSG, bit_count); \
            } \
            dbg_bytes(bytes, (bit_count+7)/8, "%s", fmt, ##__VA_ARGS__);   \
        }

    #define __SHORTFILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #define SHORTEN(str, len) ((strlen(str) <= (len)) ? (str) : ((str) + (strlen(str) - len)))

    #ifdef T4P4S_DEBUG_LINENO
        #define lcore_debug(M, ...)   fprintf(stderr, "%11.11s@%4d [CORE" T4LIT(%2d,core) "@" T4LIT(%d,socket) "] " M "", SHORTEN(__SHORTFILENAME__, 13), __LINE__, (int)(rte_lcore_id()), rte_lcore_to_socket_id(rte_lcore_id()), ##__VA_ARGS__)
        #define no_core_debug(M, ...) fprintf(stderr, "%11.11s@%4d [NO-CORE ] " M "", SHORTEN(__SHORTFILENAME__, 13), __LINE__, ##__VA_ARGS__)
    #else
        // no filename/line number printout
        #define lcore_debug(M, ...)   fprintf(stderr, T4LIT(%2d,core) "@" T4LIT(%d,socket) " " M "", (int)(rte_lcore_id()), rte_lcore_to_socket_id(rte_lcore_id()), ##__VA_ARGS__)
        #define no_core_debug(M, ...) fprintf(stderr, "---- " M "", ##__VA_ARGS__)
    #endif


    #define debug_printf(M, ...)   ((rte_lcore_id() == UINT_MAX) ? no_core_debug(M, ##__VA_ARGS__) : lcore_debug(M, ##__VA_ARGS__)); \

    #define debug(M, ...) \
        { \
            pthread_mutex_lock(&dbg_mutex); \
            debug_printf(M, ##__VA_ARGS__); \
            pthread_mutex_unlock(&dbg_mutex); \
        }

#else
    #define dbg_bytes(bytes, byte_count, MSG, ...)
    #define dbg_print(bytes, bit_count, MSG, ...)
    #define debug(M, ...)
#endif



void sleep_millis(int millis);
