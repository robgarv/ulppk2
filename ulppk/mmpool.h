
/*
 *****************************************************************

<GPL>

Copyright: © 2001-2015 Robert C Garvey

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 .
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 .
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
X-Comment: On Debian systems, the complete text of the GNU General Public
 License can be found in `/usr/share/common-licenses/GPL-3'.

</GPL>
*********************************************************************
*/
#ifndef MMPOOL_H_
#define MMPOOL_H_

#include <mmfor.h>
#include <dqacc.h>


/**
 * @file mmpool.c
 * 
 * @brief Memory Mapped Buffer Pool Management.
 * 
 * This package defines a persistent buffer pool. Buffers are mapped to
 * disk files through the mmfor, mmapfile and mmatom mechanisms.
 * 
 * A buffer pool consists of the following objects
 * 
 * 1) A BUFFER POOL MANAGEMENT FILE (BPMF). This contains control
 *    structures necessary to manage the allocation and deallocation
 *    of buffers.
 * 2) A BUFFER POOL CONTENTS FILE (BPCF). This is a file of fixed
 *    length buffers (records).
 * 
 * Both kinds of files are managed as memory mapped files.
 * 
 * The BPMF consists of two deque headers (see dqacc.h) and an array
 * of deque items. Deque items are indices of buffers in the contents
 * (BPCF) file. The two deque headers represent two sets of buffers. The
 * first set contains the buffers "in the pool", which are available for use.
 * This is called the "in deque", dq_inpool. The second set are the buffers
 * which have been allocated for use and are therefore "out of the pool".
 * This set is called the "out deque", dq_outpool.
 * 
 * Each buffer in the BPCF file consists of a short control header
 * and user specifed data.
 *
 * Manages pools of memory mapped fixed length records.
 *
 * BPMF == Buffer Pool Management Files. Contains information used to
 * manage the state of the buffer pool. This structure contains two double
 * ended queues (deques). One deque contains buffer pool indices of those
 * buffers which have not been allocated and are hence "in the pool". The
 * other contains indices of those blocks have have been allocated and are
 * hence "out of the pool" and presumably being used by processes.
 *
 * BPCF == Buffer Pool Content Files ... contains the actual buffers, an
 * array of fixed length memory blocks.
 */
 /*
  ***********************************
  * ENVIRONMENT VARIABLES -- environment variables
  * read by this library.
  ***********************************
 */
 
 // Data directory
 
 #define MMPOOL_ENV_DATA_DIR "MMPOOL_DATA_DIR"
 #define DEFAULT_MPOOL_ENV_DATA_DIR "/var/local/ulpkk/pools"
 

#define MMPOOL_MAX_POOL_NAME 32

// RCG PATCH typedef unsigned long BPOOL_INDEX;	// buffer pool index
typedef size_t BPOOL_INDEX;	// buffer pool index

typedef struct _mmpool_variables {
	int init;
	char* data_dir;			// data directory string
} MMPOOL_VARIABLES;

/**
 * Status codes,
 */
typedef enum  _enum_bpool_status {
	BPOOL_STAT_CREATED = 1,			///< A new buffer pool was created
	BPOOL_STAT_OPENED_R,			///< an existing buffer pool was opened for reading
	BPOOL_STAT_OPENED_W,			///< an existing buffer pool was opened for writing
	BPOOL_STAT_OPENED_RW			///<  an existing buffer pool was opened for read/write
} BPOOL_STATUS;

/**
 * Buffer Pool Contents File Reference structure
 */
typedef struct _bpcf_buffer {
	char sync_word[2];			///< 2 Byte sync word always contains 0xA4A4 (10100101)
	unsigned short bp_id;		///< Buffer pool id. Which pool does this buffer belong to?
	BPOOL_INDEX bpindex;		///< index into the buf byte array of start of buffer.
	size_t buff_len;			///< length is size of user data + header
} BPCF_BUFFER_REF;

/**
 * Buffer pool status structure
 */
typedef struct _bpmf_stats {
	char name[MMPOOL_MAX_POOL_NAME+1];	///< symbolic name of the pool
	unsigned short bp_id;		///< buffer pool ID
	unsigned short rqst_capacity;	///< requested min capacity (count of buffers)
	unsigned short capacity;	///< actual allocated capacity (count of buffers)
	size_t max_data_size;		///< max data capacity of buffers in this pool. (buffer user data bytes)
	unsigned short remaining;	///<  buffers remaining in pool
	long align4byte[0];			///< makes this end on a 4 byte alignment
} BPMF_STATS;					///< Pool statistics

