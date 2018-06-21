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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ethdev.h"

#include "compiler.h"
#include "lib.h"
#include "pktbuf.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define ETHDEV_PACKET_VERSION TPACKET_V2
#define ETHDEV_PACKET_HDRLEN TPACKET_ALIGN(sizeof(struct tpacket2_hdr))

#ifdef OPENWRT
#define RX_PACKET_FILTER (PACKET_MASK_ANY & ~(1 << PACKET_LOOPBACK) & ~(1 << PACKET_OUTGOING))
#endif

#define RX_SOCKADDR_LL_OFFSET ETHDEV_PACKET_HDRLEN
#define TX_DATA_OFFSET ETHDEV_PACKET_HDRLEN

#define MAX_ORDER 4

#define F_FLAG_VALUE(x) ((uint16_t)(1ull << (x)))
#define F_FLAG(name)    F_FLAG_VALUE(FEATURE_##name)
#define F_GETCMD(name)  ETHTOOL_G##name
#define F_SETCMD(name)  ETHTOOL_S##name
#define F_GETSTR(name)  "ETHTOOL_G"#name
#define F_SETSTR(name)  "ETHTOOL_S"#name
#define F_DATA(name)    {F_FLAG(name), F_GETCMD(name), F_SETCMD(name), F_GETSTR(name), F_SETSTR(name)}

enum features
{
    FEATURE_RXCSUM = 0,
    FEATURE_TXCSUM,
    FEATURE_GRO,
    FEATURE_GSO,
    FEATURE_TSO,
    FEATURE_UFO,
    FEATURE_COUNT
};

struct feature_data
{
    const uint16_t flag;
    const uint32_t get_cmd;
    const uint32_t set_cmd;
    const char* const get_str;
    const char* const set_str;
};

static const struct feature_data feature_data[FEATURE_COUNT] =
{
    [FEATURE_RXCSUM] = F_DATA(RXCSUM),
    [FEATURE_TXCSUM] = F_DATA(TXCSUM),
    [FEATURE_GRO]    = F_DATA(GRO),
    [FEATURE_GSO]    = F_DATA(GSO),
    [FEATURE_TSO]    = F_DATA(TSO),
    [FEATURE_UFO]    = F_DATA(UFO),
};

static int32_t change_interface_features(int control_fd, struct ifreq* if_req, uint16_t features)
{
    char message[64];
    const char* command_str;
    struct ethtool_value ethtool_value;
    unsigned int i;
    uint16_t original_features = 0;

    if_req->ifr_data = (caddr_t)&ethtool_value;

    for (i = 0; i < FEATURE_COUNT; ++i)
    {
        ethtool_value.cmd = feature_data[i].get_cmd;

        if (ioctl(control_fd, SIOCETHTOOL, if_req) == -1 && errno != ENOTSUP)
        {
            command_str = feature_data[i].get_str;
            goto error;
        }

        original_features |= ethtool_value.data ? feature_data[i].flag : 0;

        ethtool_value.cmd = feature_data[i].set_cmd;
        ethtool_value.data = features & 1u;

        if (ioctl(control_fd, SIOCETHTOOL, if_req) == -1 && errno != ENOTSUP)
        {
            command_str = feature_data[i].set_str;
            goto error;
        }

        features >>= 1;
    }

    return original_features;

error:
    snprintf(message, sizeof(message), "ioctl (SIOCETHTOOL / %s)", command_str);
    print_errno(message);

    return -1;
}

static int configure_interface(struct ethdev* ethdev, const char* if_name)
{
    int control_fd, if_index, if_mtu, if_flags;
    int32_t if_features;
    struct ifreq if_req;

    control_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (control_fd < 0)
    {
        print_errno("socket");
        return -1;
    }

    strncpy(if_req.ifr_name, if_name, IF_NAMESIZE);

    if_index = if_nametoindex(if_name);
    if (if_index == 0)
    {
        print_errno("if_nametoindex");
        goto error;
    }

    if (ioctl(control_fd, SIOCGIFMTU, &if_req) == -1)
    {
        print_errno("ioctl (SIOCGIFMTU)");
        goto error;
    }
    if_mtu = if_req.ifr_mtu;

    if (ioctl(control_fd, SIOCGIFFLAGS, &if_req) == -1)
    {
        print_errno("ioctl (SIOCGIFFLAGS)");
        goto error;
    }
    if_flags = if_req.ifr_flags;

    if_req.ifr_flags |= IFF_PROMISC;
    if (ioctl(control_fd, SIOCSIFFLAGS, &if_req) == -1)
    {
        print_errno("ioctl (SIOCSIFFLAGS)");
        goto error;
    }

    if_features = change_interface_features(control_fd, &if_req, 0);
    if (if_features < 0)
        goto error;
    
    close(control_fd);

    ethdev->interface.index = if_index;
    strncpy(ethdev->interface.name, if_name, IF_NAMESIZE);
    ethdev->interface.mtu = if_mtu;
    ethdev->interface.flags = if_flags;
    ethdev->interface.features = if_features;

    return 0;

error:
    close(control_fd);

    ethdev->interface.index = 0;

    return -1;
}

