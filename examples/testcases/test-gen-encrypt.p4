
#include "common-boilerplate-pre.p4"

enum bit<2> Op { KEEP = 0, ENCRYPT = 1, DECRYPT = 2, ENCRYPT_THEN_DECRYPT = 3 }

struct metadata {}

header padded_op_t {
    bit<6> padding;
    Op     op;
}

struct headers {
    padded_op_t op;
    ethernet_t  ethernet;
}

extern void encrypt<T>(in T offset);
extern void decrypt<T>(in T offset);

PARSER {
    state start {
        packet.extract(hdr.op);
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_MAIN {
    apply {
        Op op = hdr.op.op;
        hdr.op.setInvalid();
        bit<8> offset = 0;
        if (op == Op.ENCRYPT || op == Op.ENCRYPT_THEN_DECRYPT) {
            encrypt({offset});
        }
        if (op == Op.DECRYPT || op == Op.ENCRYPT_THEN_DECRYPT) {
            decrypt({offset});
        }
    }

}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "common-boilerplate-post.p4"
