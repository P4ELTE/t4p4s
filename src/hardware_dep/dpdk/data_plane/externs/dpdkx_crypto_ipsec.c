// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdkx_crypto.h"

#if T4P4S_INIT_CRYPTO

#include <time.h>
#include <stdlib.h>
#include <rte_dev.h>
#include <rte_bus_vdev.h>
#include <rte_errno.h>
#include <rte_ip.h>

#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
    #include <rte_cryptodev_scheduler.h>
    #include <dataplane.h>
#endif

extern void do_sync_crypto_operation(crypto_task_type_e task_type, int offset, SHORT_STDPARAMS);

// -----------------------------------------------------------------------------
// Implementation of P4 architecture externs
extern void do_async_crypto_operation(crypto_task_type_e task_type, int offset, int phase, SHORT_STDPARAMS);

const uint32_t ETH_LENGTH = 14;
const uint32_t IP_LENGTH = 20;

const uint32_t TOTAL_HMAC_LENGTH = 16;
const uint32_t KEPT_HMAC_LENGTH = 12;

void EXTERNIMPL0(ipsec_encapsulate)(SHORT_STDPARAMS) {
    uint8_t* wrapper_pointer = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);
#if defined ASYNC_MODE && ASYNC_MODE == ASYNC_MODE_PD
    switch(pd->program_restore_phase){
        case 0: break;
        case 1: goto PHASE1;
        case 2: goto PHASE2;
    }
#endif
    {
        int wrapper_size = rte_pktmbuf_pkt_len(pd->wrapper);
        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), wrapper_size, "Starting IPsec (" T4LIT(%dB) "): ", wrapper_size);


        // Tunnel mode: we encrypt everything after eth header
        const uint32_t length_to_encrypt_from_original = wrapper_size - ETH_LENGTH;
        const uint32_t payload_length = wrapper_size - ETH_LENGTH - IP_LENGTH;
        const uint32_t esp_trail_length = 2;
        const uint32_t padding_size = (16 - (length_to_encrypt_from_original + esp_trail_length) % 16) % 16;
        const uint32_t to_encrypt_size = length_to_encrypt_from_original + padding_size + esp_trail_length;

        const uint32_t esp_head_size = 8;
        const uint32_t iv_size = 16;


        debug("   :: wrapper_size: %d\n",wrapper_size);
        debug("   :: esp_head_size: %d\n",esp_head_size);
        debug("   :: iv_size: %d\n",iv_size);
        debug("   :: to_encrypt_size: %d\n",to_encrypt_size);
        debug("   :: payload_length: %d\n",payload_length);
        debug("   :: esp_head_size + iv_size + to_encrypt_size - payload_length: %d\n",esp_head_size + iv_size + to_encrypt_size - payload_length);

        rte_pktmbuf_append(pd->wrapper, esp_head_size + iv_size + to_encrypt_size - payload_length);
        debug("   :: Added " T4LIT(%dB) " of " T4LIT(padding,field) ", " T4LIT(1B) " of " T4LIT(pad length size,field) ", and " T4LIT(1B) " of " T4LIT(next header size,field) " for a total of " T4LIT(%dB) "\n", padding_size, to_encrypt_size);


        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "Actual wrapper (" T4LIT(%dB) "): ", rte_pktmbuf_pkt_len(pd->wrapper));
        debug("   :: Copy " T4LIT(%dB) " data from wrapper \n", to_encrypt_size);
        // Important to use memmove, because the source and destination can overlap
        memmove(wrapper_pointer + ETH_LENGTH + IP_LENGTH + esp_head_size + iv_size, wrapper_pointer + ETH_LENGTH, to_encrypt_size);
        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "Actual wrapper (" T4LIT(%dB) "): ", rte_pktmbuf_pkt_len(pd->wrapper));

        uint8_t* data_pointer = wrapper_pointer + ETH_LENGTH + IP_LENGTH;
        // ESP (SPI + Sequence number)
        *data_pointer++ = 0;
        *data_pointer++ = 0;
        *data_pointer++ = 0x02;
        *data_pointer++ = 0x22;

        // Sequence number
        *data_pointer++ = 0;
        *data_pointer++ = 0;
        *data_pointer++ = 0;
        *data_pointer++ = 1;

        //IV
        for(int a=0;a<16;a++){
            *data_pointer++ = 0x00;
        }
        // Jump over the payload
        data_pointer += length_to_encrypt_from_original;
        // And set the padding bytes
        uint8_t padding_value = 1;
        for(int padding_iterator = 0; padding_iterator < padding_size; padding_iterator++){
            *data_pointer = padding_value;
            data_pointer++;
            padding_value++;
        }
        //ESP TRAIL
        *data_pointer++ = (uint8_t)padding_size; // Pad length
        *data_pointer++ = 4; // Set next header to ipv4

        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "Actual wrapper (" T4LIT(%dB) "): ", rte_pktmbuf_pkt_len(pd->wrapper));

        do_async_crypto_operation(CRYPTO_TASK_ENCRYPT, ETH_LENGTH + IP_LENGTH + esp_head_size + iv_size, 0, SHORT_STDPARAMS_IN);
    }