static int restore_interface(const struct ethdev* ethdev)
{
    int result, control_fd;
    struct ifreq if_req;

    if (ethdev->interface.index == 0)
        return 0;

    result = 0;

    control_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (control_fd < 0)
    {
        print_errno("socket");
        return -1;
    }

    strncpy(if_req.ifr_name, ethdev->interface.name, IF_NAMESIZE);

    if_req.ifr_flags = ethdev->interface.flags;
    if (ioctl(control_fd, SIOCSIFFLAGS, &if_req) == -1)
    {
        print_errno("ioctl (SIOCSIFFLAGS)");
        result = -1;
    }

    if (change_interface_features(control_fd, &if_req, ethdev->interface.features) < 0)
        result = -1;

    close(control_fd);

    return result;
}

static int setup_buffers(struct ethdev* ethdev, const struct ethdev_parameters* parameters, int socket_fd)
{
    int sockopt;
    socklen_t sockopt_size;
    uint16_t i;
    uint8_t* rx_buffer, * tx_buffer;
    struct iovec* rx_frames, * tx_frames;
    struct mmsghdr* rx_messages, * tx_messages;
    struct sockaddr_ll* rx_ll_info;

    if (getsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &sockopt, &sockopt_size) == -1)
    {
        print_errno("getsockopt (SO_RCVBUF)");
        return -1;
    }

    if ((sockopt / 2) < parameters->rx_frame_nr * ETH_FRAME_SIZE_MAX)
    {
        sockopt = parameters->rx_frame_nr * ETH_FRAME_SIZE_MAX;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &sockopt, sizeof(sockopt)) == -1)
        {
            print_errno("setsockopt (SO_RCVBUF)");
            return -1;
        }
    }

    if (getsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &sockopt, &sockopt_size) == -1)
    {
        print_errno("getsockopt (SO_SNDBUF)");
        return -1;
    }

    if ((sockopt / 2) < parameters->tx_frame_nr * ETH_FRAME_SIZE_MAX)
    {
        sockopt = parameters->tx_frame_nr * ETH_FRAME_SIZE_MAX;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &sockopt, sizeof(sockopt)) == -1)
        {
            print_errno("setsockopt (SO_SNDBUF)");
            return -1;
        }
    }

    rx_buffer = malloc(parameters->rx_frame_nr * parameters->rx_frame_size);
    rx_frames = malloc(parameters->rx_frame_nr * sizeof(struct iovec));
    rx_messages = malloc(parameters->rx_frame_nr * sizeof(struct mmsghdr));
    rx_ll_info = malloc(parameters->rx_frame_nr * sizeof(struct sockaddr_ll));

    for (i = 0; i < parameters->rx_frame_nr; ++i)
    {
        rx_frames[i].iov_base = rx_buffer + i * parameters->rx_frame_size + parameters->rx_frame_headroom;
        rx_frames[i].iov_len = parameters->rx_frame_size - parameters->rx_frame_headroom;

        rx_messages[i].msg_hdr.msg_name = &rx_ll_info[i];
        rx_messages[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_ll);
        rx_messages[i].msg_hdr.msg_iov = &rx_frames[i];
        rx_messages[i].msg_hdr.msg_iovlen = 1;
        rx_messages[i].msg_hdr.msg_control = NULL;
        rx_messages[i].msg_hdr.msg_controllen = 0;
        rx_messages[i].msg_hdr.msg_flags = 0;
    }

    tx_buffer = malloc(parameters->tx_frame_nr * parameters->tx_frame_size);
    tx_frames = malloc(parameters->tx_frame_nr * sizeof(struct iovec));
    tx_messages = malloc(parameters->tx_frame_nr * sizeof(struct mmsghdr));

    for (i = 0; i < parameters->tx_frame_nr; ++i)
    {
        tx_frames[i].iov_base = tx_buffer + i * parameters->tx_frame_size;
        tx_frames[i].iov_len = 0;

        tx_messages[i].msg_hdr.msg_name = NULL;
        tx_messages[i].msg_hdr.msg_namelen = 0;
        tx_messages[i].msg_hdr.msg_iov = &tx_frames[i];
        tx_messages[i].msg_hdr.msg_iovlen = 1;
        tx_messages[i].msg_hdr.msg_control = NULL;
        tx_messages[i].msg_hdr.msg_controllen = 0;
        tx_messages[i].msg_hdr.msg_flags = 0;
    }

    ethdev->rx.buffer.ptr = rx_buffer;
    ethdev->rx.buffer.frame_size = parameters->rx_frame_size;
    ethdev->rx.buffer.frame_nr = parameters->rx_frame_nr;
    ethdev->rx.buffer.frame_headroom = parameters->rx_frame_headroom;
    ethdev->rx.buffer.frames = rx_frames;
    ethdev->rx.buffer.messages = rx_messages;
    ethdev->rx.buffer.ll_info = rx_ll_info;
    ethdev->rx.buffer.enqueued = UINT16_MAX;

    ethdev->tx.buffer.ptr = tx_buffer;
    ethdev->tx.buffer.frame_size = parameters->tx_frame_size;
    ethdev->tx.buffer.frame_nr = parameters->tx_frame_nr;
    ethdev->tx.buffer.frame_headroom = 0;
    ethdev->tx.buffer.frames = tx_frames;
    ethdev->tx.buffer.messages = tx_messages;
    ethdev->tx.buffer.ll_info = NULL;
    ethdev->tx.buffer.enqueued = 0;

    return 0;
}

