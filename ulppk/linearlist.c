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

/*
 * linearlist.c
 *
 *  Created on: Nov 13, 2009
 *      Author: R_Garvey
 */

/**
 * @file linearlist.c
 *
 * @brief Implementation of a linear list of memory mapped records.
 *
 * A linear list has a fixed capacity determined at creation time.
 * Active records begin at index 0 and are kept contiguous. Inactive
 * records are maintained at high indices.
 *
 * See linearlist.h for some general comments.
 */
#include <string.h>
#include <linearlist.h>
#include <ulppk_log.h>

/**
 * Create a linear list file. Set up the header record at mmfor record index 0.
 * @param filepath Path to the memory mapped I/O file
 * @param mode see mmatom.h for a description of MMA_ACCESS_MODES
 * @param flags see mmatom.h for a description of MMA_MAP_FLAGS
 * @param permissions see man page open(2)
 * @param rec_size Size of the records.
 * @param nrecs Record capacity of the linear list.
 * @return Pointer to a MMFOR_HANDLE
 */
MMFOR_HANDLE* linlist_create(char* filepath,  MMA_ACCESS_MODES mode,
		MMA_MAP_FLAGS flags, int permissions, size_t rec_size, size_t nrecs) {

	size_t total_recs;
	MMFOR_HANDLE* mmforhp;
	LINEAR_LIST_HEADER* headerp;

	total_recs = nrecs + 1;		// reserve index 0 for control record
	mmforhp = mmfor_create(filepath, mode, flags, permissions, rec_size, total_recs);

	// Now write the linear list header to record 0.
	headerp = (LINEAR_LIST_HEADER*)mmfor_x2p(mmforhp, 0);
	headerp->nextx = 1;
	headerp->capacity = nrecs;
	headerp->rec_size = rec_size;

	return mmforhp;

}

/**
 * Open a previously created linear list file.
 *
 * @param filepath Path to the memory mapped I/O file
 * @param mode see mmatom.h for a description of MMA_ACCESS_MODES
 * @param flags see mmatom.h for a description of MMA_MAP_FLAGS
 * @return mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 */
MMFOR_HANDLE* linlist_open(char* filepath, MMA_ACCESS_MODES mode, MMA_MAP_FLAGS flags) {
	return mmfor_open(filepath, mode, flags);
}

/**
 * Close a previously opened liner list.
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @return 0 on success, non-zero on failure
 */
int linlist_close(MMFOR_HANDLE* mmforhp) {
	return mmfor_close(mmforhp);
}

/**
 * Translate a zero based user record index
 * to a pointer into the memory mapped file region.
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @param x Index to translate
 * @return Pointer to memory mapped record.
 */
void* linlist_x2p(MMFOR_HANDLE* mmforhp, size_t x) {
	int x1;

	// First user record is at index 1.
	x1 = x + 1;
	return mmfor_x2p(mmforhp, x1);
}

/**
 * Translate an address to a memory mapped file record to
 * a zero based record index.
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @param p Pointer to translate
 * @return Index of memory mapped record.
 */
size_t linlist_p2x(MMFOR_HANDLE* mmforhp, void*p) {
	int x0;

	// First user record is at index 1, but user
	// wants zero based indexing.
	x0 = mmfor_p2x(mmforhp, p);
	return (x0 - 1);
}

/**
 * Return a count of user records. (Does not count the header
 * record.) This count is the number of records the file can
 * contain, or its capacity.
 */
size_t linlist_capacity(MMFOR_HANDLE* mmforhp) {
	int nrecs;

	nrecs = mmfor_record_count(mmforhp) - 1;
	return nrecs;

}

/**
 * This function returns the count of records actually
 * active in the file. The file can accept
 *
 * linlist_capacity(mmforhp) - linlist_length(mmforhp)
 *
 * additional records. Reflects current size of list.
 *
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @return Number of active records in the linear list file.
 */
