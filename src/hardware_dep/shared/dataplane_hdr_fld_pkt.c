// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "dataplane_hdr_fld_pkt.h"

void activate_hdr(header_instance_t hdr, packet_descriptor_t* pd) {
    pd->headers[hdr].pointer = (pd->header_tmp_storage + hdr_infos[hdr].byte_offset);
    pd->is_emit_reordering = true;
}

void deactivate_hdr(header_instance_t hdr, packet_descriptor_t* pd) {
    pd->headers[hdr].pointer = 0;
    pd->is_emit_reordering = true;
}
