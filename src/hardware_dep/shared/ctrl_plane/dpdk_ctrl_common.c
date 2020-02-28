// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

extern controller c;

void notify_controller_initialized()
{
    char buffer[sizeof(struct p4_header)];
    struct p4_header* h;

    h = create_p4_header(buffer, 0, sizeof(struct p4_header));
    h->type = P4T_CTRL_INITIALIZED;

    netconv_p4_header(h);

    send_p4_msg(c, buffer, sizeof(struct p4_header));
}
