// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Eotvos Lorand University, Budapest, Hungary

#include "dpdkx_crypto.h"

#if T4P4S_INIT_CRYPTO

#include <time.h>
#include <stdlib.h>
#include <rte_dev.h>
#include <rte_bus_vdev.h>
#include <rte_errno.h>

#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
    #include <rte_cryptodev_scheduler.h>
    #include <dataplane.h>
#endif

extern void do_blocking_sync_op(crypto_task_type_e op, int offset, SHORT_STDPARAMS);

// -----------------------------------------------------------------------------
// Implementation of P4 architecture externs

void ipsec_prepare_encrypt_msg(int headers_size, int pad_length_size, int next_header_size, int esp_size, int iv_size, int wrapper_size, int original_payload_size, int padding_size, int to_encrypt_size, SHORT_STDPARAMS) {
    rte_pktmbuf_append(pd->wrapper,esp_size + iv_size + to_encrypt_size - original_payload_size);
    uint8_t* data = rte_pktmbuf_mtod(pd->wrapper, uint8_t*);

    uint8_t* data_pointer = data;
    // Pass the existing header and leave space for the ESP and IV in payload (we forget about the original payload)
    data_pointer += headers_size + esp_size + iv_size;
    // Important to use memmove, because the source and destination can overlap
    memmove(data_pointer, data, wrapper_size);

    // Run to the end of the copied header+payload
    data_pointer += wrapper_size;
    // And set the padding bytes
    memset(data_pointer, 0xef, padding_size);

    // Save the size of padding
    data_pointer += padding_size;
    memset(data_pointer, (uint8_t)padding_size, 1);

    // Set next header to ipv4
    data_pointer += 1;
    memset(data_pointer, 4, 1);

    // TODO: add ESP
    // ESP
    memset(data + headers_size, 0x1a, 8);
    // TODO: use random IV
    // IV
    memset(data + headers_size + esp_size, 0x1b, 8);
}

void ipsec_adjust_result(int total_HMAC_length, int kept_HMAC_length, SHORT_STDPARAMS) {
    // We keep only 12 bytes from 16 byte HMAC
    rte_pktmbuf_trim(pd->wrapper, total_HMAC_length - kept_HMAC_length);

    pd->payload_size = rte_pktmbuf_pkt_len(pd->wrapper);
    // TODO properly update pd so that it uses the new packet
}

void EXTERNIMPL0(ipsec_encapsulate)(SHORT_STDPARAMS) {
    int headers_size = 34;
    int pad_length_size = 1;
    int next_header_size = 1;
    int esp_size = 8;
    int iv_size = 8;

    int total_HMAC_length = 16;
    int kept_HMAC_length = 12;

    int wrapper_size = rte_pktmbuf_pkt_len(pd->wrapper);
    int original_payload_size = wrapper_size - headers_size;
    int padding_size = (16 - (wrapper_size + pad_length_size + next_header_size)% 16) % 16;
    int to_encrypt_size = padding_size + wrapper_size + pad_length_size + next_header_size;

    dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), wrapper_size, "Starting IPsec (" T4LIT(%dB) "): ", wrapper_size);
    debug("   :: Added " T4LIT(%dB) " of " T4LIT(padding,field) ", " T4LIT(1B) " of " T4LIT(pad length size,field) ", and " T4LIT(1B) " of " T4LIT(next header size,field) " for a total of " T4LIT(%dB) "\n", padding_size, to_encrypt_size);

    ipsec_prepare_encrypt_msg(headers_size, pad_length_size, next_header_size, esp_size, iv_size, wrapper_size, original_payload_size, padding_size, to_encrypt_size, SHORT_STDPARAMS_IN);

    do_blocking_sync_op(CRYPTO_TASK_ENCRYPT, headers_size + esp_size + iv_size, SHORT_STDPARAMS_IN);
    do_blocking_sync_op(CRYPTO_TASK_MD5_HMAC, headers_size, SHORT_STDPARAMS_IN);

    ipsec_adjust_result(total_HMAC_length, kept_HMAC_length, SHORT_STDPARAMS_IN);

    int HMAC_offset = rte_pktmbuf_pkt_len(pd->wrapper) - kept_HMAC_length;
    dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*), rte_pktmbuf_pkt_len(pd->wrapper), "   :: IPsec done, keeping " T4LIT(%dB) " of " T4LIT(%dB) " " T4LIT(HMAC,field) " at offset " T4LIT(%d) ": ", kept_HMAC_length, total_HMAC_length, HMAC_offset);
    dbg_bytes(rte_pktmbuf_mtod(pd->wrapper, uint8_t*) + HMAC_offset, kept_HMAC_length, "   :: IPsec done, keeping " T4LIT(%dB) " of " T4LIT(%dB) " " T4LIT(HMAC,field) " at offset " T4LIT(%d) ": ", kept_HMAC_length, total_HMAC_length, HMAC_offset);
}

#endif
