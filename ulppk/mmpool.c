
/*
 *****************************************************************

<GPL>

Copyright: © 2001-2009 Robert C Garvey

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

/**
 * @file mmpool.c
 * @brief Memory Mapped Buffer Pool Module
 *
 * A buffer pool is a collection of memory blocks which can be allocated
 * and returned to future use by calling applications. I some respects, its
 * operation is reminiscent of standard C library's heap (malloc and free).
 *
 * But memory allocators behind malloc and free are considerably more sophisticated
 * than a buffer pool. They allow allocation of arbitrarily sized blocks of memory,
 * implement strategies to combat fragementation, etc. (See
 * http://gee.cs.oswego.edu/dl/html/malloc.html for a good discussion.)
 *
 * Buffer pools are a less flexible but considerably faster memory allocation scheme.
 * The applications code defines one or more pools of fixed size blocks.
 *
 * A buffer pool is characterized by
 * <ol>
 * <li>A logical name and numberic ID</li>
 * <li>The size of the buffers in bytes</li>
 * <li>The number of total buffers allocated to the pool</li>
 * <li>The number of unused buffers remaining in the pool</li>
 * </ol>
 *
 * See mmpool.h for a discussion of the data structures.
 * See the mmbuffpool.c utility, which is used to create, report, display and zap
 * (clear out) buffer pools.)
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stddef.h>

#include <mmpool.h>
#include <appenv.h>
#include <diagnostics.h>

static MMPOOL_VARIABLES variables = {
	0,				// init flag
	NULL			// data dir name 
};

static void init();

static MMA_HANDLE* new_bpmf(
	char name[MMPOOL_MAX_POOL_NAME],	// Symbolic name of the buffer pool
	unsigned short bp_id,		// Buffer pool ID
	size_t max_data_size,		// max number of user bytes to be written to these buffers
	unsigned short rqst_capacity	// min number of buffers to allocate
);
static MMFOR_HANDLE* new_bpcf(
	char name[MMPOOL_MAX_POOL_NAME],	// name of the pool
	size_t max_data_size,		// max number of user data bytes one buffer will hold
	unsigned short nitems		// number of buffers to create
);
static off_t bpcf_data_offset();

static int allocate_buffers(MMA_HANDLE* bpmfhp, MMFOR_HANDLE* bpcfhp);

static char* bpfile_full_path(const char* filename);

BPCF_BUFFER_REF* mmpool_buffx2refp(BPOOL_HANDLE* bphp, BPOOL_INDEX bpx);

static MMA_HANDLE* open_bpmf(char* pool_name);

static MMFOR_HANDLE* open_bpcf(char* pool_name);

static int search_deque(DQHEADER* dequep, BPOOL_INDEX bpx);

static void lock_pool(BPOOL_HANDLE* bphp);

static void unlock_pool(BPOOL_HANDLE* bphp);

#if 0
static void setup_deques(BPOOL_HANDLE* bphp);
#endif
/**
 * @brief Define a buffer pool. If the pool has already been defined, no action
 * is taken. A valid handle is returned. The status will be BPOOL_OPENED_RW
 * to indicate the pool has been opened for read/write.
 *
 * @param name Symbolic name of the pool
 * @param bp_id Buffer pool ID assigned by the caller.
 * @param max_data_size Max number of user bytes to be written to these buffers
 * @param rqst_capacity Min number of buffers to allocate.
 * @return Pointer to buffer pool handle.
 */