PHASE1:
    {
        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "Actual wrapper (" T4LIT(%dB) "): ", rte_pktmbuf_pkt_len(pd->wrapper));
        debug("offset for hmac: %d\n", ETH_LENGTH + IP_LENGTH)
        do_async_crypto_operation(CRYPTO_TASK_MD5_HMAC, ETH_LENGTH + IP_LENGTH, 1 , SHORT_STDPARAMS_IN);
    }
PHASE2:
    {
        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "Actual wrapper (" T4LIT(%dB) "): ", rte_pktmbuf_pkt_len(pd->wrapper));

        // We keep only 12 bytes from 16 byte HMAC
        rte_pktmbuf_trim(pd->wrapper, TOTAL_HMAC_LENGTH - KEPT_HMAC_LENGTH);

        pd->payload_size = rte_pktmbuf_pkt_len(pd->wrapper);
        //*((uint16_t*)(wrapper_pointer + ETH_LENGTH + 3)) = (uint16_t )pd->payload_size - ETH_LENGTH;
        // TODO: use proper endian
        *(wrapper_pointer + ETH_LENGTH + 3) = (uint8_t )pd->payload_size - ETH_LENGTH;

        int HMAC_offset = rte_pktmbuf_pkt_len(pd->wrapper) - KEPT_HMAC_LENGTH;
        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "   :: IPsec done, keeping " T4LIT(%dB) " of " T4LIT(%dB) " " T4LIT(HMAC,field) " at offset " T4LIT(%d) ": ", KEPT_HMAC_LENGTH, TOTAL_HMAC_LENGTH, HMAC_offset);
        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*) + HMAC_offset, KEPT_HMAC_LENGTH, "   :: IPsec done, keeping " T4LIT( % dB) " of " T4LIT(%dB) " " T4LIT(HMAC,field) " at offset " T4LIT(%d) ": ", KEPT_HMAC_LENGTH, TOTAL_HMAC_LENGTH, HMAC_offset);

        uint16_t* cksum_pointer = (uint16_t*)(wrapper_pointer + ETH_LENGTH + 10);
        *(cksum_pointer) = 0;

        uint32_t* protocol_pointer = (uint32_t*)(wrapper_pointer + ETH_LENGTH + 9);
        *protocol_pointer = 0x32;

        uint32_t* ip_src_pointer = (uint32_t*)(wrapper_pointer + ETH_LENGTH + 12);
        *ip_src_pointer = 0x03030303;
        uint32_t* ip_dst_pointer = (uint32_t*)(wrapper_pointer + ETH_LENGTH + 16);
        *ip_dst_pointer = 0x04040404;

        dbg_bytes(wrapper_pointer + ETH_LENGTH, IP_LENGTH , "   :: Ipv4 checksum calculation input: ");
        uint16_t calculated_cksum = rte_raw_cksum(wrapper_pointer + ETH_LENGTH, IP_LENGTH);
        calculated_cksum = (calculated_cksum == 0xffff) ? calculated_cksum : ((~calculated_cksum) & 0xffff);
        *(cksum_pointer) = calculated_cksum;

        dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "   :: IPsec done, final result (" T4LIT(%dB) "):: ", rte_pktmbuf_pkt_len(pd->wrapper));
    }
}

#endif
