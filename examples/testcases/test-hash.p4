// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

struct metadata {
}

header expected_result_t {
    bit<16> f16_1;
    bit<16> f16_2;
    bit<16> f16_3;
    bit<16> f16_4;
    bit<16> f16_5;
    bit<16> f16_6;
    bit<16> f16_7;
    bit<16> f16_8;
    bit<16> f16_9;
    bit<16> f16_10;

    bit<32> f32_1;
    bit<32> f32_2;
    bit<32> f32_3;
    bit<32> f32_4;
    bit<32> f32_5;
}

struct headers {
    expected_result_t expected_result;
}

PARSER {
    state start {
        packet.extract(hdr.expected_result);
        transition accept;
    }
}

CTL_MAIN {
    apply {
        bit<16> input16 = 0x1234;
        bit<24> input24 = 0x123456;
        bit<32> input32 = 0x12345678;
        bit<40> input40 = 0x123456789a;
        bit<48> input48 = 0x123456789abc;
        bit<16> result16;
        bit<32> result32;

        HASH(result16, HashAlgorithm.identity, (bit<16>)0, {input16}, (bit<32>)0);
        hdr.expected_result.f16_1 = 0x1234;
        HASH(result16, HashAlgorithm.identity, (bit<16>)0, {input24}, (bit<32>)0);
        hdr.expected_result.f16_2 = 0x1234;
        HASH(result16, HashAlgorithm.identity, (bit<16>)0, {input32}, (bit<32>)0);
        hdr.expected_result.f16_3 = 0x1234;
        HASH(result16, HashAlgorithm.identity, (bit<16>)0, {input40}, (bit<32>)0);
        hdr.expected_result.f16_4 = 0x1234;
        HASH(result16, HashAlgorithm.identity, (bit<16>)0, {input48}, (bit<32>)0);
        hdr.expected_result.f16_5 = 0x1234;


        HASH(result16, HashAlgorithm.xor16, (bit<16>)0, {input16}, (bit<32>)0);
        hdr.expected_result.f16_6 = 0x1234;
        HASH(result16, HashAlgorithm.xor16, (bit<16>)0, {input24}, (bit<32>)0);
        hdr.expected_result.f16_7 = 0x4434;
        HASH(result16, HashAlgorithm.xor16, (bit<16>)0, {input32}, (bit<32>)0);
        hdr.expected_result.f16_8 = 0x444C;
        HASH(result16, HashAlgorithm.xor16, (bit<16>)0, {input40}, (bit<32>)0);
        hdr.expected_result.f16_9 = 0xde4c;
        HASH(result16, HashAlgorithm.xor16, (bit<16>)0, {input48}, (bit<32>)0);
        hdr.expected_result.f16_10 = 0xdef0;

        HASH(result16, HashAlgorithm.random, (bit<16>)0, {input16}, (bit<32>)0);
        LOG("Result = {} (expected: random 16 bit)", {result16});
        HASH(result16, HashAlgorithm.random, (bit<16>)0, {input32}, (bit<32>)0);
        LOG("Result = {} (expected: random 16 bit)", {result16});


        HASH(result32, HashAlgorithm.identity, (bit<16>)0, {input16}, (bit<32>)0);
        hdr.expected_result.f32_1 = 0x00001234;
        HASH(result32, HashAlgorithm.identity, (bit<16>)0, {input24}, (bit<32>)0);
        hdr.expected_result.f32_2 = 0x00123456;
        HASH(result32, HashAlgorithm.identity, (bit<16>)0, {input32}, (bit<32>)0);
        hdr.expected_result.f32_3 = 0x12345678;
        HASH(result32, HashAlgorithm.identity, (bit<16>)0, {input40}, (bit<32>)0);
        hdr.expected_result.f32_4 = 0x12345678;
        HASH(result32, HashAlgorithm.identity, (bit<16>)0, {input48}, (bit<32>)0);
        hdr.expected_result.f32_5 = 0x12345678;

        HASH(result32, HashAlgorithm.random, (bit<16>)0, {input16}, (bit<32>)0);
        LOG("Result = {} (expected: random 32 bit)", {result32});
        HASH(result32, HashAlgorithm.random, (bit<16>)0, {input32}, (bit<32>)0);
        LOG("Result = {} (expected: random 32 bit)", {result32});
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.expected_result);
    }
}

#include "common-boilerplate-post.p4"
