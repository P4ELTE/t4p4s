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

#ifndef ETHDEV_H
#define ETHDEV_H

#include "compiler.h"
#include "pktbuf.h"

#include <stdint.h>

#include <linux/if_packet.h>
#include <net/if.h>

#define ETH_HEADER_SIZE 14
#define ETH_VLAN_TAG_SIZE 4
#define ETH_PAYLOAD_SIZE_MIN 46
#define ETH_PAYLOAD_SIZE_MAX 1500
#define ETH_FRAME_SIZE_MIN (ETH_HEADER_SIZE + ETH_PAYLOAD_SIZE_MIN)
#define ETH_FRAME_SIZE_MAX (ETH_HEADER_SIZE + ETH_VLAN_TAG_SIZE + ETH_PAYLOAD_SIZE_MAX)

#define ETHDEV_TIMEOUT_INFINITE -1

struct iovec;
struct mmsghdr;

struct ethdev_parameters
{
    const char* if_name;
    uint16_t rx_frame_size;
    uint16_t rx_frame_nr;
    uint16_t rx_frame_headroom;
    uint16_t tx_frame_size;
    uint16_t tx_frame_nr;
};

struct ethdev_buffer
{
    void* ptr;
    uint16_t frame_size;
    uint16_t frame_nr;
    uint16_t frame_headroom;
    uint16_t enqueued;
    struct iovec* frames;
    struct mmsghdr* messages;
    struct sockaddr_ll* ll_info;
};

struct ethdev_ring
{
    void* map;
    uint32_t map_size;
    uint16_t frame_size;
    uint16_t frame_nr;
    uint16_t frame_headroom;
    uint16_t index;
    uint16_t index_mask;
    uint16_t enqueued;
    struct iovec* frames;
};

union ethdev_buffer_u
{
    struct ethdev_buffer buffer;
    struct ethdev_ring ring;
};

struct ethdev_interface
{
    uint32_t index;
    char name[IF_NAMESIZE];
    uint32_t mtu;
    uint16_t flags;
    uint16_t features;
};

struct ethdev
{
    struct ethdev_interface interface;
    int socket_fd;
    union ethdev_buffer_u rx;
    union ethdev_buffer_u tx;
};

int ethdev_open(struct ethdev* ethdev, const struct ethdev_parameters* parameters);
void ethdev_close(struct ethdev* ethdev);

int32_t ethdev_rx_burst(struct ethdev* ethdev, struct pktbuf* packets, uint16_t packet_nr);
int32_t ethdev_rx_burst_poll(struct ethdev* ethdev, struct pktbuf* packets, uint16_t packet_nr, int timeout);

int32_t ethdev_tx_burst(struct ethdev* ethdev, const struct pktbuf* packets, uint16_t packet_nr);
int32_t ethdev_tx_burst_poll(struct ethdev* ethdev, const struct pktbuf* packets, uint16_t packet_nr, int timeout);

uint16_t ethdev_txq_length(const struct ethdev* ethdev);
int32_t ethdev_txq_enqueue(struct ethdev* ethdev, const struct pktbuf* packet);
int32_t ethdev_txq_clear(struct ethdev* ethdev);
int32_t ethdev_txq_send(struct ethdev* ethdev);

#if !defined(ETHDEV_NO_PACKET_MMAP) && defined(ETHDEV_RX_NOCOPY)
static always_inline void ethdev_flush_packet(struct pktbuf* const packet)
{
    *(__u32*)packet->pointer = TP_STATUS_KERNEL;
}
#endif

#endif