static void free_buffers(struct ethdev* ethdev)
{
    free(ethdev->rx.buffer.ll_info);
    free(ethdev->rx.buffer.messages);
    free(ethdev->rx.buffer.frames);
    free(ethdev->rx.buffer.ptr);
    free(ethdev->tx.buffer.ll_info);
    free(ethdev->tx.buffer.messages);
    free(ethdev->tx.buffer.frames);
    free(ethdev->tx.buffer.ptr);
}

static uint16_t setup_ring_params(struct tpacket_req* tp_req, uint16_t frame_size, uint16_t frame_nr)
{
    /*
    tp_block_size must be a multiple of PAGE_SIZE (1)
    tp_frame_size must be greater than TPACKET_HDRLEN (obvious)
    tp_frame_size must be a multiple of TPACKET_ALIGNMENT
    tp_frame_nr   must be exactly frames_per_block*tp_block_nr
    */

    uint32_t page_size = sysconf(_SC_PAGESIZE);
    uint32_t max_block_size = page_size << MAX_ORDER;
    uint32_t block_size;
    uint16_t block_frames, block_nr;

    frame_size = TPACKET_ALIGN(frame_size);

    if (frame_size <= (ETHDEV_PACKET_HDRLEN + sizeof(struct sockaddr_ll)))
    {
        errno = EINVAL;
        return 0;
    }

    block_size = page_size;
    while (block_size < frame_size)
    {
        block_size += page_size;
    }

    if (block_size > max_block_size)
    {
        errno = EINVAL;
        return 0;
    }

    block_frames = block_size / frame_size;
    block_nr = DIV_ROUND_UP(frame_nr, block_frames);

    if (frame_nr != block_nr * block_frames)
    {
        errno = EINVAL;
        return 0;
    }

    tp_req->tp_frame_size = frame_size;
    tp_req->tp_frame_nr = frame_nr;
    tp_req->tp_block_size = block_size;
    tp_req->tp_block_nr = block_nr;

    return block_frames;
}

