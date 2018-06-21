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

#include "backend.h"
#include "config.h"
#include "ethdev.h"
#include "pktbuf.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

lookup_table_t* tables[NB_TABLES];
counter_t* counters[NB_COUNTERS];
p4_register_t* registers[NB_REGISTERS];

struct ethdev* ports;
uint8_t port_count;

extern void main_loop(void);

static void create_table(lookup_table_t* t, unused int socketid)
{
    t->socketid = -1;

    if (t->key_size == 0)
        return;

    switch (t->type)
    {
    case LOOKUP_EXACT:
        exact_create(t, -1);
        break;
    case LOOKUP_LPM:
        lpm_create(t, -1);
        break;
    case LOOKUP_TERNARY:
        ternary_create(t, -1);
        break;
    default:
        debug(" :::: invalid table type: %u\n", t->type);
        break;
    }
}

static void create_tables(void)
{
    unsigned int i;
    
    if (table_config == NULL || NB_TABLES == 0)
        return;

    debug("Initializing tables...\n");
    
    for (i = 0; i < NB_TABLES; ++i)
    {
        lookup_table_t* const t = &table_config[i];
        tables[i] = t;

        debug("  :::: initializing table %s...\n", t->name);
        
        create_table(t, -1);
    }
}

static void init_counter(void* data)
{
    *(uint32_t*)data = 0;
}

static void create_counters(void)
{
    unsigned int i;

    if (counter_config == NULL || NB_COUNTERS == 0)
        return;

    debug("Initializing counters...\n");

    for (i = 0; i < NB_COUNTERS; ++i)
    {
        counter_t* const c = &counter_config[i];
        counters[i] = c;

        debug("  :::: initializing counter %s...\n", c->name);

        c->values = malloc(sizeof(vector_t));
        vector_init(c->values, 1, c->size, sizeof(uint32_t), &init_counter, -1);
    }
}

static void create_registers(void)
{
    unsigned int i;

    if (register_config == NULL || NB_REGISTERS == 0)
        return;

    debug("Initializing registers...\n");

    for (i = 0; i < NB_REGISTERS; ++i)
    {
        p4_register_t* const r = &register_config[i];
        registers[i] = r;

        debug("  :::: initializing register %s...\n", r->name);
        
        r->values = malloc(r->width * r->size);
    }
}

uint8_t initialize(int argc, char** argv)
{
    unsigned int i;
    
    struct ethdev_parameters params =
    {
        .if_name = NULL,
        .rx_frame_size = RX_FRAME_SIZE,
        .rx_frame_nr = RX_QUEUE_LENGTH,
        .rx_frame_headroom = RX_FRAME_HEADROOM,
        .tx_frame_size = TX_FRAME_SIZE,
        .tx_frame_nr = TX_QUEUE_LENGTH
    };

    if (argc < 2)
        return 0;

    create_tables();
    create_counters();
    create_registers();

    ports = malloc((argc - 1) * sizeof(*ports));
    port_count = 0;
    
    debug("Initializing ports...\n");

    for (i = 1; i < argc; ++i)
    {
        params.if_name = argv[i];

        if (ethdev_open(&ports[port_count], &params) == 0)
        {
            debug("  :::: created port %u (%s)\n", port_count, params.if_name);
            port_count++;            
        }
        else
        {
            debug("  :::: failed to create port %u (%s)\n", port_count, params.if_name);
        }
    }

    if (port_count < (argc - 1))
        ports = realloc(ports, port_count * sizeof(*ports));

    return port_count;
}

void uninitialize(void)
{
    uint8_t port;

    for (port = 0; port < port_count; ++port)
    {
        ethdev_close(&ports[port]);
    }

    free(ports);
}

int launch(void)
{
    if (port_count == 0)
    {
        printf("No ports were configured. Exiting...\n");
        return -1;
    }

    main_loop();

    return 0;
}

void exact_add_promote(int id, uint8_t* key, uint8_t* value)
{
    exact_add(tables[id], key, value);
}

void lpm_add_promote(int id, uint8_t* key, uint8_t depth, uint8_t* value)
{
    lpm_add(tables[id], key, depth, value);
}

void ternary_add_promote(int id, uint8_t* key, uint8_t* mask, uint8_t* value)
{
    ternary_add(tables[id], key, mask, value);
}

void table_setdefault_promote(int id, uint8_t* value)
{
    table_setdefault(tables[id], value);
}

uint16_t calculate_csum16(const void* buf, uint16_t length)
{
    const uint16_t* data = buf;
    uint32_t csum = 0;

    assert((uintptr_t)data % sizeof(uint16_t) == 0);

    while (length >= (4 * sizeof(uint16_t)))
    {
        csum += data[0];
        csum += data[1];
        csum += data[2];
        csum += data[3];
        data += 4;
        length -= 4 * sizeof(uint16_t);
    }

    if (length & (2 * sizeof(uint16_t)))
    {
        csum += data[0];
        csum += data[1];
        data += 2;
        length -= 2 * sizeof(uint16_t);
    }

    if (length & sizeof(uint16_t))
    {
        csum += *data++;
        length -= sizeof(uint16_t);
    }

    if (length & sizeof(uint8_t))
        csum += *(const uint8_t*)data;

    csum = (csum & 0xffff) + (csum >> 16);
    csum = (csum & 0xffff) + (csum >> 16);

    return (uint16_t)~csum;
}

uint32_t packet_length(packet_descriptor_t* pd)
{
    return (uint32_t)pd->wrapper->data_size;
}
