 #include <stdlib.h>// sugar@4
 #include <string.h>// sugar@5
 #include "includes/freescale/freescale_lib.h"// sugar@6
 #include "actions.h"// sugar@7
 
 extern void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables);// sugar@9

 extern void increase_counter (int counterid, int index);// sugar@11

 void apply_table_smac(packet_descriptor_t* pd, lookup_table_t** tables);// sugar@14
 void apply_table_dmac(packet_descriptor_t* pd, lookup_table_t** tables);// sugar@14

 uint8_t reverse_buffer[6];// sugar@18
 void table_smac_key(packet_descriptor_t* pd, uint8_t* key) {// sugar@27
 EXTRACT_BYTEBUF(pd, field_instance_ethernet_srcAddr, key)// sugar@35
 key += 6;// sugar@36
 }// sugar@44

 void table_dmac_key(packet_descriptor_t* pd, uint8_t* key) {// sugar@27
 EXTRACT_BYTEBUF(pd, field_instance_ethernet_dstAddr, key)// sugar@35
 key += 6;// sugar@36
 }// sugar@44

 void apply_table_smac(packet_descriptor_t* pd, lookup_table_t** tables)// sugar@50
 {// sugar@51
     debug("  :::: EXECUTING TABLE smac\n");// sugar@52
     uint8_t* key[6];// sugar@53
     table_smac_key(pd, (uint8_t*)key);// sugar@54
     uint8_t* value = exact_lookup(tables[TABLE_smac], (uint8_t*)key);// sugar@55
     if(value == NULL) {// sugar@60
              debug("    :: NO RESULT, NO DEFAULT ACTION, IGNORING PACKET.\n");// sugar@61
              return;// sugar@62
     }// sugar@63
     int index = *(int*)(value+sizeof(struct smac_action)); (void)index;// sugar@56
     struct smac_action* res = (struct smac_action*)value;// sugar@57
     if(res == NULL) {// sugar@60
         debug("    :: NO RESULT, NO DEFAULT ACTION, IGNORING PACKET.\n");// sugar@61
         return;// sugar@62
     }// sugar@63
     switch (res->action_id) {// sugar@64
 case action_mac_learn:// sugar@66
   debug("    :: EXECUTING ACTION mac_learn...\n");// sugar@67
 action_code_mac_learn(pd, tables);// sugar@71
     break;// sugar@72
 case action__nop:// sugar@66
   debug("    :: EXECUTING ACTION _nop...\n");// sugar@67
 action_code__nop(pd, tables);// sugar@71
     break;// sugar@72
     }// sugar@73
 switch (res->action_id) {// sugar@82
 case action_mac_learn:// sugar@84
     return apply_table_dmac(pd, tables);// sugar@85
     break;// sugar@86
 case action__nop:// sugar@84
     return apply_table_dmac(pd, tables);// sugar@85
     break;// sugar@86
 }// sugar@87
 }// sugar@88

 void apply_table_dmac(packet_descriptor_t* pd, lookup_table_t** tables)// sugar@50
 {// sugar@51
     debug("  :::: EXECUTING TABLE dmac\n");// sugar@52
     uint8_t* key[6];// sugar@53
     table_dmac_key(pd, (uint8_t*)key);// sugar@54
     uint8_t* value = exact_lookup(tables[TABLE_dmac], (uint8_t*)key);// sugar@55
     int index = *(int*)(value+sizeof(struct dmac_action)); (void)index;// sugar@56
     struct dmac_action* res = (struct dmac_action*)value;// sugar@57
     if(res == NULL) {// sugar@60
         debug("    :: NO RESULT, NO DEFAULT ACTION, IGNORING PACKET.\n");// sugar@61
         return;// sugar@62
     }// sugar@63
     switch (res->action_id) {// sugar@64
 case action_forward:// sugar@66
   debug("    :: EXECUTING ACTION forward...\n");// sugar@67
 action_code_forward(pd, tables, res->forward_params);// sugar@69
     break;// sugar@72
 case action_bcast:// sugar@66
   debug("    :: EXECUTING ACTION bcast...\n");// sugar@67
 action_code_bcast(pd, tables);// sugar@71
     break;// sugar@72
     }// sugar@73
 switch (res->action_id) {// sugar@82
 case action_forward:// sugar@84
     // sugar@85
     break;// sugar@86
 case action_bcast:// sugar@84
     // sugar@85
     break;// sugar@86
 }// sugar@87
 }// sugar@88

 void init_headers(packet_descriptor_t* packet_desc) {// sugar@91
 packet_desc->headers[header_instance_standard_metadata] = (header_descriptor_t) { .type = header_instance_standard_metadata, .length = header_instance_byte_width[header_instance_standard_metadata],// sugar@95
                               .pointer = calloc(header_instance_byte_width[header_instance_standard_metadata], sizeof(uint8_t)) };// sugar@96
 packet_desc->headers[header_instance_ethernet] = (header_descriptor_t) { .type = header_instance_ethernet, .length = header_instance_byte_width[header_instance_ethernet], .pointer = NULL };// sugar@98
 }// sugar@99


 void init_keyless_tables() {// sugar@107
 }// sugar@115

 void init_dataplane(packet_descriptor_t* pd, lookup_table_t** tables) {// sugar@117
     init_headers(pd);// sugar@118
     init_keyless_tables();// sugar@119
 }// sugar@120

 void update_packet(packet_descriptor_t* pd) {// sugar@123
     uint32_t value32, res32;// sugar@124
     (void)value32, (void)res32;// sugar@125
 }// sugar@131

 
 void handle_packet(packet_descriptor_t* pd, lookup_table_t** tables)// sugar@135
 {// sugar@136
     int value32;// sugar@137
     EXTRACT_INT32_BITS(pd, field_instance_standard_metadata_ingress_port, value32)// sugar@138
     debug("### HANDLING PACKET ARRIVING AT PORT %" PRIu32 "...\n", value32);// sugar@139
     parse_packet(pd, tables);// sugar@140
     update_packet(pd);// sugar@141
 }// sugar@142