BPOOL_HANDLE* mmpool_define_pool(
	char name[MMPOOL_MAX_POOL_NAME],	// Symbolic name of the buffer pool
	unsigned short bp_id,		// Buffer pool ID
	size_t max_data_size,		// max number of user bytes to be written to these buffers
	unsigned short rqst_capacity	// min number of buffers to allocate
) {
	MMA_HANDLE* bpmf_mmahp = NULL;		// mmatom handle to BPMF object
	MMFOR_HANDLE* bpcf_mmafhp = NULL;		// MM File of Records handle to BPCF object
	BPMF_REC*  bpmf_recp;
	BPOOL_HANDLE* bphp;

	init();
	if (mmpool_bpfiles_exist(name)) {
		return mmpool_open(name);
	}	
	bpmf_mmahp = new_bpmf(name, bp_id, max_data_size, rqst_capacity);
	if (NULL == bpmf_mmahp) {
		return NULL;
	}
	
	// Get ptr to memory mapped BPMF_REC 
	bpmf_recp = (BPMF_REC*)mma_data_pointer(bpmf_mmahp);
	bpcf_mmafhp = new_bpcf(name, max_data_size, bpmf_recp->stats.capacity);

	if (NULL == bpcf_mmafhp) {
		// TODO: Need to destroy the mmahp here.
		return NULL;
	}
	
	// We have the Buffer Pool Management File setup ... we have
	// the Buffer Pool Contents File setup. Now all we have to do
	// is divide the bytes in the BPCF into buffers and allocate
	// them to the pool.
	
	allocate_buffers(bpmf_mmahp, bpcf_mmafhp);
	
	// Construct a BPOOL_HANDLE structure 
	bphp = (BPOOL_HANDLE*)calloc(1, sizeof(BPOOL_HANDLE));
	strncpy(bphp->pool_name, name, MMPOOL_MAX_POOL_NAME);
	bphp->bpmfp = bpmf_mmahp;
	bphp->bpcfp = bpcf_mmafhp;
	bphp->bpmf_recp = bpmf_recp;
			
	return bphp;	
}
/**
 * Open a buffer pool. If the pool does not exist or another error occurs,
 * this function returns NULL. Otherwise it returns a pointer to a 
 * BPOOL_HANDLE.
 * @param pool_name Symbolic name of the buffer pool
 */
BPOOL_HANDLE* mmpool_open(char* pool_name) {
	MMA_HANDLE* bpmfp;
	MMFOR_HANDLE* bpcfp;
	BPOOL_HANDLE* bphp;
	
	init();
	
	// First, make sure the pool management and contents files exist
	if (!mmpool_bpfiles_exist(pool_name)) {
		return NULL;		// We need to define this pool!
	}
	
	// Pool files exist ... so open the Buffer Pool Management File
	bpmfp = open_bpmf(pool_name);
	
	if (bpmfp == NULL) {
		// Error encountered opening the BPMF ... bail out
		return NULL;
	}
	
	// Open the Buffer Pool Contents File
	bpcfp = open_bpcf(pool_name);
	
	if (bpcfp == NULL) {
		// Error encountered opening the BPCF ... clean up and bail out
		mmapfile_close(bpmfp);
		return NULL;
	}
	
	// Construct a BPOOL_HANDLE structure 
	bphp = (BPOOL_HANDLE*)calloc(1, sizeof(BPOOL_HANDLE));
	strncpy(bphp->pool_name, pool_name, MMPOOL_MAX_POOL_NAME);
	bphp->bpmfp = bpmfp;
	bphp->bpcfp = bpcfp;
	bphp->bpmf_recp = (BPMF_REC*)mma_data_pointer(bpmfp);

	return bphp;
}

/**
 * @brief Close a buffer pool and unmap all associated memory regions
 * @param bphp Pointer to buffer pool handle structure.
 * @return 0 on success, non-zero on failure.
 */
int mmpool_close(BPOOL_HANDLE* bphp) {
	int retval = 0;

	if (bphp->bpcfp != NULL) {
		mmfor_close(bphp->bpcfp);
		if (bphp->bpmfp != NULL) {
			mmapfile_close(bphp->bpmfp);
		} else {
			APP_ERR(stderr, "Unable to close Buffer Pool Management File after Contents File Closed!");
		}
	} else {
		retval = 1;
	}
	return retval;
}


/**
 * @brief Get a buffer from the pool
 *
 * @param bphp Pointer to buffer pool handle structure.
 * @return Pointer to a buffer reference structure.
 *
 */
