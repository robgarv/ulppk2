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
 * linearlist.h
 *
 *  Created on: Nov 13, 2009
 *      Author: R_Garvey
 */

/**
 * @file linearlist.h
 *
 * @brief Generalized linear list functions.
 *
 *	A linear list is a memory mapped file of records which are
 *	managed in a fashion that support addition and deletion. It
 *	is presumed that the records are of equal size.
 *
 *      A linear list is implemented on top of an mmfor file.
 *      The first record in the mmfor file (index 0) is the control record.
 *
 *      Addition to the list is accomplished by writing to the record
 *      at index nextx and incrementing nextx. Deletion is accomplished
 *      by copying the record at nextx-1 to the record being deleted
 *      and decrementing nextx.
 *
 *      Consequently, deletion will cause ordering of the records
 *      in the list to change (unless the record being deleted is at the
 *      end of the list.)
 */

#ifndef LINEARLIST_H_
#define LINEARLIST_H_

#include <mmfor.h>

/**
 * Linear list header record. This is the
 * first record in a linear list file, and is
 * used to manage add and delete operations.
 */
typedef struct {
	int nextx;       ///< next available record index
	int capacity;    ///< number of user records this list can store
	size_t rec_size; ///< size of the records
} LINEAR_LIST_HEADER ;

/**
 *
 * This can be used by union types to distinguish between data
 * records and the header record.
 */
typedef enum {
	LINLIST_RECTYPE_HEADER = 1,		///< Header record
	LINLIST_RECTYPE_DATA			///< Data record
} LINEAR_LIST_RECTYPE;

#ifdef __cplusplus
extern "C" {
#endif
	void* linlist_add_record(MMFOR_HANDLE* mmforhp, void* userp);
	size_t linlist_capacity(MMFOR_HANDLE* mmforhp);
	int linlist_close(MMFOR_HANDLE* mmforhp);
	MMFOR_HANDLE* linlist_create(char* filepath,  MMA_ACCESS_MODES mode,
			MMA_MAP_FLAGS flags, int permissions, size_t rec_size, size_t nrecs);
	size_t linlist_length(MMFOR_HANDLE* mmforhp);
	int linlist_list_lock(MMFOR_HANDLE* mmforhp);
	int linlist_list_unlock(MMFOR_HANDLE* mmforhp);
	int linlist_delete_recordx(MMFOR_HANDLE* mmforhp, size_t x);
	int linlist_delete_recordp(MMFOR_HANDLE* mmforhp, void* p);
	MMFOR_HANDLE* linlist_open(char* filepath, MMA_ACCESS_MODES mode, MMA_MAP_FLAGS flags);
	size_t linlist_p2x(MMFOR_HANDLE* mmforhp, void*p);
	void* linlist_x2p(MMFOR_HANDLE* mmforhp, size_t x);

#ifdef __cplusplus
}
#endif

#endif /* LINEARLIST_H_ */
