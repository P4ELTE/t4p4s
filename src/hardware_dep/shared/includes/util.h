// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <rte_common.h>

void dbg_fprint_bytes(FILE* out_file, void* bytes, int byte_count);


#ifdef T4P4S_BRIGHT
#define T4HIGHLIGHT_BOLD         "1"
#define T4HIGHLIGHT_DEFAULT      "39"
#define T4HIGHLIGHT_BLACK        "30"
#define T4HIGHLIGHT_RED          "31"
#define T4HIGHLIGHT_GREEN        "32"
#define T4HIGHLIGHT_YELLOW       "33"
#define T4HIGHLIGHT_BLUE         "34"
#define T4HIGHLIGHT_MAGENTA      "35"
#define T4HIGHLIGHT_CYAN         "36"
#define T4HIGHLIGHT_LIGHTGRAY    "37"
#define T4HIGHLIGHT_DARKGRAY     "90"
#define T4HIGHLIGHT_LIGHTRED     "91"
#define T4HIGHLIGHT_LIGHTGREEN   "92"
#define T4HIGHLIGHT_LIGHTYELLOW  "93"
#define T4HIGHLIGHT_LIGHTBLUE    "94"
#define T4HIGHLIGHT_LIGHTMAGENTA "95"
#define T4HIGHLIGHT_LIGHTCYAN    "96"
#define T4HIGHLIGHT_WHITE        "97"
#define T4ON   "\e[" T4HIGHLIGHT_LIGHTRED "m"
#define T4OFF   "\e[0m"
#else
#define T4ON    
#define T4OFF   
#endif


#ifdef T4P4S_DEBUG
    #define __SHORTFILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #define dbg_bytes(bytes, byte_count, M, ...)   \
        { \
            debug(M, ##__VA_ARGS__); \
            dbg_fprint_bytes(stderr, bytes, byte_count); \
            fprintf(stderr, "\n"); \
        }
#else
    #define dbg_bytes(bytes, byte_count, M, ...)   
#endif

#endif