static int setup_rings(struct ethdev* ethdev, const struct ethdev_parameters* parameters, int socket_fd)
{
    int sockopt;
    uint16_t rx_block_frames, tx_block_frames, block, frame, i;
    size_t rx_map_size, tx_map_size;
    uint8_t* rx_map, * tx_map;
    struct iovec* rx_frames, * tx_frames;
    struct tpacket_req rx_req, tx_req;

    rx_block_frames = setup_ring_params(&rx_req, parameters->rx_frame_size, parameters->rx_frame_nr);
    if (rx_block_frames == 0)
    {
        print_errno("setup_ring_params (RX)");
        return -1;
    }

    tx_block_frames = setup_ring_params(&tx_req, parameters->tx_frame_size, parameters->tx_frame_nr);
    if (tx_block_frames == 0)
    {
        print_errno("setup_ring_params (TX)");
        return -1;
    }

    sockopt = 1;
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_LOSS, &sockopt, sizeof(sockopt)) == -1)
    {
        print_errno("setsockopt (PACKET_LOSS)");
        return -1;
    }

    sockopt = ETHDEV_PACKET_VERSION;
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_VERSION, &sockopt, sizeof(sockopt)) == -1)
    {
        print_errno("setsockopt (PACKET_VERSION)");
        return -1;
    }

    sockopt = parameters->rx_frame_headroom;
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_RESERVE, &sockopt, sizeof(sockopt)) == -1)
    {
        print_errno("setsockopt (PACKET_RESERVE)");
        return -1;
    }

    if (setsockopt(socket_fd, SOL_PACKET, PACKET_RX_RING, &rx_req, sizeof(rx_req)) == -1)
    {
        print_errno("setsockopt (PACKET_RX_RING)");
        return -1;
    }

    if (setsockopt(socket_fd, SOL_PACKET, PACKET_TX_RING, &tx_req, sizeof(tx_req)) == -1)
    {
        print_errno("setsockopt (PACKET_TX_RING)");
        return -1;
    }

    rx_map_size = rx_req.tp_block_size * rx_req.tp_block_nr;
    tx_map_size = tx_req.tp_block_size * tx_req.tp_block_nr;

    rx_map = mmap(NULL, rx_map_size + tx_map_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED | MAP_POPULATE, socket_fd, 0);
    if (rx_map == MAP_FAILED)
    {
        print_errno("mmap");
        return -1;
    }
    tx_map = rx_map + rx_map_size;

    rx_frames = malloc(sizeof(*rx_frames) * rx_req.tp_frame_nr);
    tx_frames = malloc(sizeof(*tx_frames) * tx_req.tp_frame_nr);

    i = 0;
    for (block = 0; block < rx_req.tp_block_nr; ++block)
    {
        for (frame = 0; frame < rx_block_frames; ++frame)
        {
            rx_frames[i].iov_base = rx_map + block * rx_req.tp_block_size + frame * rx_req.tp_frame_size;
            rx_frames[i].iov_len = rx_req.tp_frame_size;
            i++;
        }
    }

    i = 0;
    for (block = 0; block < tx_req.tp_block_nr; ++block)
    {
        for (frame = 0; frame < tx_block_frames; ++frame)
        {
            tx_frames[i].iov_base = tx_map + block * tx_req.tp_block_size + frame * tx_req.tp_frame_size;
            tx_frames[i].iov_len = tx_req.tp_frame_size;
            i++;
        }
    }

    ethdev->rx.ring.map = rx_map;
    ethdev->rx.ring.map_size = rx_map_size;
    ethdev->rx.ring.frame_size = rx_req.tp_frame_size;
    ethdev->rx.ring.frames = rx_frames;
    ethdev->rx.ring.frame_nr = rx_req.tp_frame_nr;
    ethdev->rx.ring.frame_headroom = parameters->rx_frame_headroom;
    ethdev->rx.ring.index = 0;
    ethdev->rx.ring.index_mask = rx_req.tp_frame_nr - 1;
    ethdev->rx.ring.enqueued = UINT16_MAX;

    ethdev->tx.ring.map = tx_map;
    ethdev->tx.ring.map_size = tx_map_size;
    ethdev->tx.ring.frame_size = tx_req.tp_frame_size;
    ethdev->tx.ring.frames = tx_frames;
    ethdev->tx.ring.frame_nr = tx_req.tp_frame_nr;
    ethdev->tx.ring.frame_headroom = 0;
    ethdev->tx.ring.index = 0;
    ethdev->tx.ring.index_mask = tx_req.tp_frame_nr - 1;
    ethdev->tx.ring.enqueued = 0;

    return 0;
}

static void free_rings(struct ethdev* ethdev)
{
    munmap(ethdev->rx.ring.map, ethdev->rx.ring.map_size + ethdev->tx.ring.map_size);
    free(ethdev->rx.ring.frames);
    free(ethdev->tx.ring.frames);
}

