// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Eotvos Lorand University, Budapest, Hungary

#include "common-boilerplate-pre.p4"

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
CTL_MAIN {
    apply {
        hdr.offset.offset = hdr.offset.offset + 8w1;
        md5_hmac({hdr.offset.offset});
        hdr.offset.offset = hdr.offset.offset - 8w1;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.offset);
    }
}


#include "common-boilerplate-post.p4"
