// SPDX-License-Identifier: Apache-2.0
// Copyright 2016 Eotvos Lorand University, Budapest, Hungary

#include <unistd.h>


#ifdef T4P4S_DEBUG
    #include "backend.h"
    #include <pthread.h>

    pthread_mutex_t dbg_mutex;

    void dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count) {
        #ifndef T4P4S_DEBUG_PKT_MAXBYTES
            int reasonable_upper_limit = 80;
        #else
            int reasonable_upper_limit = T4P4S_DEBUG_PKT_MAXBYTES;
        #endif

        if (byte_count <= 0)    return;

        if (byte_count > reasonable_upper_limit) {
            fprintf(out_file, "(showing %dB) ", reasonable_upper_limit);
        }

        for (int i = 0; i < (byte_count <= reasonable_upper_limit ? byte_count : reasonable_upper_limit); ++i) {
            fprintf(out_file, i%2 == 0 ? "%02x" : "%02x ", ((uint8_t*)bytes)[i]);
        }

        if (byte_count > reasonable_upper_limit) {
            fprintf(out_file, "...");
        }
    }
#endif


void sleep_millis(int millis) {
    usleep(millis * 1000);
}
