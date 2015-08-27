
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
#ifndef MMATOM_H_
#define MMATOM_H_

#include <sys/mman.h>
#include <limits.h>

/**
 * @file mmatom.h
 * @brief Memory mapped "atom" definitions.
 * 
 * "Atom" in this context signifies a logical pairing of a file description
 * and a memory mapped I/O region. It is called an "Atom" because it is the
 * simplest unit
 */
 
 /**
  * Access modes ... also known as protection modes. Not to be confused
  * with file permissions, these govern access to the memory mapped space.
  */
 typedef enum {
 	MMA_NONE = 0,				///< None
 	MMA_WRITE,					///< Write access
 	MMA_READ,					///< Read access
 	MMA_READ_WRITE,				///< Read/write access
 	MMA_MAX = MMA_READ_WRITE
 } MMA_ACCESS_MODES;
 
 /**
  * Underlying object type.
  * (Initially only disk files are supported.)
  */
 typedef enum {
 	MMT_NONE,
 	MMT_FILE
 } MMA_OBJECT_TYPES;

 /**
  * Sharing flags.
  */
typedef enum {
	MMF_SHARED = 1,				///< Mapped region sharable between processes
	MMF_PRIVATE,				///< Mapped region only accessible by the creating process.
	MMF_MAX = MMF_PRIVATE
} MMA_MAP_FLAGS;

/**
 * Disk file access reference
 */
 typedef struct {
 	char* str_pathname;			///< Pathname of backing file
 	int oflags;					///< See man page open(2)
 	int mode;					///< File permissions bit map (see man page open(2)
 	int filedes;				///< File descriptor
 	long len;					///< length in bytes
 } MMA_DISK_FILE_REF;
 
 /**
  * Mapped memory region reference
  */
 typedef struct {
 	void* addr;			///< address hint. Normally 0
 	size_t len;			///< length of the memory mapped region
 	int prot;			///< protection mode bits
 	int flags;			///< flag bits (MAP_SHARED, MAP_PRIVATE, MAP_FIXED)
 	int filedes;		///< file descriptor of underlying object
 	off_t off;			///< offset
 	void* pa;			///< returned from mmap ... pointer to memory mapped area
 } MMA_MEMMAP_REF;

#define MMA_MAX_TAG_LEN 256

/**
 * Memory mapped atom handle. This structure is used to reference
 * and access the memory mapped atom.
 */
typedef struct {
	char tag[MMA_MAX_TAG_LEN];		///< a logical name for the atom
	MMA_ACCESS_MODES access_mode;	///< Access (read and or write)
	MMA_OBJECT_TYPES obj_type;		///< Backing object type
	union {
		MMA_DISK_FILE_REF* df_refp;	///< Pointer to backing disk file reference structure
		void* void_refp;			///< Pointer to TBD backing object reference
	} u;
	MMA_MEMMAP_REF mm_ref;			///< pointer to memorary mapped region reference structure
} MMA_HANDLE;


/*
 * Function prototypes
 */

#ifdef __cplusplus
extern "C" {
#endif
 
 char* mma_strerror(char* buff, size_t len);
/*
 * Create a new MM atom. 
 */
MMA_HANDLE* mma_create(
	char* tag,					// just a logical identifier string for the atom
	MMA_OBJECT_TYPES obj_type,
	MMA_ACCESS_MODES access_mode,
	void* reference,			// object reference pointer
	size_t len,
	MMA_MAP_FLAGS flags
);

/*
 * Construct a MMA_DISK_FILE_REF
 */
 MMA_DISK_FILE_REF* mma_new_disk_file_ref(
 	char* str_pathname,				// full path name of the disk file
 	int oflags, 					// flags as defined by open function
 	int mode,						// zero or a permission mode bit map like chmod (create only)
 	size_t len						// required length of file (create only)
 );

/*
 * Convert access mode and ugokey to compatible file mode (see chmod)
 * ugokey is a string of the form "u", "ug", "ugo", "g", etc. Order
 * is not important. u => user, g => group, o => other.
 */
int mma_file_mode(MMA_ACCESS_MODES access_mode, char* ugokey);

/*
 * Retrieve data pointer from a handle
 */
void* mma_data_pointer(MMA_HANDLE* mmahp); 

/*
 * Lock the atom. This locks the entire range ob bytes in the memory
 * mapped region. Note that atoms implement mandatory locking.
 */
int mma_lock_atom_read(MMA_HANDLE* mmahp);

/*
 * Lock the atom for write. This locks the entire range of bytes in the memory
 * mapped region. The calling process will wait on the lock if necessary.
 */
int mma_lock_atom_write(MMA_HANDLE* mmahp);

/*
 * Unlock the atom.
 */
int mma_unlock_atom(MMA_HANDLE* mmahp);

/*
 * Access method ... get a disk file atom's file path. Returns NULL if atom is
 * not open or if not a disk file based memory mapped atom.
 */
char* mma_get_disk_file_path(MMA_HANDLE* mmahp);

/*
 * Destroy a memory mapped atom.
 * Unmap a memorary mapped atom's memory region. Any
 * pointers referring to addresses contained by the memory mapped
 * atom will be invalid after this call. Memory consumed by
 * the handle is freed.
 */
int mma_destroy_atom(MMA_HANDLE* mmahp);

#ifdef __cplusplus
}
#endif

/*
 * Error codes
 */
 
 #define MMA_ERR_PATH_MAX 1
 #define MMA_ERR_FILE_OPEN 2
 #define MMA_ERR_NULL_IO_REF 3
 #define MMA_ERR_UNSUPPORTED_TYPE 4
 #define MMA_ERR_MAP_FAILED 5
 #define MMA_ERR_FILE_SET_SIZE 6
 #define MMA_FILE_MAP_SIZE 7
 #define MMA_ERR_FILE_STATUS 8
 #define MMA_MANDATORY_LOCK_FAIL 9
 #define MMA_INVALID_FILENAME 10
 
#endif /*MMATOM_H_*/
