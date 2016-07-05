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
#include "messages.h"
#include <stdio.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* TODO: handle ntoh and hton !!! */

struct p4_header *create_p4_header(char* buffer, uint16_t offset, uint16_t maxlength) {
	struct p4_header* header;
	if (offset+sizeof(struct p4_header) > maxlength) return 0; /* buffer overflow */
	header = (struct p4_header*)(buffer + offset);
	header->version = P4_VERSION;
	header->type = 0;
	header->length = sizeof (struct p4_header);
	header->xid = 0;
	return header;
}

inline struct p4_header* netconv_p4_header(struct p4_header* m)
{
	m->length = htons(m->length);
	m->xid = htonl(m->xid);
	return m;
}

inline struct p4_header *unpack_p4_header(char* buffer, uint16_t offset) {
	return (struct p4_header*)(buffer + offset);
}

void check_p4_header( struct p4_header* a, struct p4_header* b) {
	assert(a->version == b->version);
	assert(a->type == b->type);
	assert(a->length == b->length);
	assert(a->xid == b->xid);
}

struct p4_add_table_entry* create_p4_add_table_entry(char* buffer, uint16_t offset, uint16_t maxlength) {
	struct p4_add_table_entry* add_table_entry;
        if (offset+sizeof(struct p4_add_table_entry) >= maxlength) return 0; /* buffer overflow */
	add_table_entry = (struct p4_add_table_entry*)(buffer + offset);
	add_table_entry->header.length = sizeof (struct p4_add_table_entry);
	add_table_entry->header.type = P4T_ADD_TABLE_ENTRY;
	add_table_entry->read_size = 0;
	add_table_entry->table_name[0] = '\0';
	return add_table_entry;
}

inline struct p4_add_table_entry* netconv_p4_add_table_entry(struct p4_add_table_entry* m) {
	return m; /*nothing to do*/
}

struct p4_field_match_lpm* add_p4_field_match_lpm(struct p4_add_table_entry* add_table_entry, uint16_t maxlength) {
	struct p4_field_match_lpm* field_match_lpm;
	if (add_table_entry->header.length + sizeof(struct p4_field_match_lpm) > maxlength) return 0; /* buffer overflow */
	field_match_lpm = (struct p4_field_match_lpm*)( ( (char*)add_table_entry ) + add_table_entry->header.length);
	add_table_entry->header.length += sizeof(struct p4_field_match_lpm);
	add_table_entry->read_size += 1;
	field_match_lpm->header.type = P4_FMT_LPM;
	field_match_lpm->prefix_length = 0;
	return field_match_lpm;
}

inline struct p4_field_match_lpm* netconv_p4_field_match_lpm(struct p4_field_match_lpm* m) {
	m->prefix_length = htons(m->prefix_length);
	return m;
}

inline struct p4_field_match_exact* netconv_p4_field_match_exact(struct p4_field_match_exact* m) {
        m->length = htons(m->length);
        return m;
}

inline struct p4_field_match_range* netconv_p4_field_match_range(struct p4_field_match_range* m) {
        m->length = htons(m->length);
        return m;
}

inline struct p4_field_match_ternary* netconv_p4_field_match_ternary(struct p4_field_match_ternary* m) {
        m->length = htons(m->length);
        return m;
}

inline struct p4_field_match_valid* netconv_p4_field_match_valid(struct p4_field_match_valid* m) {
        m->length = htons(m->length);
        return m;
}

inline struct p4_field_match_header* netconv_p4_field_match_complex(struct p4_field_match_header *m, int* size) {
	switch (m->type)
	{
		case P4_FMT_EXACT:
			netconv_p4_field_match_exact((struct p4_field_match_exact*)m);
			*size = sizeof(struct p4_field_match_exact);
                        break;
		case P4_FMT_TERNARY:
			netconv_p4_field_match_ternary((struct p4_field_match_ternary*)m);
			*size = sizeof(struct p4_field_match_ternary);
                        break;
		case P4_FMT_LPM:
			netconv_p4_field_match_lpm((struct p4_field_match_lpm*)m);
			*size = sizeof(struct p4_field_match_lpm);
                        break;
		case P4_FMT_RANGE:
			netconv_p4_field_match_range((struct p4_field_match_range*)m);
			*size = sizeof(struct p4_field_match_range);
                        break;
		case P4_FMT_VALID:
			netconv_p4_field_match_valid((struct p4_field_match_valid*)m);
			*size = sizeof(struct p4_field_match_valid);
			break;
		default:
			*size = 0;
			break;
	}
	return m;
}