static int init_socket(struct ethdev* ethdev, const struct ethdev_parameters* parameters)
{
    int socket_fd, sockopt;
    struct sockaddr_ll sll;

    socket_fd = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, 0);
    if (socket_fd < 0)
    {
        print_errno("socket");
        return -1;
    }

#ifdef OPENWRT
    sockopt = RX_PACKET_FILTER;
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_RECV_TYPE, &sockopt, sizeof(sockopt)) == -1)
    {
        print_errno("setsockopt (PACKET_RECV_TYPE)");
        goto error;
    }
#endif

#ifdef ETHDEV_QDISC_BYPASS
    sockopt = 1;
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_QDISC_BYPASS, &sockopt, sizeof(sockopt)) == -1)
    {
        print_errno("setsockopt (PACKET_QDISC_BYPASS)");
        goto error;
    }
#endif

#ifndef ETHDEV_NO_PACKET_MMAP
    if (setup_rings(ethdev, parameters, socket_fd) == -1)
        goto error;
#else
    if (setup_buffers(ethdev, parameters, socket_fd) == -1)
        goto error;
#endif

    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ethdev->interface.index;
    if (bind(socket_fd, (struct sockaddr*)&sll, sizeof(sll)) == -1)
    {
        print_errno("bind");
        goto error_bind;
    }

    return socket_fd;

error_bind:
#ifndef ETHDEV_NO_PACKET_MMAP
    free_rings(ethdev);
#else
    free_buffers(ethdev);
#endif
error:
    close(socket_fd);

    return -1;
}

static always_inline int32_t rx_burst(struct ethdev* ethdev, struct pktbuf* packets, uint16_t packet_nr)
{
    const struct mmsghdr* message;
    const struct iovec* frame;
    const struct sockaddr_ll* sll;
    int received, i, j;

    assert(packet_nr <= ethdev->rx.buffer.frame_nr);

    received = recvmmsg(ethdev->socket_fd, ethdev->rx.buffer.messages, packet_nr, 0, NULL);
    if (received == -1)
    {
        if (unlikely(errno != EWOULDBLOCK))
            print_errno("recvmmsg");
    }

    j = 0;
    for (i = 0; i < received; ++i)
    {
        message = &ethdev->rx.buffer.messages[i];
        frame = &ethdev->rx.buffer.frames[i];
        sll = &ethdev->rx.buffer.ll_info[i];

        if (unlikely(message->msg_hdr.msg_flags & MSG_TRUNC))
            continue;

#ifndef OPENWRT
        if (sll->sll_pkttype == PACKET_OUTGOING)
            continue;
#endif

#ifndef ETHDEV_RX_NOCOPY
        pktbuf_set_data(&packets[j++], frame->iov_base, message->msg_len);
#else
        pktbuf_use_data_with_headroom(&packets[j++], frame->iov_base, message->msg_len, ethdev->rx.buffer.frame_headroom);
#endif
    }

    received = j;

    return received;
}

static always_inline int32_t rx_burst_mmap(struct ethdev* ethdev, struct pktbuf* packets, uint16_t packet_nr)
{
    volatile const uint8_t* frame, * data;
    volatile const struct sockaddr_ll* sll;
    volatile struct tpacket2_hdr* hdr;
    uint16_t received;

    assert(packet_nr <= ethdev->rx.ring.frame_nr);

    received = 0;
    while (received < packet_nr)
    {
        frame = ethdev->rx.ring.frames[ethdev->rx.ring.index].iov_base;
        hdr = (struct tpacket2_hdr*)frame;
        sll = (const struct sockaddr_ll*)(frame + RX_SOCKADDR_LL_OFFSET);
        data = frame + hdr->tp_mac;

        if (hdr->tp_status & TP_STATUS_USER)
        {
            if (unlikely(hdr->tp_len > hdr->tp_snaplen))
            {
                hdr->tp_status = TP_STATUS_KERNEL;
                ethdev->rx.ring.index = (ethdev->rx.ring.index + 1) & ethdev->rx.ring.index_mask;
                continue;
            }

#ifndef OPENWRT
            if (sll->sll_pkttype != PACKET_OUTGOING)
            {
#endif
#ifndef ETHDEV_RX_NOCOPY
                pktbuf_set_data(&packets[received], (void*)data, hdr->tp_snaplen);
                hdr->tp_status = TP_STATUS_KERNEL;
#else
                pktbuf_use_data_with_headroom(&packets[received], (void*)data, hdr->tp_snaplen, ethdev->rx.ring.frame_headroom);
                packets[received].pointer = (void*)&hdr->tp_status;
#endif
                received++;
#ifndef OPENWRT
            }
            else
            {
                hdr->tp_status = TP_STATUS_KERNEL;
            }
#endif
            ethdev->rx.ring.index = (ethdev->rx.ring.index + 1) & ethdev->rx.ring.index_mask;
        }
        else
        {
            break;
        }
    }

    return received;
}

