
#include "common-boilerplate-pre.p4"

struct metadata {
}

header all_t {
    bool   f1;
    bool   f2;
    bool   f3;
    bool   f4;
    bool   f5;
    bool   f6;
    bool   f7;
    bool   f8;
}

header bottom_t {
    bit<4> padding;
    bool   f5;
    bool   f6;
    bool   f7;
    bool   f8;
}

struct headers {
    all_t all;
}

PARSER {
    state start {
        packet.extract(hdr.all);
        transition accept;
    }
}

CTL_MAIN {
    bottom_t bot;

    action action_top1(inout all_t all, bool f1) {
        all.f1 = true;
    }

    action action_top(inout all_t all, bool f1, bool f2, bool f3, bool f4) {
        all.f1 = f1;
        all.f2 = f2;
        all.f3 = f3;
        all.f4 = f4;
    }

    action action_bottom(out bottom_t bottom, bool f5, bool f6, bool f7, bool f8) {
        bottom.setValid();
        bottom.f5 = f5;
        bottom.f6 = f6;
        bottom.f7 = f7;
        bottom.f8 = f8;
    }

    apply {
        action_top(hdr.all, hdr.all.f1 || true, hdr.all.f2 || true, hdr.all.f3 || true, hdr.all.f4 || true);
        action_top1(hdr.all, hdr.all.f1 || true);
        //action_top1(hdr.all, true);
        //hdr.all.f1 = true;

        action_bottom(bot, hdr.all.f1 || true, hdr.all.f2 || true, hdr.all.f3 || true, hdr.all.f4 || true);
        hdr.all.f5 = bot.f5;
        hdr.all.f6 = bot.f6;
        hdr.all.f7 = bot.f7;
        hdr.all.f8 = bot.f8;
    }
}

CTL_EMIT {
    apply {
        packet.emit(hdr.all);
    }
}

#include "common-boilerplate-post.p4"
