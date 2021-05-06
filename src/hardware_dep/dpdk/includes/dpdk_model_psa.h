// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include "backend.h"
#include "dpdk_smem.h"

#include "util_packet.h"
#include "common.h"

#define T4P4S_MODEL psa

#define INGRESS_META_FLD    FLD(all_metadatas,ingress_port)
#define EGRESS_META_FLD     FLD(all_metadatas,egress_port)
#define EGRESS_INIT_VALUE   0
#define EGRESS_DROP_VALUE   true


typedef enum_PSA_CounterType_t T4P4S_COUNTER_e;
typedef enum_PSA_MeterType_t T4P4S_METER_e;


void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);

// p4.psa: bool psa_normal(in psa_ingress_output_metadata_t istd);
// p4.psa: bool psa_clone_i2e(in psa_ingress_output_metadata_t istd);
// p4.psa: bool psa_clone_e2e(in psa_egress_output_metadata_t istd);
// p4.psa: bool psa_resubmit(in psa_ingress_output_metadata_t istd);
// p4.psa: bool psa_recirculate(in psa_egress_output_metadata_t istd,

// p4.psa: void assert(in bool check);
// p4.psa: void assume(in bool check);

// p4.psa: PacketReplicationEngine {...}
// p4.psa: BufferingQueueingEngine {...}

// p4.psa: ActionProfile {...}
// p4.psa: ActionSelector {...}

// p4.psa: Hash<O> {...}

// p4.psa: Checksum<W> {...}
void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS);
void verify_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS);
void update_checksum__b8s__b16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_PSA_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS);
void update_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_PSA_HashAlgorithm_t algo, SHORT_STDPARAMS);

// p4.psa: Random<T> {...}

// p4.psa: Digest<T> {...}
#ifdef T4P4S_TYPE_mac_learn_digest_t
    void extern_Digest_pack__mac_learn_digest_t(mac_learn_digest_t* mac_learn_digest, Digest_t* out_digest, SHORT_STDPARAMS);
#endif
#ifdef T4P4S_TYPE_learn_digest_t
    void extern_Digest_pack__learn_digest_t(learn_digest_t* learn_digest, Digest_t* out_digest, SHORT_STDPARAMS);
#endif

// p4.psa: Counter<W, S> {...}
// p4.psa: DirectCounter<W> {...}
// p4.psa: Meter<S> {...}
// p4.psa: DirectMeter {...}
// p4.psa: Register<T, S> {...}
    // void extern_counter_count(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS);
    // void extern_meter_execute_meter(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t d, meter_t e, SHORT_STDPARAMS);
    // void extern_register_read(uint32_t index, uint32_t a, uint32_t b, register_t c, SHORT_STDPARAMS);
    // void extern_register_write(uint32_t index, uint32_t a, uint32_t b, register_t* c, SHORT_STDPARAMS);

// p4.psa: InternetChecksum {...}
typedef uint16_t InternetChecksum;
void InternetChecksum_t_init(InternetChecksum* checksum);
void extern_InternetChecksum_clear(InternetChecksum* checksum);
void extern_InternetChecksum_add(InternetChecksum* checksum);
void InternetChecksum_t_get(InternetChecksum* checksum);
void InternetChecksum_t_subtract(InternetChecksum* checksum, uint16_t data);
uint16_t InternetChecksum_t_get_state(InternetChecksum* checksum);
void InternetChecksum_t_set_state(InternetChecksum* checksum, uint16_t checksum_state);
