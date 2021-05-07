// SPDX-License-Identifier: Apache-2.0
// Copyright 2017 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "backend.h"

#include "util_packet.h"
#include "common.h"

#include "dpdk_smem_repr.h"

#define T4P4S_MODEL v1model

#define INGRESS_META_FLD    FLD(all_metadatas,ingress_port)
#define EGRESS_META_FLD     FLD(all_metadatas,egress_port)
#define EGRESS_INIT_VALUE   0
#define EGRESS_DROP_VALUE   100


void set_handle_packet_metadata(packet_descriptor_t* pd, uint32_t portid);


typedef enum_CounterType_t T4P4S_COUNTER_e;
typedef enum_MeterType_t T4P4S_METER_e;


// v1model.p4: action_profile.action_profile(bit<32> size)
typedef struct {
    uint32_t size;
} action_profile_t;

// v1model.p4: action_selector.action_selector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
typedef struct {
    enum_HashAlgorithm_t algorithm;
    uint32_t             size;
    uint32_t             outputWidth;
} action_selector_t;


void verify(bool check, error_error_t toSignal, SHORT_STDPARAMS);


// v1model.p4: extern void random<T>(out T result, in T lo, in T hi);
void random__u8(uint8_t* out, uint8_t min, uint8_t max, SHORT_STDPARAMS);
void random__u16(uint16_t* out, uint16_t min, uint16_t max, SHORT_STDPARAMS);
void random__u32(uint32_t* out, uint32_t min, uint32_t max, SHORT_STDPARAMS);

void random_fun(uint32_t* out, int min, int max, SHORT_STDPARAMS);

// v1model.p4: extern void digest<T>(in bit<32> receiver, in T data);


// v1model.p4: extern void hash<O, T, D, M>(out O result, in HashAlgorithm algo, in T base, in D data, in M max);
void hash(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS);
void hash__u16__u16__u16s__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS);
void hash__u16__u16__u32s__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS);
void hash__u16__u16__bufs__u32(uint16_t* result, enum_HashAlgorithm_t hash, uint16_t base, uint8_buffer_t data, uint32_t max, SHORT_STDPARAMS);

// v1model.p4: extern void verify_checksum<T, O>(in bool condition, in T data, in O checksum, HashAlgorithm algo);
void verify_checksum(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum__u8s__u16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum__u4s__u16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum__u32s__u16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);

void verify_checksum_offload(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void verify_checksum_offload__u8s__u16(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);

// v1model.p4: extern void update_checksum<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
void update_checksum(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void update_checksum__u8s__u16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);
void update_checksum__u32s__u16(bool cond, uint8_buffer_t data, bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, SHORT_STDPARAMS);

void update_checksum_offload(bitfield_handle_t cksum_field_handle, enum_HashAlgorithm_t algorithm, uint8_t len_l2, uint8_t len_l3, SHORT_STDPARAMS);

// v1model.p4: extern void verify_checksum_with_payload<T, O>(in bool condition, in T data, in O checksum, HashAlgorithm algo);
void verify_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS);
void verify_checksum_with_payload__u32s__u16(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS);

// v1model.p4: extern void update_checksum_with_payload<T, O>(in bool condition, in T data, inout O checksum, HashAlgorithm algo);
void update_checksum_with_payload(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS);
void update_checksum_with_payload__u32s__u16(bool condition, uint8_buffer_t data, bitfield_handle_t checksum, enum_HashAlgorithm_t algo, SHORT_STDPARAMS);

