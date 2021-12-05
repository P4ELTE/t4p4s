// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include <unistd.h>
#include "rte_mbuf.h"
#include "util_debug.h"

#ifdef T4P4S_DEBUG
    #include "backend.h"
    #include <pthread.h>

    pthread_mutex_t dbg_mutex;

    void dbg_lock() {
#ifndef T4P4S_NO_DBG_LOCK
        pthread_mutex_lock(&dbg_mutex);
#endif
    }

    void dbg_unlock() {
#ifndef T4P4S_NO_DBG_LOCK
        pthread_mutex_unlock(&dbg_mutex);
#endif
    }

    int reasonable_upper_limit() {
        #ifndef T4P4S_DEBUG_PKT_MAXBYTES
            return 80;
        #else
            return T4P4S_DEBUG_PKT_MAXBYTES;
        #endif
    }

    int dbg_sprint_bytes_limit(char* out, void* bytes, int byte_count, int upper_limit, const char* sep) {
        if (byte_count <= 0)    return 0;

        int char_count = 0;

        if (byte_count > upper_limit) {
            char_count += sprintf(out, "(showing %dB) ", upper_limit);
        }

        int limit = (byte_count <= upper_limit ? byte_count : upper_limit);
        for (int i = 0; i < limit; ++i) {
            char_count += sprintf(out+char_count, "%02x%s", ((uint8_t*)bytes)[i], i%2 == 0 || i == limit-1 ? "" : sep);
        }

        if (byte_count > upper_limit) {
            char_count += sprintf(out+char_count, "...");
        }

        return char_count;
    }

    int dbg_fprint_bytes_limit(FILE* out_file, void* bytes, int byte_count, int upper_limit, const char* sep) {
        if (byte_count <= 0)    return 0;

        int char_count = 0;

        if (byte_count > upper_limit) {
            char_count += fprintf(out_file, "(showing %dB) ", upper_limit);
        }

        int limit = (byte_count <= upper_limit ? byte_count : upper_limit);
        for (int i = 0; i < limit; ++i) {
            char_count += fprintf(out_file, "%02x%s", ((uint8_t*)bytes)[i], i%2 == 0 || i == limit-1 ? "" : sep);
        }

        if (byte_count > upper_limit) {
            char_count += fprintf(out_file, "...");
        }

        return char_count;
    }

    int dbg_sprint_bytes(char* out, void* bytes, int byte_count) {
        return dbg_sprint_bytes_limit(out, bytes, byte_count, reasonable_upper_limit(), " ");
    }

    int dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count) {
        return dbg_fprint_bytes_limit(out_file, bytes, byte_count, reasonable_upper_limit(), " ");
    }
#endif

void sleep_millis(int millis) {
    usleep(millis * 1000);
}
