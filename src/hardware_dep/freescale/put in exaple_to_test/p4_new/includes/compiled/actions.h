 #ifndef __ACTION_INFO_GENERATED_H__// sugar@1
 #define __ACTION_INFO_GENERATED_H__// sugar@2
 

 #define FIELD(name, length) uint8_t name[(length + 7) / 8];// sugar@5

 enum actions {// sugar@8
 action_mac_learn,// sugar@14
 action__nop,// sugar@14
 action_forward,// sugar@14
 action_bcast,// sugar@14
 };// sugar@15
 struct action_forward_params {// sugar@20
 FIELD(port, 9);// sugar@22
 };// sugar@23
 struct smac_action {// sugar@26
     int action_id;// sugar@27
     union {// sugar@28
     };// sugar@32
 };// sugar@33
 struct dmac_action {// sugar@26
     int action_id;// sugar@27
     union {// sugar@28
 struct action_forward_params forward_params;// sugar@31
     };// sugar@32
 };// sugar@33
 void apply_table_smac(packet_descriptor_t *pd, lookup_table_t** tables);// sugar@36
 void action_code_mac_learn(packet_descriptor_t *pd, lookup_table_t **tables);// sugar@41
 void action_code__nop(packet_descriptor_t *pd, lookup_table_t **tables);// sugar@41
 void apply_table_dmac(packet_descriptor_t *pd, lookup_table_t** tables);// sugar@36
 void action_code_forward(packet_descriptor_t *pd, lookup_table_t **tables, struct action_forward_params);// sugar@39
 void action_code_bcast(packet_descriptor_t *pd, lookup_table_t **tables);// sugar@41
 #endif// sugar@43
