// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "v1-boilerplate-pre.p4"

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

PARSER {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_INGRESS {
    apply {
        bit<16> input16 = 0x1234;
        bit<24> input24 = 0x123456;
        bit<32> input32 = 0x12345678;
        bit<40> input40 = 0x123456789a;
        bit<48> input48 = 0x123456789abc;
        bit<16> result16;
        bit<32> result32;

        hash(result16, HashAlgorithm.identity, (bit<16>)0, {input16}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x1234)", {result16});
        hash(result16, HashAlgorithm.identity, (bit<16>)0, {input24}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x1234)", {result16});
        hash(result16, HashAlgorithm.identity, (bit<16>)0, {input32}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x1234)", {result16});
        hash(result16, HashAlgorithm.identity, (bit<16>)0, {input40}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x1234)", {result16});
        hash(result16, HashAlgorithm.identity, (bit<16>)0, {input48}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x1234)", {result16});


        hash(result16, HashAlgorithm.xor16, (bit<16>)0, {input16}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x1234)", {result16});
        hash(result16, HashAlgorithm.xor16, (bit<16>)0, {input24}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x4434)", {result16});
        hash(result16, HashAlgorithm.xor16, (bit<16>)0, {input32}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x444C)", {result16});
        hash(result16, HashAlgorithm.xor16, (bit<16>)0, {input40}, (bit<32>)0);
        log_msg("Result = {} (expected: 0xde4c)", {result16});
        hash(result16, HashAlgorithm.xor16, (bit<16>)0, {input48}, (bit<32>)0);
        log_msg("Result = {} (expected: 0xdef0)", {result16});

        hash(result16, HashAlgorithm.random, (bit<16>)0, {input16}, (bit<32>)0);
        log_msg("Result = {} (expected: random 16 bit)", {result16});
        hash(result16, HashAlgorithm.random, (bit<16>)0, {input32}, (bit<32>)0);
        log_msg("Result = {} (expected: random 16 bit)", {result16});

        log_msg("======== 32 bit results");
        log_msg("");

        hash(result32, HashAlgorithm.identity, (bit<16>)0, {input16}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x00001234)", {result32});
        hash(result32, HashAlgorithm.identity, (bit<16>)0, {input24}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x00123456)", {result32});
        hash(result32, HashAlgorithm.identity, (bit<16>)0, {input32}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x12345678)", {result32});
        hash(result32, HashAlgorithm.identity, (bit<16>)0, {input40}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x12345678)", {result32});
        hash(result32, HashAlgorithm.identity, (bit<16>)0, {input48}, (bit<32>)0);
        log_msg("Result = {} (expected: 0x12345678)", {result32});


        hash(result32, HashAlgorithm.random, (bit<16>)0, {input16}, (bit<32>)0);
        log_msg("Result = {} (expected: random 16 bit)", {result32});
        hash(result32, HashAlgorithm.random, (bit<16>)0, {input32}, (bit<32>)0);
        log_msg("Result = {} (expected: random 16 bit)", {result32});

    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "v1-boilerplate-post.p4"
