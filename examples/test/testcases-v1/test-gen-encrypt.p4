#include "v1-boilerplate-pre.p4"

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

extern void do_encryption_async();
extern void do_decryption_async();

PARSER {
    state start {
        packet.extract(hdr.op);
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

CTL_INGRESS {
    apply {
        Op op = hdr.op.op;
        hdr.op.setInvalid();
        if (op == Op.ENCRYPT || op == Op.ENCRYPT_THEN_DECRYPT) {
            do_encryption_async();
        }
        if (op == Op.DECRYPT || op == Op.ENCRYPT_THEN_DECRYPT) {
            do_decryption_async();
        }
    }

}

CTL_EMIT {
    apply {
        packet.emit(hdr.ethernet);
    }
}

#include "v1-boilerplate-post.p4"
