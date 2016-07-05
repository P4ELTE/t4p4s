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
#ifndef _MESSAGES_H_
#define _MESSAGES_H_ 1

#include <stdint.h>

#define P4_VERSION 1
#define true 1
#define false 0


struct p4_header {
	uint8_t version;	/* version */
	uint8_t type;		/* P4T constants */
	uint16_t length;	/* Length including this header */
	uint32_t xid;		/* Transaction ID */
};


enum p4_type {
	/* Immutable messages */
	P4T_HELLO = 0,
	P4T_ERROR = 1,
	P4T_ECHO_REQUEST = 2,
	P4T_ECHO_REPLY = 3,
	P4T_SUCCESS = 4,

	/* Switch informations */	
	P4T_GET_TABLE_DEFINITIONS = 100,
	P4T_GET_TABLE_COUNTERS = 101,
	P4T_GET_TABLE_ENTRIES = 102,

	/* Controller commands */
	P4T_SET_DEFAULT_ACTION = 103,
	P4T_ADD_TABLE_ENTRY = 104, /*with and without selector (action, action_profile)*/
	P4T_MODIFY_TABLE_ENTRY = 105,
	P4T_REMOVE_TABLE_ENTRY = 106,
	P4T_DROP_TABLE_ENTRIES = 107,
	P4T_ADD_AP_MEMBER = 108,
	P4T_REMOVE_AP_MEMBER = 109,

	/* Digest passed */
	P4T_DIGEST = 110
};

struct p4_hello {
	struct p4_header header;
	/* Hello element list */
	struct p4_hello_elem_header *elements; /* List of elements - 0 or more */
};

/* Hello elements types.
*/
enum p4_hello_elem_type {
	OFPHET_VERSIONBITMAP = 1 /* Bitmap of version supported. */
};

/* Common header for all Hello Elements */
struct p4_hello_elem_header {
	uint16_t type; /* One of OFPHET_*. */
	uint16_t length; /* Length in bytes of the element,
including this header, excluding padding. */
};

/* Version bitmap Hello Element */
struct p4_hello_elem_versionbitmap {
	uint16_t type; /* OFPHET_VERSIONBITMAP. */
	uint16_t length; /* Length in bytes of this element,
including this header, excluding padding. */
	/* Followed by:
		* - Exactly (length - 4) bytes containing the bitmaps, then
		* - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
		* bytes of all-zero bytes */
	uint32_t* bitmaps; /* List of bitmaps - supported versions */
};/* Version bitmap Hello Element */

struct p4_error {
	struct p4_header header;
	uint16_t error_code;
};

struct p4_success {
	struct p4_header header;
	uint32_t id;
};

#define P4_MAX_TABLE_NAME_LEN 128
#define P4_MAX_FIELD_NAME_LEN 128
#define P4_MAX_ACTION_NAME_LEN 128

enum p4_field_match_type {
	P4_FMT_EXACT = 0,
	P4_FMT_TERNARY = 1,
	P4_FMT_LPM = 2,
	P4_FMT_RANGE = 3,
	P4_FMT_VALID = 4
};

struct p4_field_match_desc {
	char name[P4_MAX_FIELD_NAME_LEN];
	uint8_t type;
};

enum p4_support_timeout {
	P4_STO_FALSE = 0,
	P4_STO_TRUE = 1
};

enum p4_action_type {
	P4_AT_ACTION = 0,
	P4_AT_ACTION_PROFILE = 1
};

struct p4_table_action_desc {
	uint8_t type;
	char name[P4_MAX_ACTION_NAME_LEN];
};

struct p4_table_definition {
	struct p4_header header;
	char name[P4_MAX_TABLE_NAME_LEN];
	uint8_t reads;
	uint16_t min_size;
	uint16_t max_size;
	uint16_t size;
	uint8_t support_timeout;
	uint8_t read_size; /* number of rules in reads */
	uint8_t action_size; /* number of actions */
	/* struct p4_field_match_desc[read_size]; */
	/* struct p4_table_action_desc[action_size]; */
};


#define MAX_ACTION_PARAMETER_NAME_LENGTH 128
#define MAX_ACTION_PARAMETER_BITMAP_LENGTH 128
struct p4_action_parameter {
	char name[MAX_ACTION_PARAMETER_NAME_LENGTH];
	uint32_t length; /* TODO: How can we figure this out?????Controller should know this in advance! P4 does not handle it. */
	char bitmap[MAX_ACTION_PARAMETER_BITMAP_LENGTH];
};

/* TODO: action_profiles can also be set as default action */

struct p4_action {
        struct p4_table_action_desc description; /* Action name of action profile name */
        uint8_t param_size;
	/* struct p4_action_parameter params[param_size]; */
};

struct p4_set_default_action {
	struct p4_header header;
	char table_name[P4_MAX_TABLE_NAME_LEN];
	struct p4_action action; /* Action or action profile instance */
};

struct p4_field_match_header {
	char name[P4_MAX_FIELD_NAME_LEN];
	uint8_t type; /* p4_field_match_type */
};

#define MAX_FIELD_LENGTH 256

struct p4_field_match_lpm {
	struct p4_field_match_header header;
	uint16_t prefix_length;
	char bitmap[MAX_FIELD_LENGTH];
};

struct p4_field_match_exact {
	struct p4_field_match_header header;
        uint16_t length;
	char bitmap[MAX_FIELD_LENGTH];
};

