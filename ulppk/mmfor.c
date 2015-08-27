
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
 * @file mmfor.c
 *
 * @brief Memory mapped file of records (array) access.
 *
 * A memory mapped file of records can be treated similarly
 * to an array. That is, it can be accessed either through
 * pointer or index.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <mmfor.h>

static int lock_cntrl(MMFOR_HANDLE* mmforhp, int cmd, int type, size_t start);

/**
 * @brief Create a new memory mapped file of records.
 *
 * @param filepath Pathname of the file to be created.
 * @param mode Memory mapped atom access mode. (see mmatom.h)
 * @param flags Shared/private (see mmatom.h)
 * @param permissions access permissions (see man open(2))
 * @param rec_size Size of a record in bytes
 * @param nrecs Number of records file must store
 * @return Returns pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 */
 MMFOR_HANDLE* mmfor_create(char* filepath,  MMA_ACCESS_MODES mode,
 	MMA_MAP_FLAGS flags, int permissions, size_t rec_size, size_t nrecs) {

	MMFOR_HANDLE* mmforhp;
	MMFOR_HEADER* mmforhdp;
	size_t len;
	
	len = (rec_size * nrecs) + sizeof(MMFOR_HEADER);
	
	mmforhp = (MMFOR_HANDLE*)calloc(1, sizeof(MMFOR_HANDLE));
	mmforhp->mmahp = mmapfile_create(NULL, filepath, len, mode, flags,
		permissions);
	if (mmforhp->mmahp != NULL) {
		mmforhdp = (MMFOR_HEADER*)mma_data_pointer(mmforhp->mmahp);
		mmforhdp->rec_size = rec_size;
		mmforhdp->file_size = len;
		mmforhdp->nrecs = nrecs;
		mmforhp->rec_size = rec_size;
		mmforhp->nrecs = nrecs;
	} else {
		free(mmforhp);
		mmforhp = NULL;
	}

 	return mmforhp;
 }
 /**
  * @brief Intialize/open an existing memory mapped file of records.
  *
  * @param filepath Pathname of the file to be created.
  * @param mode Memory mapped atom access mode. (see mmatom.h)
  * @param flags Shared/private (see mmatom.h)
  * @return Returns pointer to a MMFOR_HANDLE representing the memory
  * 	mapped file and region.
  */
MMFOR_HANDLE* mmfor_open(char* filepath, MMA_ACCESS_MODES mode, MMA_MAP_FLAGS flags) {
	MMFOR_HANDLE* mmforhp;
	MMFOR_HEADER* mmforhdp;
	 		
	mmforhp = (MMFOR_HANDLE*)calloc(1, sizeof(MMFOR_HANDLE));
	mmforhp->mmahp = mmapfile_open(NULL, filepath, mode, flags);
	if (mmforhp->mmahp != NULL) {
		mmforhdp = (MMFOR_HEADER*)mma_data_pointer(mmforhp->mmahp);
		mmforhp->nrecs = mmforhdp->nrecs;
		mmforhp->rec_size = mmforhdp->rec_size;
	} else {
		free(mmforhp);
		mmforhp = NULL;
	}
	return mmforhp;
 }
 
/**
 * @brief Close a file of records. This will unmap the
 * memory mapped region.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @return 0 on success, non-zero otherwise.
 */
 int mmfor_close(MMFOR_HANDLE* mmforhp) {
	int status = 0;
 	status = mma_destroy_atom(mmforhp->mmahp);
 	free(mmforhp);
 	return status;
 }

 	
/**
 * @brief Given a record index, calcuate pointer into the memory mapped file.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param x Zero based record index.
 * @return Pointer to the record in the memory mapped region.
 */
void* mmfor_x2p(MMFOR_HANDLE* mmforhp, size_t x) {
	void* p0;
	void* px;
	MMFOR_HEADER* mmforhdp;
	
	mmforhdp = (MMFOR_HEADER*)mma_data_pointer(mmforhp->mmahp);
	p0 = ((void*)mmforhdp) + sizeof(MMFOR_HEADER);
	px = p0 + (mmforhdp->rec_size) * x;
	return px;
}

/**
 * @brief Given a pointer, calculate the record index.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param p Pointer to the record in the memory mapped region.
 * @return Zero based record index.
 */
size_t mmfor_p2x(MMFOR_HANDLE* mmforhp, void* p) {
	void* p0;
	size_t nbytes;
	size_t x;
	MMFOR_HEADER* mmforhdp;
	
	mmforhdp = (MMFOR_HEADER*)mma_data_pointer(mmforhp->mmahp);
	p0 = (void*)(mmforhdp + 1);
	nbytes = (size_t)(p - p0);
	x = nbytes / mmforhdp->rec_size;
	return x;
}

/**
 * @brief Given a memory mapped file handle and a record index, lock the record
 * for reading. Multiple processes can access during a read lock, but a process
 * atttempting a write lock will be blocked until read locks are released. A
 * write lock will cause this function to block.
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param x Zero based record index.
 * @return 0 on success. Non-zero on failure.
 */
