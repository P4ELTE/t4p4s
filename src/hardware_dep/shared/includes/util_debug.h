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
    #define T4LIT(txt,...)      T4COLOR(T4LIGHT_##__VA_ARGS__) #txt T4COLOR(T4LIGHT_off)
#endif

#define T4LIGHT_ T4LIGHT_default



#include <pthread.h>

void dbg_lock();
void dbg_unlock();

#ifdef T4P4S_DEBUG
    int dbg_sprint_bytes_limit(char* out, void* bytes, int byte_count, int upper_limit, const char* sep);
    int dbg_fprint_bytes_limit(FILE* out_file, void* bytes, int byte_count, int upper_limit, const char* sep);
    int dbg_sprint_bytes(char* out, void* bytes, int byte_count);
    int dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count);

    extern pthread_mutex_t dbg_mutex;


    #define __SHORTFILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #define dbg_bytes(bytes, byte_count, MSG, ...)   \
        { \
            dbg_lock(); \
            debug_printf(MSG T4COLOR(T4LIGHT_bytes), ##__VA_ARGS__); \
            dbg_fprint_bytes(stderr, bytes, byte_count); \
            fprintf(stderr, T4COLOR(T4LIGHT_off) "\n"); \
            dbg_unlock(); \
        }

    #define dbg_mbuf(buf, MSG, ...) dbg_bytes(rte_pktmbuf_mtod(buf, uint8_t*), rte_pktmbuf_pkt_len(buf), MSG " (" T4LIT(%uB) "): ", ##__VA_ARGS__, rte_pktmbuf_pkt_len(buf))

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
        #define short_lcore_debug(lcid,sc,lcc,sfn,sid,M,...)   fprintf(stderr, "%11.11s@%4d [CORE" T4LIT(%*d,core) "] " M "", sfn, __LINE__, lcc ? 2 : 1, lcid, ##__VA_ARGS__)
        #define lcore_debug(lcid,sc,lcc,sfn,sid,M,...)         fprintf(stderr, "%11.11s@%4d [CORE" T4LIT(%*d,core) "@" T4LIT(%d,socket) "] " M "", sfn, __LINE__, lcc ? 2 : 1, lcid, sid, ##__VA_ARGS__)
        #define no_core_debug(lcid,sc,lcc,sfn,sid,M,...)       fprintf(stderr, "%11.11s@%4d [     %s%s] " M "", sfn, __LINE__, lcc ? " " : "", sc == 1 ? "" : "  ", ##__VA_ARGS__)
    #else
        // no filename/line number printout
        #define short_lcore_debug(lcid,sc,lcc,sfn,sid,M,...)   fprintf(stderr, T4LIT(%*d,core) " " M "", lcc ? 2 : 1, lcid, ##__VA_ARGS__)
        #define lcore_debug(lcid,sc,lcc,sfn,sid,M,...)         fprintf(stderr, T4LIT(%*d,core) "@" T4LIT(%d,socket) " " M "", lcc ? 2 : 1, lcid, sid, ##__VA_ARGS__)
        #define no_core_debug(lcid,sc,lcc,sfn,sid,M,...)       fprintf(stderr, "-%s%s " M "", lcc ? "-" : "", sc == 1 ? "" : "--", ##__VA_ARGS__)
    #endif


    #define debug_printf2(lcid, sc, lcc, sfn, M, ...)   ((lcid == UINT_MAX) ? no_core_debug(lcid, sc, lcc, sfn, M, ##__VA_ARGS__) : sc == 1 ? short_lcore_debug(lcid, sc, lcc, sfn, M, ##__VA_ARGS__) : lcore_debug(lcid, sc, lcc, sfn, M, ##__VA_ARGS__));

    #define debug_printf(M, ...)   debug_printf2(rte_lcore_id(), rte_socket_count(), rte_lcore_count() > 9, SHORTEN(__SHORTFILENAME__, 13), rte_lcore_to_socket_id(rte_lcore_id()), M, ##__VA_ARGS__)


    #define debug(M, ...) \
        { \
            dbg_lock(); \
            debug_printf(M, ##__VA_ARGS__); \
            dbg_unlock(); \
        }

#else
    #define dbg_bytes(bytes, byte_count, MSG, ...)
    #define dbg_print(bytes, bit_count, MSG, ...)
    #define dbg_mbuf(buf, MSG, ...)
    #define debug(M, ...)
    #define short_lcore_debug(lcid,sc,lcc,sfn,sid,M,...)
    #define lcore_debug(lcid,sc,lcc,sfn,sid,M,...)
    #define no_core_debug(lcid,sc,lcc,sfn,sid,M,...)
    #define debug_printf2(lcid, sc, lcc, sfn, M, ...)
    #define debug_printf(M, ...)
    #define debug(M, ...)
#endif


#define lcore_report(M, ...)   fprintf(stderr, T4LIT(%2d,core) "@" T4LIT(%d,socket) " " M "", (int)(rte_lcore_id()), rte_lcore_to_socket_id(rte_lcore_id()), ##__VA_ARGS__)

#define report(M, ...) \
    { \
        dbg_lock(); \
        lcore_report(M, ##__VA_ARGS__); \
        dbg_unlock(); \
    }

typedef struct occurence_counter_s {
    int counter;
    uint64_t start_cycle;
} occurence_counter_t;

#if T4P4S_STATS
    #define COUNTER_INIT(oc) {(oc).counter=-1;}
    #define COUNTER_ECHO(oc,print_template){ \
            if((oc).counter == -1) { \
                (oc).start_cycle = rte_get_tsc_cycles(); \
                (oc).counter++; \
            } \
            if(rte_get_tsc_cycles() - (oc).start_cycle > rte_get_timer_hz()){ \
                report(print_template,(oc).counter); \
                (oc).start_cycle = rte_get_tsc_cycles(); \
                (oc).counter = 0; \
            } \
        }
    #define COUNTER_STEP(oc){ \
               (oc).counter++;  \
            }
#else
    #define COUNTER_INIT(oc)
    #define COUNTER_ECHO(oc,print_template)
    #define COUNTER_STEP(oc)
#endif




typedef struct time_measure_s{
    uint64_t start_cycle;
    uint64_t echo_start_cycle;
    int counter;
    uint64_t time_sum;
}time_measure_t;

#define TIME_MEASURE_INIT(tm) {tm.echo_start_cycle = 0;}
#define TIME_MEASURE_ECHO(tm,print_template) { \
            if(tm.echo_start_cycle == 0) { \
                tm.echo_start_cycle = rte_get_tsc_cycles(); \
            } \
            if(rte_get_tsc_cycles() - tm.echo_start_cycle > rte_get_timer_hz() && tm.counter > 0){ \
                report(print_template,tm.time_sum); \
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


#define ONE_PER_SEC(timer) if(rte_get_tsc_cycles() - (timer) > rte_get_timer_hz()?((timer) = rte_get_tsc_cycles()),true:false)


void sleep_millis(int millis);
