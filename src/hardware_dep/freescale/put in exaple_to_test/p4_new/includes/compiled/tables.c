#ifndef TABLES_C
#define TABLES_C
 #include "includes/shared/dataplane.h"// sugar@5
 #include "actions.h"// sugar@6
 #include "data_plane_data.h"// sugar@7

 lookup_table_t table_config[NB_TABLES] = {// sugar@9
 {// sugar@12
  .name= "smac",// sugar@13
  .type = LOOKUP_EXACT,// sugar@14
  .key_size = 6,// sugar@15
  .val_size = sizeof(struct smac_action),// sugar@16
  .min_size = 0, //512,// sugar@17
  .max_size = 255 //512// sugar@18
 },// sugar@19
 {// sugar@12
  .name= "dmac",// sugar@13
  .type = LOOKUP_EXACT,// sugar@14
  .key_size = 6,// sugar@15
  .val_size = sizeof(struct dmac_action),// sugar@16
  .min_size = 0, //512,// sugar@17
  .max_size = 255 //512// sugar@18
 },// sugar@19
 };// sugar@20
 counter_t counter_config[NB_COUNTERS] = {// sugar@22
 };// sugar@37
 p4_register_t register_config[NB_REGISTERS] = {// sugar@39
 };// sugar@52
#endif