int mmfor_lock_record_x_read(MMFOR_HANDLE* mmforhp, size_t x) {
	size_t file_offset;
	void* p;

	p = mmfor_x2p(mmforhp, x);
	return mmfor_lock_record_p_read(mmforhp,p);
}

/**
 * @brief Given a memory mapped file handle and a pointer to an address in the
 * mapped memory region, lock the record for reading. Multiple processes can
 * access during a read lock, but a process atttempting a write lock will
 * be blocked until read locks are released. A write lock will cause
 * this function to block.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param p Pointer to the record in the memory mapped region.
 * @return 0 on succes, non-zero on failure.
 */
int mmfor_lock_record_p_read(MMFOR_HANDLE* mmforhp, void* p) {
	size_t file_offset;

	file_offset = mmapfile_p2fileoffset(mmforhp->mmahp, p);
	return lock_cntrl(mmforhp, F_SETLKW, F_RDLCK, file_offset);
}

/**
 * @brief Given a memory mapped file handle and a record index, lock the record
 * for writing. Multiple processes can access during a read lock, but a process
 * atttempting a write lock will be blocked until read locks are released. A
 * lock will cause this function to block until read or write access is released.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param x Zero based record index.
 * @return 0 on success. Non-zero on failure.
 */
int mmfor_lock_record_x_write(MMFOR_HANDLE* mmforhp, size_t x) {
	void* p;

	p = mmfor_x2p(mmforhp, x);
	return mmfor_lock_record_p_read(mmforhp,p);
}

/**
 * @brief Given a memory mapped file handle and a pointer to an address in the
 * mapped memory region, lock the record for writing. Multiple processes can
 * access during a read lock, but a process atttempting a write lock will
 * be blocked until read locks are released. A lock will cause
 * this function to block until read or write access is released.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param p Pointer to the record in the memory mapped region.
 * @return 0 on succes, non-zero on failure.
 */
int mmfor_lock_record_p_write(MMFOR_HANDLE* mmforhp, void* p) {
	size_t file_offset;

	file_offset = mmapfile_p2fileoffset(mmforhp->mmahp, p);
	return lock_cntrl(mmforhp, F_SETLKW, F_WRLCK, file_offset);
}

/**
 * @brief Given a memory mapped file handle and a record index, unlock
 * access to the record.
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param x Zero based record index.
 * @return 0 on success. Non-zero on failure.
 */
int mmfor_unlock_record_x(MMFOR_HANDLE* mmforhp, size_t x) {
	void *p;

	p = mmfor_x2p(mmforhp, x);
	return mmfor_unlock_record_p(mmforhp, p);
}

/**
 * @brief Given a memory mapped file handle and a pointer to a record
 * in the memory mapped region, unlock access to the record.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @param p Pointer to the record in the memory mapped region.
 * @return 0 on succes, non-zero on failure.
 */
int mmfor_unlock_record_p(MMFOR_HANDLE* mmforhp, void* p) {
	size_t file_offset;

	file_offset = mmapfile_p2fileoffset(mmforhp->mmahp, p);
	return lock_cntrl(mmforhp, F_SETLK, F_UNLCK, file_offset);

}

/**
 * @brief Return the number of records in this file.
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * @return Record capacity of the memory mapped file of records.
 */
size_t mmfor_record_count(MMFOR_HANDLE* mmforhp) {
	return mmforhp->nrecs;
}

/**
 * @brief Return size of the user area of the record
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * 	@return Size of the record.
 */
size_t mmfor_record_size(MMFOR_HANDLE* mmforhp){
	return mmforhp->rec_size;
}

static int lock_cntrl(MMFOR_HANDLE* mmforhp, int cmd, int type, size_t start) {
	struct flock lock;

	lock.l_type = type;
	lock.l_start = start;
	lock.l_whence = SEEK_SET;
	lock.l_len = mmforhp->rec_size;

	return (fcntl(mmforhp->mmahp->mm_ref.filedes, cmd, &lock));
}

/**
 * @brief Lock the entire file for read access. (Uses mma_lock_atom_read).
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * 	@return 0 on success, non-zero if failure.
 */
int mmfor_lock_file_read(MMFOR_HANDLE* mmforhp) {
	return mma_lock_atom_read(mmforhp->mmahp);
}

/**
 *
 * @brief Lock the entire file for write access. (Uses mma_lock_atom_write).
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * 	@return 0 on success, non-zero if failure.
 */
int mmfor_lock_file_write(MMFOR_HANDLE* mmforhp) {
	return mma_lock_atom_write(mmforhp->mmahp);
}

/**
 * @brief Release lock access to the file. (Uses mma_unlock_atom).
 *
 * @param mmforhp pointer to a MMFOR_HANDLE representing the memory
 * 	mapped file and region.
 * 	@return 0 on success, non-zero if failure.
 */
int mmfor_unlock_file(MMFOR_HANDLE* mmforhp) {
	return mma_unlock_atom(mmforhp->mmahp);
}