BPCF_BUFFER_REF* mmpool_getbuff(BPOOL_HANDLE* bphp) {
	BPCF_BUFFER_REF* buff_refp = NULL;
	BPOOL_INDEX bpx;
	lock_pool(bphp);
	if (!dq_rtd(&bphp->bpmf_recp->dq_inpool, &bpx)) {
		// Get reference to the buffer
		buff_refp = mmpool_buffx2refp(bphp, bpx);
		// Add the buffer to the out of pool deque
		dq_abd(&bphp->bpmf_recp->dq_outpool, &bpx);
		bphp->bpmf_recp->stats.remaining--;
	}
	unlock_pool(bphp);
	return buff_refp;
}

/**
 * @brief Return a buffer to the pool.
 *
 * @param bphp Pointer to buffer pool handle structure.
 * @param buffp Pointer to buffer reference structure of buffer to return
 * @return Pointer to a buffer reference structure.
 * @return 0 if successful, non-zero error code otherwise.
 */
int mmpool_putbuff(BPOOL_HANDLE* bphp, BPCF_BUFFER_REF* buffp) {
	int error = 0;
	lock_pool(bphp);
	/*
	 * Search the out of pool deque for the item. 
	 */
	if (search_deque(&bphp->bpmf_recp->dq_outpool, buffp->bpindex)) {
		// Found a match ...  do the deallocation.
		dq_abd(&bphp->bpmf_recp->dq_inpool, &buffp->bpindex);
		bphp->bpmf_recp->stats.remaining++;
	} else {
		// The buffer pool index of the passed reference is NOT
		// in use ... this is an error condition
		error = 1;
	}
	unlock_pool(bphp);
	return error;
}

/**
 * @brief Given a buffer reference, return a pointer to the memory mapped
 * data region of the buffer.
 *
 * @param buff_refp Pointer to a buffer reference structure.
 * @return Pointer to the memory mapped buffer.
 */
void* mmpool_buffer_data(BPCF_BUFFER_REF* buff_refp) {
	void* p;
	
	p = ((void*) buff_refp) + bpcf_data_offset();
	return p;
}

/**
 * @brief Given a valid buffer data pointer, return a pointer
 * to its buffer reference.
 *
 * @param datap Pointer to the memory mapped buffer.
 * @return Pointer to a buffer reference structure.
 */
BPCF_BUFFER_REF* mmpool_buffer_data2refp(void* datap) {
	BPCF_BUFFER_REF* buffrefp;

	buffrefp = (BPCF_BUFFER_REF*)(datap - bpcf_data_offset());
	return buffrefp;
}
/**
 * @brief Get pointer to buffer pool stats structure.
 *
 * This provides the current state of buffer pool status
 * and statistics.
 *
 * @param bphp Pointer to buffer pool handle structure.
 * @return Pointer to buffer pool stats structure.
 */ 
BPMF_STATS* mmpool_getstats(BPOOL_HANDLE* bphp) {
	return &bphp->bpmf_recp->stats;
}

/**
 * @brief Deallocate all buffers currently out of the pool.
 *
 * Zap 'em clean and put them back in the "in pool" deque.
 *
 * @param bphp Pointer to buffer pool handle structure.
 * @return count of items returned to the pool.
 */
unsigned short mmpool_zap_pool(BPOOL_HANDLE* bphp) {
	BPOOL_INDEX bpx;
	BPCF_BUFFER_REF* bufrefp;
	void* vp;
	size_t user_data_size;
	int return_count = 0;

	while (!dq_rtd(&bphp->bpmf_recp->dq_outpool, &bpx)) {
		bufrefp = mmpool_buffx2refp(bphp, bpx);			// get buffer reference
		vp = mmpool_buffer_data(bufrefp);				// get pointer to user data
		user_data_size = bphp->bpmf_recp->stats.max_data_size;	// get the max user data size
		memset(vp, 0, user_data_size);					// zap the user data
		dq_abd(&bphp->bpmf_recp->dq_inpool, &bpx);		// put it back into the pool
		bphp->bpmf_recp->stats.remaining++;				// bump current buffer count
		return_count++;
	}
	return return_count;
}

/**
 * @brief Given the symbolic name of a pool, derive the BPMF file name
 *
 * @param pool_name Symbolic name of the pool
 * @return The name of the buffer pool management file (not full path).
 */
char* mmpool_bpmf_filename(char* pool_name) {
	static char buff[MMPOOL_MAX_POOL_NAME+6];
	sprintf(buff, "%s.bpmf", pool_name);
	return buff;
}