size_t linlist_length(MMFOR_HANDLE* mmforhp) {
	LINEAR_LIST_HEADER* headerp;

	headerp = mmfor_x2p(mmforhp, 0);
	return (headerp->nextx - 1);
}
/**
 * Given data at userp, add a new record to the linear
 * list that contains the user data. userp does NOT usually
 * point to the memory mapped linear list file region. It is a
 * buffer provided by the calling application and may or
 * may not itself be a memory mapped object.
 *
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @param userp Pointer to data to be added (copied) to the next available linear list
 * 	record.
 * @return Pointer to the memory mapped record on success,
 * NULL if the list is full.
 */
void* linlist_add_record(MMFOR_HANDLE* mmforhp, void* userp) {
	int x;
	LINEAR_LIST_HEADER* headerp;
	void* recp;

	// Lock access
	linlist_list_lock(mmforhp);

	// Get the header record
	headerp = (LINEAR_LIST_HEADER*)mmfor_x2p(mmforhp, 0);

	if (headerp->nextx > headerp->capacity) {
		recp = NULL;
	} else {
		recp = mmfor_x2p(mmforhp, headerp->nextx);
		memcpy(recp, userp, headerp->rec_size);
		headerp->nextx++;
	}
	// Unlock access
	linlist_list_unlock(mmforhp);
	return recp;
}

/**
 * Delete the record pointed to by p from the linear list file.
 *
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @param p Pointer to the record to delete.
 * @return 0 on success, 1 on failure
 */
int linlist_delete_recordp(MMFOR_HANDLE* mmforhp, void* p) {
	long del_recx;

	del_recx = linlist_p2x(mmforhp, p);
	return linlist_delete_recordx(mmforhp, del_recx);
}

/**
 * Delete the record at file index x.
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @param x Index of the record to delete.
 * @return 0 on success, 1 on failure
 */
int linlist_delete_recordx(MMFOR_HANDLE* mmforhp, size_t x) {
	LINEAR_LIST_HEADER* headerp;
	size_t fence;
	void* pfence;
	void* pdelrec;
	int retval = 0;

	// Lock access to table
	linlist_list_lock(mmforhp);

	headerp = (LINEAR_LIST_HEADER*)mmfor_x2p(mmforhp, 0);
	fence = headerp->nextx - 2;		// -1 to get high index, another -1 to account for header
	if (fence < 0) {
		// List is empty. Can't delete
		ULPPK_LOG(ULPPK_LOG_ERROR, "list is empty ... cannot delete");
		retval = 1;
	} else if (x > fence) {
		// Invalid record index
		ULPPK_LOG(ULPPK_LOG_ERROR, "Invalid record index: recordx = %d fence = %d",
				x, fence);
		retval = 1;
	} else if (fence == x) {
		// Deleting record at the fence. Just decrement nextx
		headerp->nextx -= 1;
		ULPPK_LOG(ULPPK_LOG_DEBUG, "Deleted fence record [%d]", x);
	} else {
		// Copy record at the fence to record index x and dec nextx
		pdelrec = linlist_x2p(mmforhp, x);
		pfence = linlist_x2p(mmforhp, fence);
		memcpy(pdelrec, pfence, headerp->rec_size);
		ULPPK_LOG(ULPPK_LOG_DEBUG, "Deleted recordx [%d] moved record [%d]",
				x, fence);

		headerp->nextx -= 1;
	}
	linlist_list_unlock(mmforhp);
	return retval;
}

/**
 * Lock access to the list.
 *
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @return 0 on success, non-zero on failure
 */
int linlist_list_lock(MMFOR_HANDLE* mmforhp) {
#ifdef _LINLIST_DEBUG_LOCKS
	ULPPK_LOG(ULPPK_LOG_DEBUG, "Locking QRL file: path = %s",
			mma_get_disk_file_path(mmforhp->mmahp));
#endif
	return mma_lock_atom_write(mmforhp->mmahp);
}

/**
 * Unlock access to the list.
 * @param mmforhp Pointer to a memory mapped file of records (MMFOR) handle
 * @return 0 on success, non-zero on failure.
 */
int linlist_list_unlock(MMFOR_HANDLE* mmforhp) {
#ifdef _LINLIST_DEBUG_LOCKS
	ULPPK_LOG(ULPPK_LOG_DEBUG, "Unlocking QRL file: path = %s",
			mma_get_disk_file_path(mmforhp->mmahp));
#endif

	return mma_unlock_atom(mmforhp->mmahp);
}
