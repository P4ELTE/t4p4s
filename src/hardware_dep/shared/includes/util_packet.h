// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#ifndef __UTIL_PACKET_H_
#define __UTIL_PACKET_H_

#define SHORT_STDPARAMS packet_descriptor_t* pd, lookup_table_t** tables
#define SHORT_STDPARAMS_IN pd, tables
#define STDPARAMS packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate
#define STDPARAMS_IN pd, tables, pstate

#endif
