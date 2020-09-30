// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include "util_packet.h"

extern lookup_table_t table_config[];

void init_dataplane(SHORT_STDPARAMS);
void handle_packet(uint32_t portid, STDPARAMS);
