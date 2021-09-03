# SPDX-License-Identifier: Apache-2.0
# Copyright 2021 Eotvos Lorand University, Budapest, Hungary

from utils.codegen import format_type
from compiler_common import unique_everseen

#[ #pragma once

#[ #include "dataplane_hdr_fld_pkt.h"

#[ const char* sprintf_hdr(char* out, packet_descriptor_t* pd, header_descriptor_t* hdr);

for hdr in hlir.header_instances:
    #[ const char* sprintf_hdr_${hdr.name}(char* out, packet_descriptor_t* pd, header_descriptor_t* hdr);



# TODO find a better place for this declaration

#[ int get_fld_vw_size(field_instance_e fld, packet_descriptor_t* pd);
#[