static always_inline int32_t txq_enqueue(struct ethdev* ethdev, const struct pktbuf* packet)
{
    struct iovec* const frame_buffer = &ethdev->tx.buffer.frames[ethdev->tx.buffer.enqueued];

    assert(packet->data_size <= ethdev->tx.buffer.frame_size);

    if (ethdev->tx.buffer.enqueued < ethdev->tx.buffer.frame_nr)
    {
        memcpy(frame_buffer->iov_base, packet->data, packet->data_size);
        frame_buffer->iov_len = packet->data_size;

        ethdev->tx.buffer.enqueued++;

        return 1;
    }

    return 0;
}

static always_inline int32_t txq_enqueue_mmap(struct ethdev* ethdev, const struct pktbuf* packet)
{
    volatile struct tpacket2_hdr* const hdr = ethdev->tx.ring.frames[ethdev->tx.ring.index].iov_base;
    uint8_t* const data = (uint8_t*)hdr + TX_DATA_OFFSET;

    assert(packet->data_size <= ethdev->tx.ring.frame_size);

    if (hdr->tp_status == TP_STATUS_AVAILABLE)
    {
        memcpy(data, packet->data, packet->data_size);
        hdr->tp_len = packet->data_size;
        hdr->tp_status = TP_STATUS_SEND_REQUEST;

        ethdev->tx.ring.index = (ethdev->tx.ring.index + 1) & ethdev->tx.ring.index_mask;
        ethdev->tx.ring.enqueued++;

        return 1;
    }

    return 0;
}

static always_inline int32_t txq_clear(struct ethdev* ethdev)
{
    const uint16_t cleared = ethdev->tx.buffer.enqueued;
    ethdev->tx.buffer.enqueued = 0;

    return cleared;
}

static always_inline int32_t txq_clear_mmap(struct ethdev* ethdev)
{
    uint16_t index, cleared;
    volatile struct tpacket2_hdr* hdr;

    index = (ethdev->tx.ring.index - 1) & ethdev->rx.ring.index_mask;
    hdr = ethdev->tx.ring.frames[index].iov_base;
    cleared = 0;

    while (hdr->tp_status == TP_STATUS_SEND_REQUEST)
    {
        hdr->tp_status = TP_STATUS_AVAILABLE;

        index = (index - 1) & ethdev->rx.ring.index_mask;
        hdr = ethdev->tx.ring.frames[index].iov_base;
        cleared++;
    }

    ethdev->tx.ring.index = (index + 1) & ethdev->rx.ring.index_mask;
    ethdev->tx.ring.enqueued = 0;

    return cleared;
}

static always_inline int32_t txq_send(struct ethdev* ethdev)
{
    int sent;

    sent = sendmmsg(ethdev->socket_fd, ethdev->tx.buffer.messages, ethdev->tx.buffer.enqueued, 0);
    if (unlikely(sent == -1))
    {
        print_errno("sendmmsg");
        sent = 0;
    }

    ethdev->tx.buffer.enqueued = 0;

    return sent;
}

static always_inline int32_t txq_send_mmap(struct ethdev* ethdev)
{
    const uint16_t sent = ethdev->tx.ring.enqueued;
    ethdev->tx.ring.enqueued = 0;

    send(ethdev->socket_fd, NULL, 0, 0);

    return sent;
}

int ethdev_open(struct ethdev* ethdev, const struct ethdev_parameters* parameters)
{
    if (!IS_POWER_OF_TWO(parameters->rx_frame_nr) ||
        !IS_POWER_OF_TWO(parameters->tx_frame_nr) ||
        parameters->rx_frame_headroom + ETH_FRAME_SIZE_MAX > parameters->rx_frame_size)
    {
        errno = EINVAL;
        return -1;
    }

    memset(ethdev, 0, sizeof(struct ethdev));

    if (configure_interface(ethdev, parameters->if_name) == -1)
    {
        restore_interface(ethdev);
        goto error;
    }

    ethdev->socket_fd = init_socket(ethdev, parameters);
    if (ethdev->socket_fd == -1)
        goto error;

    return 0;

error:
    memset(ethdev, 0, sizeof(struct ethdev));
    ethdev->socket_fd = -1;

    return -1;
}

