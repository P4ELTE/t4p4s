#include "backend.h"
#include "includes/shared/dataplane.h"
#include "includes/compiled/actions.h"
#include "freescale_tables.h"
#include <odp/api/spinlock.h>


void        exact_create (lookup_table_t* t, int socketid){
	t->table = malloc(sizeof(my_flow_bucket_t));
}
void          lpm_create (lookup_table_t* t, int socketid);
void      ternary_create (lookup_table_t* t, int socketid);

void    table_setdefault (lookup_table_t* t,                              uint8_t* value);

void           exact_add (lookup_table_t* t, uint8_t* key,                uint8_t* value){
	my_odp_flow_entry_t *head; 
	uint8_t* temp;
	my_flow_bucket_t *bkt = (my_flow_bucket_t *)t->table;
	my_odp_flow_entry_t *flow = malloc(sizeof(my_odp_flow_entry_t));
        flow->key = (uint8_t*)malloc(6*sizeof(uint8_t));
	flow->key[0] = key[0];
	flow->key[1] = key[1];
	flow->key[2] = key[2];
	flow->key[3] = key[3];
	flow->key[4] = key[4];
	flow->key[5] = key[5];
        flow->value = (uint8_t*)malloc(sizeof(struct dmac_action));
	memcpy(flow->value, value, sizeof(struct dmac_action));

	//LOCK(&bkt->lock);
	/*Check that entry already exist or not*/
	temp = exact_lookup(t, key);
	if (temp)
		return;

	if (!bkt->next) {
		bkt->next = flow;
	} else {
		head = bkt->next;
		flow->next = head;
		bkt->next = flow;
	}
	//UNLOCK(&bkt->lock);
}


void             lpm_add (lookup_table_t* t, uint8_t* key, uint8_t depth, uint8_t* value);
void         ternary_add (lookup_table_t* t, uint8_t* key, uint8_t* mask, uint8_t* value);

uint8_t*    exact_lookup (lookup_table_t* t, uint8_t* key){
        my_odp_flow_entry_t      *flow, *head;
	head = ((my_flow_bucket_t *)t->table)->next;
	for (flow = head; flow != NULL; flow = flow->next) {
		//printf("FLOW ENTRIES: %02x:%02x:%02x:%02x:%02x:%02x \n", flow->key[0],flow->key[1],flow->key[2],flow->key[3],flow->key[4],flow->key[5]);
		if (key[0] == flow->key[0] && key[1] == flow->key[1] && key[2] == flow->key[2] && key[3] == flow->key[3] && key[4] == flow->key[4] && key[5] == flow->key[5])
		{
			return flow->value;
		}
	}
        printf("Key not found: %02x:%02x:%02x:%02x:%02x:%02x \n", key[0],key[1],key[2],key[3],key[4],key[5]);
	return NULL;

}
uint8_t*      lpm_lookup (lookup_table_t* t, uint8_t* key);
uint8_t*  ternary_lookup (lookup_table_t* t, uint8_t* key);