/**
 * @brief Given the symbolic name of a pool, derive the BPCF file name (contents file)
 *
 * @param pool_name Symbolic name of the pool
 * @return The name of the buffer pool management file (not full path).
 */
char* mmpool_bpcf_filename(char* pool_name) {
	static char buff[MMPOOL_MAX_POOL_NAME+6];
	sprintf(buff, "%s.bpcf", pool_name);
	return buff;
}

/*
 * Determine if file structure for the named pool exists. Does not
 * do any integrity checking. Returns non-zero if named pool exists.
 */
int mmpool_bpfiles_exist(char* pool_name) {
	char* dirname;
	static char fname1[1024];
	static char fname2[1024];
	int status = 0;
	DIR* dp;
	struct dirent* dirp;
	
	dirname = variables.data_dir;
	
	if ((dp = opendir(dirname)) != NULL) {
		strcpy(fname1, mmpool_bpmf_filename(pool_name));
		strcpy(fname2, mmpool_bpcf_filename(pool_name));
		while ((dirp = readdir(dp)) != NULL) {
			if (strcmp(dirp->d_name, fname1) == 0) {
				status++;
			} else if (strcmp(dirp->d_name, fname2) == 0) {
				status++;
			}
			if (status == 2) {
				return 1;		// both BPMF and BPCF files found
			}
		}
	}
	// One or the other or both not found.
	return 0;
}

static void init() {
	char* strp;
#if 0
	char* envdatadir;
	
	if (!variables.init) {
		variables.init = 1;
		envdatadir = getenv(MMPOOL_ENV_DATA_DIR);
		if ((NULL == envdatadir) || (strlen(envdatadir) == 0)) {
			strp = (char*)calloc(strlen(DEFAULT_MPOOL_ENV_DATA_DIR) + 1, 1);
			strcpy(strp,DEFAULT_MPOOL_ENV_DATA_DIR);
		} else {
			strp = (char*)calloc(strlen(envdatadir) + 1, 1);
			strcpy(strp, envdatadir);
		}
	}
#endif
	strp = appenv_register_env_var(MMPOOL_ENV_DATA_DIR, DEFAULT_MPOOL_ENV_DATA_DIR);
	variables.data_dir = strp;
}

static MMA_HANDLE* format_bpmf(
	MMA_HANDLE* mmahp,			// memory mapped atom handle
	char name[MMPOOL_MAX_POOL_NAME],	// Symbolic name of the buffer pool
	unsigned short bp_id,		// Buffer pool ID
	size_t max_data_size,		// max number of user bytes to be written to these buffers
	unsigned short rqst_capacity,	// min number of buffers to allocate
	unsigned short alloc_capacity	// actual number of buffers allocated to the pool
) {
	void* p0;
	BPMF_STATS* statsp;
	BPMF_REC* bpmf_recp;
	DQHEADER* dqinp;
	DQHEADER* dqoutp;
	void* b1p;
	void* b2p;
	
	p0 = (void*)mma_data_pointer(mmahp);		// get ptr to first byte of data
	statsp = (BPMF_STATS*)p0;					// get ptr to stats
	bpmf_recp = (BPMF_REC*)p0;					// and ptr to BPMF_REC
	
	// Calculate pointers to in pool and out of pool deques
	
	dqinp = &bpmf_recp->dq_inpool;
	dqoutp = &bpmf_recp->dq_outpool;
	
	// Calculate pointers into memory mapped area of buffers
	// for the two deques. "In pool" buffer area comes first ...
	b1p = ((void*)dqoutp) + sizeof(DQHEADER);
	b2p = b1p + alloc_capacity * sizeof(BPOOL_INDEX);

	// Calculate the offsets relative to p0 of these two deque slot buffers
	bpmf_recp->indqbuffx = (size_t)(b1p - p0);
	bpmf_recp->outdqbuffx = (size_t)(b2p - p0);
	
	// Format the stats record
	strncpy(statsp->name, name, MMPOOL_MAX_POOL_NAME);
	statsp->bp_id = bp_id;
	statsp->rqst_capacity = rqst_capacity;
	statsp->capacity = alloc_capacity;
	statsp->max_data_size = max_data_size;
	statsp->remaining = statsp->capacity;
	 
	// Initialize the deques.
	dq_init_memmap(statsp->capacity, sizeof(BPOOL_INDEX), bpmf_recp->indqbuffx, dqinp);
	dq_init_memmap(statsp->capacity, sizeof(BPOOL_INDEX), bpmf_recp->outdqbuffx, dqoutp);
	
	return mmahp;
}


