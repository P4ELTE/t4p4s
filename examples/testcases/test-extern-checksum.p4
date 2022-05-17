
#include "common-boilerplate-pre.p4"

struct metadata {
    /* empty */
}

struct headers {
    bits32_t result;
}

PARSER {
    state start {
        packet.extract(hdr.result);
        transition accept;
    }
}

#define CUSTOM_CTL_CHECKSUM 1

CUSTOM_VERIFY_CHECKSUM {
    apply {  }
}

CUSTOM_UPDATE_CHECKSUM {
    DECLARE_CHECKSUM(bit<32>, CRC16, checksum_var)
    apply {
        CALL_UPDATE_CHECKSUM(
            checksum_var,
            true,
            ({
                8w0,
                8w0,
                16w0,
                16w0,
                16w0
            }),
            hdr.result.f32,
            csum16);
        // checksum_var.update({
        //         8w0,
        //         8w0,
        //         16w0,
        //         16w0,
        //         16w0
        //     });
    }
}

CTL_MAIN {
    apply {
        SET_EGRESS_PORT(GET_INGRESS_PORT());
        CTL_VERIFY_CHECKSUM_APPLY();
        CTL_UPDATE_CHECKSUM_APPLY();
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.result);
    }
}

#include "common-boilerplate-post.p4"