inline struct p4_digest_field* netconv_p4_digest_field(struct p4_digest_field* m) {
	m->length = htonl(m->length);
        return m;
}

inline struct p4_action* netconv_p4_action( struct p4_action* m) {
	return m; /*nothing to do*/
}

inline struct p4_action_parameter* netconv_p4_action_parameter(struct p4_action_parameter* m) {
	m->length = htonl(m->length);
        return m;
}

inline struct p4_set_default_action* netconv_p4_set_default_action(struct p4_set_default_action* m) {
	/*netconv_p4_action(&(m->action)); TODO*/
	return m; /*nothing to do*/
}

inline struct p4_add_table_entry* unpack_p4_add_table_entry(char* buffer, uint16_t offset) {
        return (struct p4_add_table_entry*)(buffer + offset);
}

inline struct p4_field_match_header* unpack_p4_field_match_header(char* buffer, uint16_t offset) {
	return (struct p4_field_match_header*)(buffer + offset);
}

inline struct p4_field_match_lpm* unpack_p4_field_match_lpm(char* buffer, uint16_t offset) {
	return (struct p4_field_match_lpm*)(buffer + offset);
}

struct p4_field_match_exact* add_p4_field_match_exact(struct p4_add_table_entry* add_table_entry, uint16_t maxlength) {
	struct p4_field_match_exact* field_match_exact;
        if (add_table_entry->header.length + sizeof(struct p4_field_match_exact) > maxlength) return 0; /* buffer overflow */
        field_match_exact = (struct p4_field_match_exact*)( ( (char*)add_table_entry ) + add_table_entry->header.length);
	add_table_entry->header.length += sizeof(struct p4_field_match_exact);
        add_table_entry->read_size += 1;
        field_match_exact->header.type = P4_FMT_EXACT;
        field_match_exact->length = 0;
        return field_match_exact;
}

inline struct p4_field_match_exact* unpack_p4_field_match_exact(char* buffer, uint16_t offset) {
        return (struct p4_field_match_exact*)(buffer + offset);
}

struct p4_field_match_range* add_p4_field_match_range(struct p4_add_table_entry* add_table_entry, uint16_t maxlength) {
        struct p4_field_match_range* field_match_range;
        if (add_table_entry->header.length + sizeof(struct p4_field_match_range) > maxlength) return 0; /* buffer overflow */
        field_match_range = (struct p4_field_match_range*)( ( (char*)add_table_entry ) + add_table_entry->header.length);
        add_table_entry->header.length += sizeof(struct p4_field_match_range);
        add_table_entry->read_size += 1;
        field_match_range->header.type = P4_FMT_RANGE;
        field_match_range->length = 0;
	field_match_range->is_signed = false;
        return field_match_range;
}

inline struct p4_field_match_range* unpack_p4_field_match_range(char* buffer, uint16_t offset) {
        return (struct p4_field_match_range*)(buffer + offset);
}

struct p4_field_match_valid* add_p4_field_match_valid(struct p4_add_table_entry* add_table_entry, uint16_t maxlength) {
        struct p4_field_match_valid* field_match_valid;
        if (add_table_entry->header.length + sizeof(struct p4_field_match_valid) > maxlength) return 0; /* buffer overflow */
        field_match_valid = (struct p4_field_match_valid*)( ( (char*)add_table_entry ) + add_table_entry->header.length);
        add_table_entry->header.length += sizeof(struct p4_field_match_valid);
        add_table_entry->read_size += 1;
        field_match_valid->header.type = P4_FMT_VALID;
        field_match_valid->length = 0;
        field_match_valid->is_valid = false;
        return field_match_valid;
}

inline struct p4_field_match_valid* unpack_p4_field_match_valid(char* buffer, uint16_t offset) {
        return (struct p4_field_match_valid*)(buffer + offset);
}