static MMA_HANDLE* new_bpmf(
	char name[MMPOOL_MAX_POOL_NAME],	// Symbolic name of the buffer pool
	unsigned short bp_id,		// Buffer pool ID
	size_t max_data_size,		// max number of user bytes to be written to these buffers
	unsigned short rqst_capacity	// min number of buffers to allocate
) {
	static char fpath[1024];
	size_t bpmf_size = 0;
	long pagesize;
	long pages;
	long remainder;
	long slop;
	long itemslop;
	unsigned short alloc_capacity;
	MMA_HANDLE* mmahp = NULL;
	char* bpmf_file_name;
	
	pagesize = sysconf(_SC_PAGESIZE);
	
	// Calculate size of the Buffer Pool Management File for this pool. This
	// will be size of the BPMF_REC control structure and the size of two deque
	// elements arrays.
	bpmf_size = sizeof(BPMF_REC) + (2 * sizeof(BPOOL_INDEX) * rqst_capacity);
	
	// Calculate number of pages and amount left over
	pages = bpmf_size / pagesize;
	remainder = bpmf_size % pagesize;
	slop = pagesize - remainder;
	
#ifndef _MMPOOL_USE_SLOP
	alloc_capacity = rqst_capacity;
#else
	if (remainder > 0) {
		pages ++;
		
		// Allocate the slop to the two deques
		itemslop = slop / (2 * sizeof(BPOOL_INDEX));
		alloc_capacity = rqst_capacity + itemslop;
		bpmf_size = sizeof(BPMF_REC) + (2 * sizeof(BPOOL_INDEX) * alloc_capacity);
	}
#endif
	
	bpmf_file_name = mmpool_bpmf_filename(name);
	sprintf(fpath, "%s/%s", variables.data_dir, bpmf_file_name);
	mmahp = mmapfile_create(name, fpath, bpmf_size, MMA_READ_WRITE, MMF_SHARED,
		0660);
	if (mmahp != NULL) {
		// Successful memory mapped file setup. Format the buffer pool
		// management file.
		format_bpmf(mmahp, name, bp_id, max_data_size, rqst_capacity, alloc_capacity);
		
	}
	return mmahp; 	
}

static MMFOR_HANDLE* new_bpcf(
	char name[MMPOOL_MAX_POOL_NAME],	// name of the pool
	size_t max_data_size,		// max number of user data bytes one buffer will hold
	unsigned short nitems		// number of buffers to create
) {
	static char fpath[1024];
	MMFOR_HANDLE* mmfhp = NULL;
	size_t buff_size;			// total size of buffer ... header + data + slop
	off_t data_offset;
	
	data_offset = bpcf_data_offset();
	buff_size = data_offset + max_data_size;
		
	sprintf(fpath, "%s/%s", variables.data_dir, mmpool_bpcf_filename(name));
	mmfhp = mmfor_create(fpath, MMA_READ_WRITE, MMF_SHARED, 0660, buff_size, nitems);
	return mmfhp;
}

/*
 * Return offset to the first byte of user data in a BPCF_BUFFER.
 * This is independent upon the particular buffer and is a function
 * of the size of the header.
 * The result is always aligned to a 4 byte boundary.
 */
static off_t bpcf_data_offset() {
	size_t header_size;
	off_t offset;
	
	header_size = sizeof(BPCF_BUFFER_REF);
	if (header_size % 4) {
		int rem;
		
		rem = sizeof(BPCF_BUFFER_REF) % 4;
		header_size += rem;
	}
	offset = (off_t)header_size;
	return offset;
}
	
