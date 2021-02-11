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


    #include <pthread.h>
    pthread_mutex_t dbg_mutex;

    #define debug_printf(M, ...)   ((rte_lcore_id() == UINT_MAX) ? no_core_debug(M, ##__VA_ARGS__) : lcore_debug(M, ##__VA_ARGS__)); \

    #define debug(M, ...) \
        { \
            pthread_mutex_lock(&dbg_mutex); \
            debug_printf(M, ##__VA_ARGS__); \
            pthread_mutex_unlock(&dbg_mutex); \
        }

    #define debug_mbuf(mbuf,message) \
    { \
        dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), \
        "\n--------------------------------\n" T4LIT(%s, port) " (" T4LIT(%d) " bytes): ", message, rte_pktmbuf_pkt_len(mbuf)); \
    }

    #define fdebug(M, ...) \
    { \
        pthread_mutex_lock(&dbg_mutex); \
        debug_printf(M, ##__VA_ARGS__); \
        pthread_mutex_unlock(&dbg_mutex); \
    }

#else
    #define dbg_bytes(bytes, byte_count, MSG, ...)
    #define dbg_print(bytes, bit_count, MSG, ...)
    #define debug(M, ...)
    #define debug_mbuf(mbuf,message)
    #define fdebug(M, ...)
#endif



typedef struct occurence_counter_s {
    int counter;
    uint64_t start_cycle;
} occurence_counter_t;


typedef struct time_measure_s{
    uint64_t start_cycle;
    uint64_t echo_start_cycle;
    int counter;
    uint64_t time_sum;
}time_measure_t;

#if 1==1
    #define COUNTER_INIT(oc) {oc.counter=0;}
    #define COUNTER_ECHO(oc,print_template){ \
            if(oc.counter == 0) { \
                oc.start_cycle = rte_get_tsc_cycles(); \
                oc.counter++; \
            } \
            if(rte_get_tsc_cycles() - oc.start_cycle > rte_get_timer_hz()){ \
                fdebug(print_template,oc.counter); \
                oc.start_cycle = rte_get_tsc_cycles(); \
                oc.counter = 0; \
            } \
        }
    #define COUNTER_STEP(oc){ \
               oc.counter++;  \
            }


    #define TIME_MEASURE_INIT(tm) {tm.echo_start_cycle = 0;}
    #define TIME_MEASURE_ECHO(tm,print_template) { \
            if(tm.echo_start_cycle == 0) { \
                tm.echo_start_cycle = rte_get_tsc_cycles(); \
            } \
            if(rte_get_tsc_cycles() - tm.echo_start_cycle > rte_get_timer_hz() && tm.counter > 0){ \
                fdebug(print_template,tm.time_sum); \
                tm.echo_start_cycle = rte_get_tsc_cycles(); \
                tm.time_sum = 0; \
                tm.counter = 0; \
            } \
        }
    #define TIME_MEASURE_START(tm){ \
               tm.start_cycle = rte_get_tsc_cycles();  \
            }
    #define TIME_MEASURE_STOP(tm){ \
               tm.time_sum += rte_get_tsc_cycles() - tm.start_cycle;  \
               tm.counter++; \
            }
#else
    #define COUNTER_INIT(oc)
    #define COUNTER_ECHO(oc,print_template)
    #define COUNTER_STEP(oc)

    #define TIME_MEASURE_INIT(tm)
    #define TIME_MEASURE_ECHO(tm,print_template)
    #define TIME_MEASURE_START(tm)
    #define TIME_MEASURE_STOP(tm)
#endif

#define ONE_PER_SEC(timer) if(rte_get_tsc_cycles() - timer > rte_get_timer_hz()?(timer = rte_get_tsc_cycles()),true:false)


void sleep_millis(int millis);