struct p4_field_match_ternary* add_p4_field_match_ternary(struct p4_add_table_entry* add_table_entry, uint16_t maxlength) {
        struct p4_field_match_ternary* field_match_ternary;
        if (add_table_entry->header.length + sizeof(struct p4_field_match_ternary) > maxlength) return 0; /* buffer overflow */
        field_match_ternary = (struct p4_field_match_ternary*)( ( (char*)add_table_entry ) + add_table_entry->header.length);
        add_table_entry->header.length += sizeof(struct p4_field_match_ternary);
        add_table_entry->read_size += 1;
        field_match_ternary->header.type = P4_FMT_TERNARY;
        field_match_ternary->length = 0;
        field_match_ternary->priority = 0;
        return field_match_ternary;
}

inline struct p4_field_match_ternary* unpack_p4_field_match_ternary(char* buffer, uint16_t offset) {
        return (struct p4_field_match_ternary*)(buffer + offset);
}

struct p4_action* add_p4_action(struct p4_header* header, uint16_t maxlength) {
        struct p4_action* action;
        if (header->length + sizeof(struct p4_action) > maxlength) return 0; /* buffer overflow */
        action = (struct p4_action*)( ( (char*)header ) + header->length);
        header->length += sizeof(struct p4_action);
        action->description.type = P4_AT_ACTION;
	action->description.name[0] = '\0';
	action->param_size = 0;
        return action;
}

inline struct p4_action* unpack_p4_action(char* buffer, uint16_t offset) {
        return (struct p4_action*)(buffer + offset);
}

struct p4_action_parameter* add_p4_action_parameter(struct p4_header* header, struct p4_action* action, uint16_t maxlength) {
        struct p4_action_parameter* action_p;
        if (header->length + sizeof(struct p4_action_parameter) > maxlength) return 0; /* buffer overflow */
        action_p = (struct p4_action_parameter*)( ( (char*)header ) + header->length);
        header->length += sizeof(struct p4_action_parameter);
        action->param_size += 1;
	action_p->name[0] = '\0';
	action_p->length = 0;
        return action_p;
}

inline struct p4_action_parameter* unpack_p4_action_parameter(char* buffer, uint16_t offset) {
        return (struct p4_action_parameter*)(buffer + offset);
}

struct p4_set_default_action* create_p4_set_default_action(char* buffer, uint16_t offset, uint16_t maxlength) {
        struct p4_set_default_action* default_action;
        if (offset+sizeof(struct p4_set_default_action) >= maxlength) return 0; /* buffer overflow */
        default_action = (struct p4_set_default_action*)(buffer + offset);
        default_action->header.length = sizeof(struct p4_set_default_action);
	default_action->header.type = P4T_SET_DEFAULT_ACTION;
        default_action->table_name[0] = '\0';
	default_action->action.description.type = P4_AT_ACTION;
	default_action->action.description.name[0] = '\0';
	default_action->action.param_size = 0;
        return default_action;
}

inline struct p4_set_default_action* unpack_p4_set_default_action(char* buffer, uint16_t offset) {
        return (struct p4_set_default_action*)(buffer + offset);
}

struct p4_digest* create_p4_digest(char* buffer, uint16_t offset, uint16_t maxlength) {
	struct p4_digest* digest;
	if (offset+sizeof(struct p4_digest) >= maxlength) return 0; /* buffer overflow */
	digest = (struct p4_digest*)(buffer + offset);
	digest->header.length = sizeof(struct p4_digest);
	digest->header.type = P4T_DIGEST;
	digest->field_list_name[0] = '\0';
	digest->list_size = 0;
	return digest;
}

inline struct p4_digest* unpack_p4_digest(char* buffer, uint16_t offset) {
        return (struct p4_digest*)(buffer + offset);
}

struct p4_digest_field* add_p4_digest_field(struct p4_digest* digest, uint16_t maxlength) {
        struct p4_digest_field* digest_f;
        if (digest->header.length + sizeof(struct p4_digest_field) > maxlength) return 0; /* buffer overflow */
        digest_f = (struct p4_digest_field*)( ( (char*)digest ) + digest->header.length);
        digest->header.length += sizeof(struct p4_digest_field);
        digest->list_size += 1;
        digest_f->name[0] = '\0';
        digest_f->length = 0;
        return digest_f;
}

inline struct p4_digest_field* unpack_p4_digest_field(char* buffer, uint16_t offset) {
        return (struct p4_digest_field*)(buffer + offset);
}