static int allocate_buffers(MMA_HANDLE* bpmfhp, MMFOR_HANDLE* bpcfhp) {
	size_t ibuff;
	void* vbuffp;
	void* vbuffp0;
	BPMF_STATS* bpmf_statsp;
	BPMF_REC* bpmf_recp;
	BPCF_BUFFER_REF buffref;
	size_t buff_size;

	bpmf_recp = (BPMF_REC*)mma_data_pointer(bpmfhp);
	bpmf_statsp = &bpmf_recp->stats;
	buff_size = bpmf_statsp->max_data_size + bpcf_data_offset();
	buffref.bp_id = bpmf_statsp->bp_id;
	buffref.buff_len = buff_size;
 	buffref.sync_word[0] = 0xA4;
 	buffref.sync_word[1] = 0xA4;
	vbuffp0 = (void*)mmfor_x2p(bpcfhp, 0);
	vbuffp = vbuffp0;
	for (ibuff = 0; ibuff < bpmf_statsp->capacity; ibuff++) {
		buffref.bpindex = ibuff;						// set this buffer's index
		memset(vbuffp, '*' , buff_size);
		memcpy(vbuffp, &buffref, sizeof(buffref));		// copy our local structure to mapped memory
		dq_abd(&bpmf_recp->dq_inpool, &ibuff);			// add buffer index to in pool deque
		vbuffp += buff_size;
	}
	return 0;	
}

static char* bpfile_full_path(const char* filename) {
	static char buff[1024];
	
	sprintf(buff, "%s/%s", variables.data_dir, filename);
	return buff;
}
static MMA_HANDLE* open_bpmf(char* pool_name) {
	return mmapfile_open(pool_name, bpfile_full_path(mmpool_bpmf_filename(pool_name)), 
		MMA_READ_WRITE, MMF_SHARED);
}

static MMFOR_HANDLE* open_bpcf(char* pool_name) {
	return mmfor_open(bpfile_full_path(mmpool_bpcf_filename(pool_name)), MMA_READ_WRITE,
		MMF_SHARED);
}

/*
 * Given a buffer index, form a buffer reference.
 */
BPCF_BUFFER_REF* mmpool_buffx2refp(BPOOL_HANDLE* bphp, BPOOL_INDEX bpx) {
	return mmfor_x2p(bphp->bpcfp, bpx);
}

/*
 * Given a buffer reference, obtain the buffer pool buffer index
 */
BPOOL_INDEX mmpool_refp2buffx(BPCF_BUFFER_REF* buff_refp) {
	return buff_refp->bpindex;
}

// Search for the specified buffer pool index. If found, pop it off the deque
// and return non-zero. If not, return zero.
static int search_deque(DQHEADER* dequep, BPOOL_INDEX bpx) {
	int nitems;
	int i;
	int x;
	
	nitems = dequep->dquse;
	for (i = 0; i < nitems; i++) {
		dq_rtd(dequep, &x);			// pop from top
		if (x == bpx) {
			return 1;				// that's our match ... return TRUE
		}
		dq_abd(dequep, &x);		// not a match .. push onto bottom
	}
	return 0;					// No match found ... return FALSE
}

#if 0
static void setup_deques(BPOOL_HANDLE* bphp) {
	DQHEADER* indqp;
	DQHEADER* outdqp;
	void* p0;
	
	indqp = &bphp->bpmf_recp->dq_inpool;
	outdqp = &bphp->bpmf_recp->dq_outpool;
	p0 = mma_data_pointer(bphp->bpmfp);
	indqp->dqbuff = p0 + bphp->bpmf_recp->indqbuffx;
	outdqp->dqbuff = p0 + bphp->bpmf_recp->outdqbuffx;
}
#endif

/*
 * Lock access to the buffer pool by locking the BPMF structure.
 * We do a write to to grab exclusive access.
 */
static void lock_pool(BPOOL_HANDLE* bphp) {
	if (bphp->bpmfp != NULL) {
		mma_lock_atom_write(bphp->bpmfp);
	}
}

static void unlock_pool(BPOOL_HANDLE* bphp) {
	if (bphp->bpmfp != NULL) {
		mma_unlock_atom(bphp->bpmfp);
	}
}
