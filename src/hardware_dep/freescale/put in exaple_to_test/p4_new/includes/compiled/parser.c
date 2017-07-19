 #include "includes/freescale/freescale_lib.h"// sugar@59
 #include "actions.h" // apply_table_* and action_code_*// sugar@60

 void print_mac(uint8_t* v) { printf("%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX\n", v[0], v[1], v[2], v[3], v[4], v[5]); }// sugar@62
 void print_ip(uint8_t* v) { printf("%d.%d.%d.%d\n",v[0],v[1],v[2],v[3]); }// sugar@63
 
 static void// sugar@65
 extract_header(uint8_t* buf, packet_descriptor_t* pd, header_instance_t h) {// sugar@66
     pd->headers[h] =// sugar@67
       (header_descriptor_t) {// sugar@68
         .type = h,// sugar@69
         .pointer = buf,// sugar@70
         .length = header_instance_byte_width[h],// sugar@71
         .var_width_field_bitwidth = 0// sugar@72
       };// sugar@73
 }// sugar@74
 
 static inline void p4_pe_header_too_short(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static inline void p4_pe_default(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static inline void p4_pe_checksum(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static inline void p4_pe_unhandled_select(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static inline void p4_pe_index_out_of_bounds(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static inline void p4_pe_header_too_long(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static inline void p4_pe_out_of_packet(packet_descriptor_t *pd) {// sugar@78
 drop(pd);// sugar@80
 }// sugar@83
 static void parse_state_start(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables);// sugar@86
 static void parse_state_parse_ethernet(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables);// sugar@86

 static void parse_state_start(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables)// sugar@111
 {// sugar@112
     uint32_t value32;// sugar@113
     (void)value32;// sugar@114
  return parse_state_parse_ethernet(pd, buf, tables);// sugar@8
// sugar@147
 }// sugar@182
 
 static void parse_state_parse_ethernet(packet_descriptor_t* pd, uint8_t* buf, lookup_table_t** tables)// sugar@111
 {// sugar@112
     uint32_t value32;// sugar@113
     (void)value32;// sugar@114
     extract_header(buf, pd, header_instance_ethernet);// sugar@119
     buf += header_instance_byte_width[header_instance_ethernet];// sugar@125
 return apply_table_smac(pd, tables);// sugar@147
 }// sugar@182
 
 void parse_packet(packet_descriptor_t* pd, lookup_table_t** tables) {// sugar@185
     parse_state_start(pd, pd->data, tables);// sugar@186
 }// sugar@187
