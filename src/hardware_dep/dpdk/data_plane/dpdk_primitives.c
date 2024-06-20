// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#ifdef T4P4S_UNITTEST
    #include "t4p4s_unittest_lib.h"
    #include "util_packet_bitfield.h"
    #include "dpdk_primitives.h"
    #include "dataplane_hdr_fld_pkt.h"
#else
    #include "dpdk_primitives_impl.h"
    #ifdef T4P4S_DEBUG
        #include "util_debug.h"
    #endif
#endif
