 #include "includes/freescale/freescale_lib.h"// sugar@7
 #include "actions.h"// sugar@8
 #include <unistd.h>// sugar@9
 #include <arpa/inet.h>// sugar@10

 //extern backend bg;// sugar@12

  void action_code_no_op(packet_descriptor_t* pd, lookup_table_t** tables ) {// sugar@315
     uint32_t value32, res32;// sugar@316
     (void)value32; (void)res32;// sugar@317
 }// sugar@327

  void action_code_drop(packet_descriptor_t* pd, lookup_table_t** tables ) {// sugar@315
     uint32_t value32, res32;// sugar@316
     (void)value32; (void)res32;// sugar@317
 }// sugar@327

  void action_code__nop(packet_descriptor_t* pd, lookup_table_t** tables ) {// sugar@315
     uint32_t value32, res32;// sugar@316
     (void)value32; (void)res32;// sugar@317
 }// sugar@327

  void action_code_mac_learn(packet_descriptor_t* pd, lookup_table_t** tables ) {// sugar@315
     uint32_t value32, res32;// sugar@316
     (void)value32; (void)res32;// sugar@317
   struct type_field_list fields;// sugar@263
    fields.fields_quantity = 2;// sugar@265
    fields.field_offsets = malloc(sizeof(uint8_t*)*fields.fields_quantity);// sugar@266
    fields.field_widths = malloc(sizeof(uint8_t*)*fields.fields_quantity);// sugar@267
    fields.field_offsets[0] = (uint8_t*) (pd->headers[header_instance_ethernet].pointer + field_instance_byte_offset_hdr[field_instance_ethernet_srcAddr]);// sugar@271
    fields.field_widths[0] = field_instance_bit_width[field_instance_ethernet_srcAddr]*8;// sugar@272
    fields.field_offsets[1] = (uint8_t*) (pd->headers[header_instance_standard_metadata].pointer + field_instance_byte_offset_hdr[field_instance_standard_metadata_ingress_port]);// sugar@271
    fields.field_widths[1] = field_instance_bit_width[field_instance_standard_metadata_ingress_port]*8;// sugar@272

    //generate_digest(bg,"mac_learn_digest",0,&fields); sleep(1);// sugar@278
// sugar@323
 }// sugar@327

  void action_code_forward(packet_descriptor_t* pd, lookup_table_t** tables , struct action_forward_params parameters) {// sugar@315
     uint32_t value32, res32;// sugar@316
     (void)value32; (void)res32;// sugar@317
  MODIFY_INT32_BYTEBUF(pd, field_instance_standard_metadata_egress_port, parameters.port, 2)// sugar@103
// sugar@323
 }// sugar@327

  void action_code_bcast(packet_descriptor_t* pd, lookup_table_t** tables ) {// sugar@315
     uint32_t value32, res32;// sugar@316
     (void)value32; (void)res32;// sugar@317
  value32 = 100;// sugar@76
  MODIFY_INT32_INT32_AUTO(pd, field_instance_standard_metadata_egress_port, value32)// sugar@27
// sugar@77
// sugar@323
 }// sugar@327