#ifdef T4P4S_TYPE_enum_CloneType
    // v1model.p4: extern void clone(in CloneType type, in bit<32> session);
    void clone(enum_CloneType_t type, uint32_t session, SHORT_STDPARAMS);

    // v1model.p4: extern void clone3<T>(in CloneType type, in bit<32> session, in T data);
    void clone3(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u16(enum_CloneType_t type, uint16_t session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u32(enum_CloneType_t type, uint32_t session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u8(enum_CloneType_t type, uint8_t session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u16s(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u32s(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u8s(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS);
    void clone3__u16ss(enum_CloneType_t type, uint32_t* session, uint8_buffer_t data, SHORT_STDPARAMS);
#endif

// v1model.p4: extern void resubmit<T>(in T data);
// v1model.p4: extern void recirculate<T>(in T data);

// v1model.p4: extern void truncate(in bit<32> length);

// v1model.p4: extern void assert(in bool check);
// v1model.p4: extern void assume(in bool check);

// v1model.p4: extern void log_msg(string msg);
void log_msg(const char* msg, SHORT_STDPARAMS);
// v1model.p4: extern void log_msg<T>(string msg, in T data);
void log_msg__u8s(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS);
void log_msg__u16s(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS);
void log_msg__u32s(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS);
void log_msg__bufs(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS);
void log_msg__ethernet_ts(const char** msg, uint8_buffer_t data, SHORT_STDPARAMS);


// v1model.p4: extern void mark_to_drop(inout standard_metadata_t standard_metadata);
void mark_to_drop(SHORT_STDPARAMS);



// v1model.p4: void count(in I index);
void extern_counter_count(uint32_t counter_array_size, T4P4S_COUNTER_e ct, uint32_t index, counter_t* counter, SHORT_STDPARAMS);

// v1model.p4: void count();
void extern_direct_counter_count(int counter_type, direct_counter_t* smem, SHORT_STDPARAMS);

// void execute_meter<T>(in I index, out T result);
void extern_meter_execute_meter__u32(uint32_t index, T4P4S_METER_e b, uint32_t c, uint8_t* d, meter_t* e, SHORT_STDPARAMS);

// void read(out T result);
void extern_direct_meter_read__u8(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS);
void extern_direct_meter_read__u16(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS);
void extern_direct_meter_read__u32(T4P4S_METER_e b, uint32_t c, meter_t* e, SHORT_STDPARAMS);

// register(bit<32> size);
void init_register__i8(register_int8_t* reg, int8_t size, SHORT_STDPARAMS) ;
void init_register__i16(register_int16_t* reg, int16_t size, SHORT_STDPARAMS) ;
void init_register__i32(register_int32_t* reg, int32_t size, SHORT_STDPARAMS) ;
void init_register__i64(register_int64_t* reg, int64_t size, SHORT_STDPARAMS) ;
void init_register__u8(register_uint8_t* reg, uint8_t size, SHORT_STDPARAMS) ;
void init_register__u16(register_uint16_t* reg, uint16_t size, SHORT_STDPARAMS) ;
void init_register__u32(register_uint32_t* reg, uint32_t size, SHORT_STDPARAMS) ;
void init_register__u64(register_uint64_t* reg, uint64_t size, SHORT_STDPARAMS) ;

// void read(out T result, in I index);
void extern_register_read__i8(int8_t x, int8_t* value_result, int8_t idx, register_int8_t* reg, SHORT_STDPARAMS);
void extern_register_read__i16(int16_t x, int16_t* value_result, int16_t idx, register_int16_t* reg, SHORT_STDPARAMS);
void extern_register_read__i32(int32_t x, int32_t* value_result, int32_t idx, register_int32_t* reg, SHORT_STDPARAMS);
void extern_register_read__i64(int64_t x, int64_t* value_result, int64_t idx, register_int64_t* reg, SHORT_STDPARAMS);
void extern_register_read__u8(uint8_t x, uint8_t* value_result, uint8_t idx, register_uint8_t* reg, SHORT_STDPARAMS);
void extern_register_read__u16(uint16_t x, uint16_t* value_result, uint16_t idx, register_uint16_t* reg, SHORT_STDPARAMS);
void extern_register_read__u32(uint32_t x, uint32_t* value_result, uint32_t idx, register_uint32_t* reg, SHORT_STDPARAMS);
void extern_register_read__u64(uint64_t x, uint64_t* value_result, uint64_t idx, register_uint64_t* reg, SHORT_STDPARAMS);

// void write(in I index, in T value);
void extern_register_write__i8(int8_t x, int idx, int8_t value, register_int8_t* reg, SHORT_STDPARAMS);
void extern_register_write__i16(int16_t x, int idx, int16_t value, register_int16_t* reg, SHORT_STDPARAMS);
void extern_register_write__i32(int32_t x, int idx, int32_t value, register_int32_t* reg, SHORT_STDPARAMS);
void extern_register_write__i64(int64_t x, int idx, int64_t value, register_int64_t* reg, SHORT_STDPARAMS);
void extern_register_write__u8(uint8_t x, int idx, uint8_t value, register_uint8_t* reg, SHORT_STDPARAMS);
void extern_register_write__u16(uint16_t x, int idx, uint16_t value, register_uint16_t* reg, SHORT_STDPARAMS);
void extern_register_write__u32(uint32_t x, int idx, uint32_t value, register_uint32_t* reg, SHORT_STDPARAMS);
void extern_register_write__u64(uint64_t x, int idx, uint64_t value, register_uint64_t* reg, SHORT_STDPARAMS);
