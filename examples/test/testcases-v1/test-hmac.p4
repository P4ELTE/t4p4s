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

extern void md5_hmac();
extern void symmetric_encrypt<T>(out T result, in T data);
extern void do_encryption();
extern void do_decryption();
CTL_INGRESS {
    apply {
        bit<64> input = 0x123456789abcde00;
        bit<32> output;
        //bit<64> output;
        log_msg("Test HMAC");
        //md5_hmac(output, input);
        md5_hmac();
        //do_encryption();
        //do_decryption();
        //symmetric_encrypt(output, input);
        //log_msg("Hmac result: {}",{output});
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}


#include "v1-boilerplate-post.p4"
