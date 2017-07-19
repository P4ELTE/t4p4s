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
#endif
