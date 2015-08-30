
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mmatom.h>

/**
 * @file mmapfile.c
 *
 * @brief Extends a memory mapped atom construct to an actual memory mapped file.
 *
 * See mmatom.c and mmatom.h.
 */
/**
 * @brief Create a memory mapped file.
 *
 * Creates a new disk file reference structure for a memory mapped atom (mmatom),
 * and creates the atom using the disk file reference as the underlying object.
 *
 * @param tag Just an arbitrary name string
 * @param filepath Path to the file.
 * @param len Length of the atom backing data structure (file) in bytes
 * @param mode See mmatom.h. Access
 * @param flags See mmatom.h.
 * @param permissions Permissions as defined by open(2)
 */
MMA_HANDLE* mmapfile_create(char* tag,
 	char* filepath, size_t len, MMA_ACCESS_MODES mode, 
 	MMA_MAP_FLAGS flags, int permissions) {
 	
 	MMA_DISK_FILE_REF* dfrefp;
 	MMA_HANDLE* mmahp = NULL;
 	
 	// Create a new disk file reference. 
 	dfrefp = mma_new_disk_file_ref(filepath, O_CREAT | O_RDWR,
 		permissions, len);
 	 	
	if (dfrefp != NULL) {
		// Create a new memory mapped atom using the file as
		// the underlying object.
		mmahp = mma_create(tag, MMT_FILE, mode, dfrefp, len, flags);
	}
	return mmahp;
 }

/**
 * @brief Open a previously created memory mapped file.
 *
 * @param tag Just an arbitrary name string for the atom
 * @param filepath Path to the file.
 * @param mode See mmatom.h. Access
 * @param flags See mmatom.h.
 */
MMA_HANDLE* mmapfile_open(char* tag, char* filepath, MMA_ACCESS_MODES mode,
 	MMA_MAP_FLAGS flags) {
	MMA_DISK_FILE_REF* dfrefp = NULL;
 	MMA_HANDLE* mmahp = NULL;
 	int oflags;
  	
 	// Set oflags based on mode
 	switch (mode) {
 		case MMA_READ:
 			oflags = O_RDWR;
 			break;
 		case MMA_WRITE:
 			oflags = O_WRONLY;
 			break;
  		case MMA_READ_WRITE:
  		default:
 			oflags = O_RDWR;
 			break;
 	}
 	
 	// Create a new disk file reference. 
 	dfrefp = mma_new_disk_file_ref(filepath, O_RDWR,
 		0, 0);
 	 	
	if (dfrefp != NULL) {
		// Create a new memory mapped atom using the file as
		// the underlying object.
		mmahp = mma_create(tag, MMT_FILE, mode, dfrefp, dfrefp->len, flags);
	}
	return mmahp;
 		
}
/**
 * @brief Close a memory mapped file explicitly.
 *
 * Note this will cause any record locks owned by the process to be cleared ... even if it has
 * other references (file descriptors) to the underlying memory mapped atom.
 *
 * Note also this unmaps the memory mapped atom's address range.
 *
 * @param mmahp Pointer to MMA_HANDLE (memory mapped atom handle) of file to close.
 * @return Always returns 0 to indicate success.
 */
int mmapfile_close(MMA_HANDLE* mmahp) {
	mma_destroy_atom(mmahp);
	return 0;
}

/**
 * @brief Return the full path name of a memory mapped file atom's disk file.
 *
 * @param mmahp Pointer to MMA_HANDLE (memory mapped atom handle) of file to close.
 * @return Full path of the actual disk file.
 */
char* mmapfile_file_path(MMA_HANDLE* mmahp) {
	return mma_get_disk_file_path(mmahp);
}

/**
 * Given a pointer into the memory mapped file's memory region,
 * calculate the offset into the disk file.
 *
 * @param mmahp Pointer to MMA_HANDLE (memory mapped atom handle) of file to close.
 * @param p Pointer to an item in the memory mapped region.
 * @return offset into disk file of the item at pointer p
 */
size_t mmapfile_p2fileoffset(MMA_HANDLE* mmahp, void* p) {
	size_t offset;

	offset = (p - mma_data_pointer(mmahp));
	return offset;
}
