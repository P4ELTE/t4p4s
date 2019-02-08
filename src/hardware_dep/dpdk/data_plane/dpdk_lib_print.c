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


// This file is included directly from `dpdk_lib.c`.


void print_port_link_status(struct rte_eth_link* link, unsigned portid) {
    if (link->link_status) {
        debug("   :: Port %d Link Up - speed %u Mbps - %s\n",
               (uint8_t)portid, (unsigned)link->link_speed,
               (link->link_duplex == ETH_LINK_FULL_DUPLEX) ?
                    ("full-duplex") : ("half-duplex\n"));
    } else {
        debug("   :: Port %d Link Down\n", (uint8_t)portid);
    }
}

// Returns 1 if all ports are up, otherwise 0.
int print_all_ports(uint8_t port_num, uint32_t port_mask, uint8_t print_flag) {
    for (uint8_t portid = 0; portid < port_num; portid++) {
        if ((port_mask & (1 << portid)) == 0)
            continue;

        struct rte_eth_link link;
        memset(&link, 0, sizeof(link));

        rte_eth_link_get_nowait(portid, &link);
        /* print link status if flag set */
        if (print_flag == 1) {
            print_port_link_status(&link, portid);
            continue;
        }
        /* clear all_ports_up flag if any link down */
        if (link.link_status == 0) {
            return 0;
        }
    }

    return 1;
}

// Check the link status of all ports in up to 9s, and print them finally
void print_all_ports_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
    uint8_t print_flag = 0;

    debug(" :::: Checking link status\n");
    fflush(stdout);
    for (uint8_t count = 0; count <= MAX_CHECK_TIME; count++) {
        int all_ports_up = print_all_ports(port_num, port_mask, print_flag);

        /* after finally printing all link status, get out */
        if (print_flag == 1)
            break;

        if (all_ports_up == 0) {
            // debug(".");
            fflush(stdout);
            rte_delay_ms(CHECK_INTERVAL);
        }

        /* set the print_flag if all ports up or timeout */
        if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
            print_flag = 1;
            debug("   :: Link status OK\n");
        }
    }
}

void print_port_mac(unsigned portid, uint8_t* mac_bytes) {
    debug(" :::: Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
           (unsigned) portid,
           mac_bytes[0], mac_bytes[1], mac_bytes[2], mac_bytes[3], mac_bytes[4], mac_bytes[5]);
}