/**
 * Buffer Pool Management File structure
 */
typedef struct _bpmf_rec {
	BPMF_STATS stats;			///< pool statstics
	size_t indqbuffx;			///< index of in the pool deque buffer area
	size_t outdqbuffx;			///< index of the out of pool deque buffer area
	DQHEADER dq_inpool;			///< deque header for buffers in the pool
	DQHEADER dq_outpool;		///< deque header for buffers out of the pool
} BPMF_REC;

/**
 * Buffer pool handle structure.
 */
typedef struct _bpmf_pool_handle {
	char pool_name[MMPOOL_MAX_POOL_NAME+1];	///< Symbolic name of the buffer pool
	unsigned short status;		///< status word.
	unsigned short spare;		///< keep alignment nice
	MMA_HANDLE* bpmfp;			///< Memory mapped atom handle to BPMF object
	MMFOR_HANDLE* bpcfp;		///< File of Records handle to BPCF object
	BPMF_REC* bpmf_recp;		///< ptr to in memory representation of BPMF record
} BPOOL_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define a buffer pool. If the pool has already been defined, no action
 * is taken. A valid handle is returned. The status will be BPOOL_OPENED_RW
 * to indicate the pool has been opened for read/write.
 */

BPOOL_HANDLE* mmpool_define_pool(
	char name[MMPOOL_MAX_POOL_NAME],	// Symbolic name of the buffer pool
	unsigned short bp_id,		// Buffer pool ID
	size_t max_data_size,		// max number of user bytes to be written to these buffers
	unsigned short rqst_capacity	// min number of buffers to allocate
);

/*
 * Open a buffer pool. If the pool does not exist or another error occurs,
 * this function returns NULL. Otherwise it returns a pointer to a 
 * BPOOL_HANDLE.
 */
BPOOL_HANDLE* mmpool_open(char* pool_name);

/*
 * Close buffer pool handle.
 */
int mmpool_close(BPOOL_HANDLE* bphp);

/*
 * Get a buffer from the pool
 */
BPCF_BUFFER_REF* mmpool_getbuff(BPOOL_HANDLE* bphp);

/*
 * Return a buffer to the pool. Returns 0 if successful, non-zero 
 * error code otherwise.
 */
int mmpool_putbuff(BPOOL_HANDLE* bphp, BPCF_BUFFER_REF* buff_refp);

/*
 * Get pointer to stats structure.
 */ 
BPMF_STATS* mmpool_getstats(BPOOL_HANDLE* bphp);

/*
 * Deallocate all buffers currently out of the pool. Zap 'em clean and
 * put them back in the "in pool" deque. Return count of items returned to
 * the pool.
 */

unsigned short mmpool_zap_pool(BPOOL_HANDLE* bphp);

/*
 * Given the symbolic name of a pool, derive the BPMF file name
 */
char* mmpool_bpmf_filename(char* pool_name);

/*
 * Given the symbolic name of a pool, derive the BPCF file name (contents file)
 */
char* mmpool_bpcf_filename(char* pool_name);

/*
 * Determine if file structure for the named pool exists.
 */
int mmpool_bpfiles_exist(char* pool_name);
 
/*
 * Given a buffer reference, return a void pointer to the user
 * data area of the buffer.
 */
void* mmpool_buffer_data(BPCF_BUFFER_REF* buff_refp);

/*
 * Given a valid buffer data pointer, return a pointer 
 * to its buffer reference.
 */
BPCF_BUFFER_REF* mmpool_buffer_data2refp(void* datap);


/*
 * Given a buffer index, form a buffer reference.
 */
BPCF_BUFFER_REF* mmpool_buffx2refp(BPOOL_HANDLE* bphp, BPOOL_INDEX bpx);

/*
 * Given a buffer reference, obtain the buffer pool buffer index
 */
BPOOL_INDEX mmpool_refp2buffx(BPCF_BUFFER_REF* buff_refp);

#ifdef __cplusplus
}
#endif
 
#endif /*MMPOOL_H_*/