struct p4_field_match_range {
	struct p4_field_match_header header;
        uint16_t length;
	uint8_t is_signed; /* boolean variable */
        char min_bitmap[MAX_FIELD_LENGTH];
	char max_bitmap[MAX_FIELD_LENGTH];
};

struct p4_field_match_ternary {
	struct p4_field_match_header header;
        uint16_t length;
	uint8_t priority;
        char bitmap[MAX_FIELD_LENGTH];
	char mask[MAX_FIELD_LENGTH];
};

struct p4_field_match_valid {
	struct p4_field_match_header header;
        uint16_t length;
	uint8_t is_valid; /* could be true or false - boolean variable */
        char bitmap[MAX_FIELD_LENGTH];
};

struct p4_add_table_entry {
	struct p4_header header;
	char table_name[P4_MAX_TABLE_NAME_LEN];
	uint8_t read_size;
	/* struct p4_field_match matches[read_size]; */
	/* struct p4_action; */
};

#define P4_MAX_FIELD_LIST_NAME_LEN 128
#define P4_MAX_FIELD_NAME_LENGTH 128
#define P4_MAX_FIELD_VALUE_LENGTH 32

struct p4_digest_field {
        char name[P4_MAX_FIELD_NAME_LENGTH];
        uint32_t length; /* TODO: How can we figure this out?????Controller should know this in advance! P4 does not handle it. */
        char value[P4_MAX_FIELD_VALUE_LENGTH];	
};

struct p4_digest {
	struct p4_header header;
	char field_list_name[P4_MAX_FIELD_LIST_NAME_LEN];
	uint8_t list_size;
	/* struct p4_digest_field field_list[list_size]; */
};

struct p4_header *create_p4_header(char* buffer, uint16_t offset, uint16_t maxlength);
struct p4_header *unpack_p4_header(char* buffer, uint16_t offset);
void check_p4_header( struct p4_header* a, struct p4_header* b);
struct p4_header* netconv_p4_header(struct p4_header* m);

struct p4_add_table_entry* create_p4_add_table_entry(char* buffer, uint16_t offset, uint16_t maxlength);
struct p4_field_match_lpm* add_p4_field_match_lpm(struct p4_add_table_entry* add_table_entry, uint16_t maxlength);
struct p4_add_table_entry* unpack_p4_add_table_entry(char* buffer, uint16_t offset);
struct p4_field_match_header* unpack_p4_field_match_header(char* buffer, uint16_t offset);
struct p4_field_match_lpm* unpack_p4_field_match_lpm(char* buffer, uint16_t offset);
struct p4_field_match_exact* add_p4_field_match_exact(struct p4_add_table_entry* add_table_entry, uint16_t maxlength);
struct p4_field_match_exact* unpack_p4_field_match_exact(char* buffer, uint16_t offset);
struct p4_field_match_range* add_p4_field_match_range(struct p4_add_table_entry* add_table_entry, uint16_t maxlength);
struct p4_field_match_range* unpack_p4_field_match_range(char* buffer, uint16_t offset);
struct p4_field_match_valid* add_p4_field_match_valid(struct p4_add_table_entry* add_table_entry, uint16_t maxlength);
struct p4_field_match_valid* unpack_p4_field_match_valid(char* buffer, uint16_t offset);
struct p4_field_match_ternary* add_p4_field_match_ternary(struct p4_add_table_entry* add_table_entry, uint16_t maxlength);
struct p4_field_match_ternary* unpack_p4_field_match_ternary(char* buffer, uint16_t offset);
struct p4_action* add_p4_action(struct p4_header* header, uint16_t maxlength);
struct p4_action* unpack_p4_action(char* buffer, uint16_t offset);
struct p4_action_parameter* add_p4_action_parameter(struct p4_header* header, struct p4_action* action, uint16_t maxlength);
struct p4_action_parameter* unpack_p4_action_parameter(char* buffer, uint16_t offset);
struct p4_set_default_action* create_p4_set_default_action(char* buffer, uint16_t offset, uint16_t maxlength);
struct p4_set_default_action* unpack_p4_set_default_action(char* buffer, uint16_t offset);
struct p4_digest* create_p4_digest(char* buffer, uint16_t offset, uint16_t maxlength);
struct p4_digest* unpack_p4_digest(char* buffer, uint16_t offset);
struct p4_digest_field* add_p4_digest_field(struct p4_digest* digest, uint16_t maxlength);
struct p4_digest_field* unpack_p4_digest_field(char* buffer, uint16_t offset);

struct p4_field_match_lpm* netconv_p4_field_match_lpm(struct p4_field_match_lpm* m);
struct p4_field_match_exact* netconv_p4_field_match_exact(struct p4_field_match_exact* m);
struct p4_field_match_range* netconv_p4_field_match_range(struct p4_field_match_range* m);
struct p4_field_match_ternary* netconv_p4_field_match_ternary(struct p4_field_match_ternary* m);
struct p4_field_match_valid* netconv_p4_field_match_valid(struct p4_field_match_valid* m);
struct p4_digest_field* netconv_p4_digest_field(struct p4_digest_field* m);
struct p4_action* netconv_p4_action( struct p4_action* m);
struct p4_action_parameter* netconv_p4_action_parameter(struct p4_action_parameter* m);
struct p4_set_default_action* netconv_p4_set_default_action(struct p4_set_default_action* m);
struct p4_field_match_header* netconv_p4_field_match_complex(struct p4_field_match_header *m, int* size);
struct p4_add_table_entry* netconv_p4_add_table_entry(struct p4_add_table_entry* m);

#endif
