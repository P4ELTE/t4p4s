// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "backend.h"

bool is_header_valid(header_instance_e hdr, packet_descriptor_t* pd) {
    return pd->headers[hdr].pointer != NULL;
}
