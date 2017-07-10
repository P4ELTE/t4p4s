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
#ifndef FREESCALE_TABLES_H
#define FREESCALE_TABLES_H

typedef struct extended_table_s {
    void*     rte_table;
    uint8_t   size;
    uint8_t** content;
} extended_table_t;

//=============================================================================
// Table size limits

#ifdef RTE_ARCH_X86_64
#define HASH_ENTRIES		1024
#else
#define HASH_ENTRIES		1024
#endif
#define LPM_MAX_RULES         1024
#define LPM6_NUMBER_TBL8S (1 << 16)

#define TABLE_MAX 256

#endif

typedef struct {
	void			*next;		/**< Pointer to next flow in list*/
	uint8_t	    	        *value;
	uint8_t			*key;
} my_odp_flow_entry_t;

typedef struct {
	odp_spinlock_t		lock;	/**< Bucket lock*/
	my_odp_flow_entry_t	*next;	/**< Pointer to first flow entry in bucket*/
} my_flow_bucket_t;


#define LOCK(a)      odp_spinlock_lock(a)
#define UNLOCK(a)    odp_spinlock_unlock(a)
#define LOCK_INIT(a) odp_spinlock_init(a)
