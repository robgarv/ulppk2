
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
#ifndef MMFOR_H
#define MMFOR_H

/**
 * @file mmfor.h
 *
 * @brief mmfor -- Memory Mapped File Of Records
 * 
 * A library module for accessing files organized
 * as arrays of structures.
 * 
 * This is built on the functions and types of mmaptom.c
 * and mmapfile.c
 *
 */
#include <mmapfile.h>

/**
 * A specialization of the MMA_HANDLE structure with additional
 * info necessary to support arrays of records.
 */
typedef struct _mmfor_handle {
	MMA_HANDLE* mmahp;		///< Pointer to memory mapped atom handle structure
	size_t rec_size;     	///< size of the records in bytes
	size_t nrecs;        	///< number of records in the file
} MMFOR_HANDLE;

/**
 * The first thing written to any of these files is the header.
 */
typedef struct _mmfor_header {
	size_t rec_size;		///< size of the records (bytes)
	size_t file_size;		///< size of the file (bytes)
	size_t nrecs;			///< number of records
} MMFOR_HEADER;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Create a new memory mapped file of records.
 */
 MMFOR_HANDLE* mmfor_create(char* filepath,  MMA_ACCESS_MODES mode,
 	MMA_MAP_FLAGS flags, int permissions, size_t rec_size, size_t nrecs);
 /*
  * Intialize an existing memory mapped file of records.
  */
MMFOR_HANDLE* mmfor_open(char* filepath, MMA_ACCESS_MODES mode, MMA_MAP_FLAGS flags); 
/*
 * Close a file of records
 */
 int mmfor_close(MMFOR_HANDLE* mmforhp);	
/*
 * Given a record index, calcuate pointer into the memory mapped file.
 */
void* mmfor_x2p(MMFOR_HANDLE* mmforhp, size_t x);

/*
 * Given a pointer, calculate the record index.
 */
size_t mmfor_p2x(MMFOR_HANDLE* mmforhp, void* p);

/*
 * Return the number of records in this file.
 */
size_t mmfor_record_count(MMFOR_HANDLE* mmforhp);
 
/*
 * Return size of the user area of the record
 */
size_t mmfor_record_size(MMFOR_HANDLE* mmforhp);

/*
 * Given a memory mapped file handle and a record index, lock the record
 * for reading. Multiple processes can access during a read lock, but a process
 * atttempting a write lock will be blocked until read locks are released. A
 * write lock will cause this function to block.
 */
int mmfor_lock_record_x_read(MMFOR_HANDLE* mmforhp, size_t x);

/*
 * Given a memory mapped file handle and a pointer to an address in the
 * mapped memory region, lock the record for reading. Multiple processes can
 * access during a read lock, but a process atttempting a write lock will
 * be blocked until read locks are released. A write lock will cause
 * this function to block.
 */
int mmfor_lock_record_p_read(MMFOR_HANDLE* mmforhp, void* p);

/*
 * Given a memory mapped file handle and a record index, lock the record
 * for writing. Multiple processes can access during a read lock, but a process
 * atttempting a write lock will be blocked until read locks are released. A
 * lock will cause this function to block until read or write access is released.
 */
int mmfor_lock_record_x_write(MMFOR_HANDLE* mmforhp, size_t x);

/*
 * Given a memory mapped file handle and a pointer to an address in the
 * mapped memory region, lock the record for writing. Multiple processes can
 * access during a read lock, but a process atttempting a write lock will
 * be blocked until read locks are released. A lock will cause
 * this function to block until read or write access is released.
 */
int mmfor_lock_record_p_write(MMFOR_HANDLE* mmforhp, void* p);

/*
 * Lock the entire file for read access. (Uses mma_lock_atom_read).
 */
int mmfor_lock_file_read(MMFOR_HANDLE* mmforhp);

/* Lock the entire file for write access. (Uses mma_lock_atom_write).
/*
 */
int mmfor_lock_file_write(MMFOR_HANDLE* mmforhp);

/*
 * Release lock access to the file. (Uses mma_unlock_atom).
 */
int mmfor_unlock_file(MMFOR_HANDLE* mmforhp);

/* Given a memory mapped file handle and a record index, unlock
 * access to the record.
 */
int mmfor_unlock_record_x(MMFOR_HANDLE* mmforhp, size_t x);

/*
 * Given a memory mapped file handle and a pointer to a record
 * in the memory mapped region, unlock access to the record.
 */
int mmfor_unlock_record_p(MMFOR_HANDLE* mmforhp, void* p);

#ifdef __cplusplus
}
#endif

#endif /*MMFOR_H*/
