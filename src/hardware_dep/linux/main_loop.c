// Copyright 2016 Eotvos Lorand University, Budapest, Hungary
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "compiler.h"
#include "ethdev.h"
#include "linux_backend.h"
#include "pktbuf.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#ifdef CLOCK_MONOTONIC_RAW
#define CLOCK_TYPE CLOCK_MONOTONIC_RAW
#else
#define CLOCK_TYPE CLOCK_MONOTONIC
#endif

#ifndef ETHDEV_RX_NOCOPY
static uint8_t rx_buffer[RX_BURST_MAX * RX_FRAME_SIZE];
#endif
static struct pktbuf rx_pktbufs[RX_BURST_MAX];
static volatile sig_atomic_t interrupted;

static void sigint_handler(int signo)
{
    interrupted = 1;
}

static always_inline uint32_t get_ingress_port(packet_descriptor_t* pd)
{
    return GET_INT32_AUTO(pd, field_instance_standard_metadata_ingress_port);
}

static always_inline uint32_t get_egress_port(packet_descriptor_t* pd)
{
    return GET_INT32_AUTO(pd, field_instance_standard_metadata_egress_port);
}

static always_inline void set_ingress_port(packet_descriptor_t* pd, uint32_t port)
{
    uint32_t res32;
    MODIFY_INT32_INT32_BITS(pd, field_instance_standard_metadata_ingress_port, port);
}

static always_inline void ucast_packet(struct pktbuf* p, uint8_t port)
{
    struct ethdev* const ethdev = &ports[port];

    ethdev_txq_enqueue(ethdev, p);

    if (unlikely(ethdev_txq_length(ethdev) == TX_QUEUE_LENGTH))
    {
        debug("### PORT %u -- TX BURST (full)\n", port);
        debug("  :::: packets: %u\n", ethdev_txq_length(ethdev));

        ethdev_txq_send(ethdev);
    }
}

static always_inline void bcast_packet(struct pktbuf* p, uint8_t ingress_port)
{
    uint8_t port;

    for (port = 0; port < ingress_port; ++port)
        ucast_packet(p, port);
    for (++port; port < port_count; ++port)
        ucast_packet(p, port);
}

static always_inline void mcast_packet(struct pktbuf* p, uint32_t port_mask)
{
    uint8_t port;

    for (port = 0; port_mask != 0 && port < port_count; port_mask >>= 1, ++port)
    {
        if ((port_mask & 1) != 0)
            ucast_packet(p, port);
    }
}

static always_inline void send_packet(packet_descriptor_t* pd)
{
    if (pd->dropped)
        return;

    const uint8_t ingress_port = (uint8_t)get_ingress_port(pd);
    const uint8_t egress_port = (uint8_t)get_egress_port(pd);

    if (egress_port != BROADCAST_PORT)
        ucast_packet(pd->wrapper, egress_port);
    else
        bcast_packet(pd->wrapper, ingress_port);
}

static void packet_received(packet_descriptor_t* pd, struct pktbuf* p, uint8_t port)
{
    pd->data = p->data;
    pd->wrapper = p;
    pd->dropped = 0;
    reset_headers(pd);
    set_ingress_port(pd, port);
    handle_packet(pd, tables);
    send_packet(pd);
}

void main_loop(void)
{
    uint8_t port;
    uint16_t received;
    unsigned int i;
    struct sigaction sigact;
#ifdef TX_BURST_DRAIN_TIMEOUT
    struct timespec prev, curr, diff;
#endif
    packet_descriptor_t pd;

    memset(&sigact, 0, sizeof(struct sigaction));
	sigemptyset(&sigact.sa_mask);
    sigact.sa_handler = sigint_handler;

    if (sigaction(SIGINT, &sigact, NULL) == -1)
    {
        print_errno("sigaction");
        return;
    }

#ifndef ETHDEV_RX_NOCOPY
    uint8_t* buffer = rx_buffer;
    for (i = 0; i < RX_BURST_MAX; ++i)
    {
        pktbuf_use_with_headroom(&rx_pktbufs[i], buffer, RX_FRAME_SIZE, RX_FRAME_HEADROOM);
        buffer += RX_FRAME_SIZE;
    }
#endif

    init_dataplane(&pd, tables);

#ifdef TX_BURST_DRAIN_TIMEOUT
    clock_gettime(CLOCK_TYPE, &prev);
#endif

    while (likely(!interrupted))
    {
#ifdef TX_BURST_DRAIN_TIMEOUT
        clock_gettime(CLOCK_TYPE, &curr);
        timespec_diff(&prev, &curr, &diff);

        if (diff.tv_nsec >= TX_BURST_DRAIN_NS || diff.tv_sec != 0)
#endif
        {
            for (port = 0; port < port_count; ++port)
            {
                if (ethdev_txq_length(&ports[port]) != 0)
                {
                    debug("### PORT %u -- TX BURST (drain)\n", port);
                    debug("  :::: packets: %u\n", ethdev_txq_length(&ports[port]));
                    
                    ethdev_txq_send(&ports[port]);
                }
            }
        }
#ifdef TX_BURST_DRAIN_TIMEOUT
        prev = curr;
#endif

        for (port = 0; port < port_count; ++port)
        {
            received = (uint16_t)ethdev_rx_burst(&ports[port], rx_pktbufs, RX_BURST_MAX);

            if (received)
            {
                debug("### PORT %u -- RX BURST\n", port);
                debug("  :::: packets (max: %u): %u\n", RX_BURST_MAX, received);
            }

            for (i = 0; i < received; ++i)
            {
                packet_received(&pd, &rx_pktbufs[i], port);
#if !defined(ETHDEV_NO_PACKET_MMAP) && defined(ETHDEV_RX_NOCOPY)
                ethdev_flush_packet(&rx_pktbufs[i]);
#endif
            }
        }
    }

    sigact.sa_handler = SIG_DFL;

    if (sigaction(SIGINT, &sigact, NULL) == -1)
        print_errno("sigaction");
}
