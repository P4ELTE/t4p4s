 #include "includes/freescale/freescale_lib.h"// sugar@9
 #include "actions.h"// sugar@10
 
 extern void table_setdefault_promote  (int tableid, uint8_t* value);// sugar@12
 extern void exact_add_promote  (int tableid, uint8_t* key, uint8_t* value);// sugar@13
 extern void lpm_add_promote    (int tableid, uint8_t* key, uint8_t depth, uint8_t* value);// sugar@14
 extern void ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value);// sugar@15

 extern void table_smac_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c// sugar@18
 extern void table_dmac_key(packet_descriptor_t* pd, uint8_t* key); // defined in dataplane.c// sugar@18

 uint8_t reverse_buffer[6];// sugar@22
 void// sugar@26
 smac_add(// sugar@27
 uint8_t field_instance_ethernet_srcAddr[6],// sugar@30
 struct smac_action action)// sugar@36
 {// sugar@37
     uint8_t key[6];// sugar@38
 memcpy(key+0, field_instance_ethernet_srcAddr, 6);// sugar@43
 exact_add_promote(TABLE_smac, (uint8_t*)key, (uint8_t*)&action);// sugar@60
 }// sugar@61

 void// sugar@63
 smac_setdefault(struct smac_action action)// sugar@64
 {// sugar@65
     table_setdefault_promote(TABLE_smac, (uint8_t*)&action);// sugar@66
 }// sugar@67
 void// sugar@26
 dmac_add(// sugar@27
 uint8_t field_instance_ethernet_dstAddr[6],// sugar@30
 struct dmac_action action)// sugar@36
 {// sugar@37
     uint8_t key[6];// sugar@38
 memcpy(key+0, field_instance_ethernet_dstAddr, 6);// sugar@43
 exact_add_promote(TABLE_dmac, (uint8_t*)key, (uint8_t*)&action);// sugar@60
 }// sugar@61

 void// sugar@63
 dmac_setdefault(struct dmac_action action)// sugar@64
 {// sugar@65
     table_setdefault_promote(TABLE_dmac, (uint8_t*)&action);// sugar@66
 }// sugar@67
 void// sugar@71
 smac_add_table_entry(struct p4_ctrl_msg* ctrl_m) {// sugar@72
 uint8_t* field_instance_ethernet_srcAddr = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[0])->bitmap);// sugar@76
 if(strcmp("mac_learn", ctrl_m->action_name)==0) {// sugar@81
     struct smac_action action;// sugar@82
     action.action_id = action_mac_learn;// sugar@83
     debug("Reply from the control plane arrived.\n");// sugar@87
     debug("Adding new entry to smac with action mac_learn\n");// sugar@88
     smac_add(// sugar@89
 field_instance_ethernet_srcAddr,// sugar@92
     action);// sugar@95
 } else// sugar@96
 if(strcmp("_nop", ctrl_m->action_name)==0) {// sugar@81
     struct smac_action action;// sugar@82
     action.action_id = action__nop;// sugar@83
     debug("Reply from the control plane arrived.\n");// sugar@87
     debug("Adding new entry to smac with action _nop\n");// sugar@88
     smac_add(// sugar@89
 field_instance_ethernet_srcAddr,// sugar@92
     action);// sugar@95
 } else// sugar@96
 debug("Table add entry: action name mismatch (%s).\n", ctrl_m->action_name);// sugar@97
 }// sugar@98
 void// sugar@71
 dmac_add_table_entry(struct p4_ctrl_msg* ctrl_m) {// sugar@72
 uint8_t* field_instance_ethernet_dstAddr = (uint8_t*)(((struct p4_field_match_exact*)ctrl_m->field_matches[0])->bitmap);// sugar@76
 if(strcmp("forward", ctrl_m->action_name)==0) {// sugar@81
     struct dmac_action action;// sugar@82
     action.action_id = action_forward;// sugar@83
 uint8_t* port = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[0])->bitmap;// sugar@85
 memcpy(action.forward_params.port, port, 2);// sugar@86
     debug("Reply from the control plane arrived.\n");// sugar@87
     debug("Adding new entry to dmac with action forward\n");// sugar@88
     dmac_add(// sugar@89
 field_instance_ethernet_dstAddr,// sugar@92
     action);// sugar@95
 } else// sugar@96
 if(strcmp("bcast", ctrl_m->action_name)==0) {// sugar@81
     struct dmac_action action;// sugar@82
     action.action_id = action_bcast;// sugar@83
     debug("Reply from the control plane arrived.\n");// sugar@87
     debug("Adding new entry to dmac with action bcast\n");// sugar@88
     dmac_add(// sugar@89
 field_instance_ethernet_dstAddr,// sugar@92
     action);// sugar@95
 } else// sugar@96
 debug("Table add entry: action name mismatch (%s).\n", ctrl_m->action_name);// sugar@97
 }// sugar@98
 void// sugar@101
 smac_set_default_table_action(struct p4_ctrl_msg* ctrl_m) {// sugar@102
 debug("Action name: %s\n", ctrl_m->action_name);// sugar@103
 if(strcmp("mac_learn", ctrl_m->action_name)==0) {// sugar@105
     struct smac_action action;// sugar@106
     action.action_id = action_mac_learn;// sugar@107
     debug("Message from the control plane arrived.\n");// sugar@111
     debug("Set default action for smac with action mac_learn\n");// sugar@112
     smac_setdefault( action );// sugar@113
 } else// sugar@114
 if(strcmp("_nop", ctrl_m->action_name)==0) {// sugar@105
     struct smac_action action;// sugar@106
     action.action_id = action__nop;// sugar@107
     debug("Message from the control plane arrived.\n");// sugar@111
     debug("Set default action for smac with action _nop\n");// sugar@112
     smac_setdefault( action );// sugar@113
 } else// sugar@114
 debug("Table setdefault: action name mismatch (%s).\n", ctrl_m->action_name);// sugar@115
 }// sugar@116
 void// sugar@101
 dmac_set_default_table_action(struct p4_ctrl_msg* ctrl_m) {// sugar@102
 debug("Action name: %s\n", ctrl_m->action_name);// sugar@103
 if(strcmp("forward", ctrl_m->action_name)==0) {// sugar@105
     struct dmac_action action;// sugar@106
     action.action_id = action_forward;// sugar@107
 uint8_t* port = (uint8_t*)((struct p4_action_parameter*)ctrl_m->action_params[0])->bitmap;// sugar@109
 memcpy(action.forward_params.port, port, 2);// sugar@110
     debug("Message from the control plane arrived.\n");// sugar@111
     debug("Set default action for dmac with action forward\n");// sugar@112
     dmac_setdefault( action );// sugar@113
 } else// sugar@114
 if(strcmp("bcast", ctrl_m->action_name)==0) {// sugar@105
     struct dmac_action action;// sugar@106
     action.action_id = action_bcast;// sugar@107
     debug("Message from the control plane arrived.\n");// sugar@111
     debug("Set default action for dmac with action bcast\n");// sugar@112
     dmac_setdefault( action );// sugar@113
 } else// sugar@114
 debug("Table setdefault: action name mismatch (%s).\n", ctrl_m->action_name);// sugar@115
 }// sugar@116
 void recv_from_controller(struct p4_ctrl_msg* ctrl_m) {// sugar@119
     debug("MSG from controller %d %s\n", ctrl_m->type, ctrl_m->table_name);// sugar@120
     if (ctrl_m->type == P4T_ADD_TABLE_ENTRY) {// sugar@121
 if (strcmp("smac", ctrl_m->table_name) == 0)// sugar@123
     smac_add_table_entry(ctrl_m);// sugar@124
 else// sugar@125
 if (strcmp("dmac", ctrl_m->table_name) == 0)// sugar@123
     dmac_add_table_entry(ctrl_m);// sugar@124
 else// sugar@125
 debug("Table add entry: table name mismatch (%s).\n", ctrl_m->table_name);// sugar@126
     }// sugar@127
     else if (ctrl_m->type == P4T_SET_DEFAULT_ACTION) {// sugar@128
 if (strcmp("smac", ctrl_m->table_name) == 0)// sugar@130
     smac_set_default_table_action(ctrl_m);// sugar@131
 else// sugar@132
 if (strcmp("dmac", ctrl_m->table_name) == 0)// sugar@130
     dmac_set_default_table_action(ctrl_m);// sugar@131
 else// sugar@132
 debug("Table setdefault: table name mismatch (%s).\n", ctrl_m->table_name);// sugar@133
     }// sugar@134
 }// sugar@135
 backend bg;// sugar@139
 void init_control_plane()// sugar@140
 {// sugar@141
     debug("Creating control plane connection...\n");// sugar@142
     bg = create_backend(3, 1000, "localhost", 11111, recv_from_controller);// sugar@143
     launch_backend(bg);// sugar@144
 /*// sugar@145
 struct smac_action action;// sugar@147
 action.action_id = action_mac_learn;// sugar@148
 smac_setdefault(action);// sugar@149
 debug("smac setdefault\n");// sugar@150
 struct dmac_action action2;// sugar@152
 action2.action_id = action_bcast;// sugar@153
 dmac_setdefault(action2);// sugar@154
 debug("dmac setdefault\n");// sugar@155
 */// sugar@156
 }// sugar@157
