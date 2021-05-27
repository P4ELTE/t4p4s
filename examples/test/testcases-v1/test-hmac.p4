// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "v1-boilerplate-pre.p4"


struct metadata {
}

header offset_t {
    bit<8> offset;
}
struct headers {
    offset_t offset;
}

PARSER {
    state start {
        packet.extract(hdr.offset);
        transition accept;
    }
}

extern void md5_hmac<T>(in T offset);
extern void symmetric_encrypt<T>(out T result, in T data);
extern void do_encryption();
extern void do_decryption();
CTL_INGRESS {
    apply {
        hdr.offset.offset = hdr.offset.offset + 8w1;
        md5_hmac({hdr.offset.offset});
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.offset);
    }
}


#include "v1-boilerplate-post.p4"