void ethdev_close(struct ethdev* ethdev)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    free_rings(ethdev);
#else
    free_buffers(ethdev);
#endif
    close(ethdev->socket_fd);

    restore_interface(ethdev);

    memset(ethdev, 0, sizeof(struct ethdev));
    ethdev->socket_fd = -1;
}

int32_t ethdev_rx_burst(struct ethdev* ethdev, struct pktbuf* packets, uint16_t packet_nr)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    return rx_burst_mmap(ethdev, packets, packet_nr);
#else
    return rx_burst(ethdev, packets, packet_nr);
#endif
}

int32_t ethdev_rx_burst_poll(struct ethdev* ethdev, struct pktbuf* packets, uint16_t packet_nr, int timeout)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    volatile const struct tpacket2_hdr* const hdr = ethdev->rx.ring.frames[ethdev->rx.ring.index].iov_base;
#endif
    struct pollfd pfd;
    int poll_result;

#ifndef ETHDEV_NO_PACKET_MMAP
    if (hdr->tp_status == TP_STATUS_KERNEL)
    {
#endif
        pfd.fd = ethdev->socket_fd;
        pfd.revents = 0;
        pfd.events = POLLIN | POLLERR;

        poll_result = poll(&pfd, 1, timeout);

        return poll_result == 1 ? ethdev_rx_burst(ethdev, packets, packet_nr) : poll_result;
#ifndef ETHDEV_NO_PACKET_MMAP
    }

    return ethdev_rx_burst(ethdev, packets, packet_nr);
#endif
}

int32_t ethdev_tx_burst(struct ethdev* ethdev, const struct pktbuf* packets, uint16_t packet_nr)
{
    uint16_t enqueued = 0;

    while (enqueued < packet_nr)
    {
#ifndef ETHDEV_NO_PACKET_MMAP
        if (txq_enqueue_mmap(ethdev, &packets[enqueued]))
#else
        if (txq_enqueue(ethdev, &packets[enqueued]))
#endif
            enqueued++;
        else
            return -enqueued;
    }
#ifndef ETHDEV_NO_PACKET_MMAP
    return txq_send_mmap(ethdev);
#else
    return txq_send(ethdev);
#endif
}

int32_t ethdev_tx_burst_poll(struct ethdev* ethdev, const struct pktbuf* packets, uint16_t packet_nr, int timeout)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    volatile const struct tpacket2_hdr* const hdr = ethdev->tx.ring.frames[ethdev->tx.ring.index].iov_base;
#endif
    struct pollfd pfd;
    int poll_result;

#ifndef ETHDEV_NO_PACKET_MMAP
    if (hdr->tp_status == TP_STATUS_KERNEL)
    {
#endif
        pfd.fd = ethdev->socket_fd;
        pfd.revents = 0;
        pfd.events = POLLOUT;

        poll_result = poll(&pfd, 1, timeout);

        return poll_result == 1 ? ethdev_tx_burst(ethdev, packets, packet_nr) : poll_result;
#ifndef ETHDEV_NO_PACKET_MMAP
    }

    return ethdev_tx_burst(ethdev, packets, packet_nr);
#endif
}

uint16_t ethdev_txq_length(const struct ethdev* ethdev)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    return ethdev->tx.ring.enqueued;
#else
    return ethdev->tx.buffer.enqueued;
#endif
}

int32_t ethdev_txq_enqueue(struct ethdev* ethdev, const struct pktbuf* packet)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    return txq_enqueue_mmap(ethdev, packet);
#else
    return txq_enqueue(ethdev, packet);
#endif
}

int32_t ethdev_txq_clear(struct ethdev* ethdev)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    return txq_clear_mmap(ethdev);
#else
    return txq_clear(ethdev);
#endif
}

int32_t ethdev_txq_send(struct ethdev* ethdev)
{
#ifndef ETHDEV_NO_PACKET_MMAP
    return txq_send_mmap(ethdev);
#else
    return txq_send(ethdev);
#endif
}
