
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

/**
 * @file mmatom.c
 *
 * @brief Implementation of a "memory mapped atom"
 *
 * At it's most basic level, memory mapped I/O involves
 * accessing memory that has been designated as a memory
 * mapped I/O region. Memory writes cause output to a device.
 * Memory reads cause input from a device.
 *
 * This module provides the most fundamental building blocks
 * for setting up and accessing memory mapped regions. More
 * elaborate constructs build on this foundation.
 *
 * The utlity mmatomx.c provides provides facilities for creating
 * and dumping memory mapped atoms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <mmatom.h>

int mma_error = 0;
int mma_os_error = 0;

/*
 * Prototypes for forward references
 */
 
static MMA_HANDLE* create_atom(int filedes,
 	MMA_ACCESS_MODES access_mode, size_t len, 
 	MMA_OBJECT_TYPES obj_type,MMA_MAP_FLAGS flags);

static int set_mandatory_locking(int filedes);

static int lock_cntrl(MMA_HANDLE* mmahp, int cmd, int type);
 	
 /**
  * @brief print error messages to a string buffer
  *
  * Translates mmatom error codes to strings and
  * writes to the given buffer.
  *
  * @param buff buffer to receive the error string
  * @param len max characters to write to buff.
  * @return a pointer to the buffer.
  */
char* mma_strerror(char* buff, size_t len) {
 	char* str_os_error;
 	char* str_mma_error;
 	int count = 0;
 	char* lbuff;
 	
 	static char* error_msgs[] = {
 		"No error",			// 0
 		"Specified Path Exceeds PATH_MAX", //1
 		"File Open Error",	// 2
 		"Null IO Reference Structure Pointer",
 		"Unsupported Underlying Object Type",
 		"mmap Call Failed",
 		"Error Setting Size of Object during MM Atom Creation",
 		"Requested map size exceeds underlying disk file size",
 		"Error obtaining memory mapped file's file status",
 		"Error setting mandatory lock for memory mapped file",
		"Illegal file name. Possible NULL pointer to char"
 	};
 	
 	memset(buff, 0, len);
 	str_os_error = strerror(mma_os_error);
 	str_mma_error = error_msgs[mma_error];
 	count = strlen(str_os_error) + strlen(str_mma_error) + 3;
 	lbuff = (char*)calloc(count, 1);
 	sprintf(lbuff, "%s\n%s\n", str_mma_error, str_os_error);
 	if (len < count) {
 		strncpy(buff, lbuff, len-1);
 	} else {
 		strcpy(buff, lbuff);
 	}
 	free(lbuff);
 	return buff;
}
/**
 * @brief Construct a MMA_DISK_FILE_REF. Returns NULL on error.
 *
 * A MMA_DISK_FILE_REF contains information necessary to acess
 * files used to back memory mapped I/O operations.
 *
 * If an error is encountered, writes an error code to mma_error.
 *
 * @param str_pathname Full path name of the disk file
 * @param oflags Flags as defined by the open(2) function. (Use O_CREAT to
 * 	create a new file, for example.)
 * @param mode Zero or a permission mode bit map like chmod (create flag only)
 * @param len Required length of the file in bytes (create flag only)
 * @return Pointer to a MMA_DISK_FILE_REF structure or NULL on error.
 */
 MMA_DISK_FILE_REF* mma_new_disk_file_ref(
 	char* str_pathname,			// full path name of the disk file
 	int oflags, 				// flags as defined by open function
 	int mode,					// zero or a permission mode bit map like chmod (create only)
 	size_t len					// required length of file (create only)
 ) {
 
 	MMA_DISK_FILE_REF* dfrefp;
 	long pagesize;
 	long pages;
	struct stat statbuf;
	 	
	if (str_pathname == NULL) {
		mma_error = MMA_INVALID_FILENAME;
		return NULL;
	}


 	if (strlen(str_pathname) < PATH_MAX) {
		dfrefp = (MMA_DISK_FILE_REF*)calloc(1, sizeof(MMA_DISK_FILE_REF));
 		dfrefp->str_pathname = (char*)calloc(strlen(str_pathname) + 1, sizeof(char));
 		strcpy(dfrefp->str_pathname, str_pathname);
 		dfrefp->oflags = oflags;
 		dfrefp->mode = mode;
 		
 		if (oflags & O_CREAT) {
 			int coflags;

	 		// Calculate required length of file. Must be an integer number
	 		// of pages in size and large enough to accommodate at least len bytes
	 		pagesize = sysconf(_SC_PAGESIZE);
	 		pages = len / pagesize;
	 		if ((len % pagesize) != 0) {
	 			pages++;
	 		}
	 		dfrefp->len = pages * pagesize;

			// Set truncate flag in local flags word ...
 			coflags = dfrefp->oflags | O_TRUNC;
 			dfrefp->filedes = open(dfrefp->str_pathname, coflags, dfrefp->mode);
 		} else {
 			dfrefp->filedes = open(dfrefp->str_pathname, dfrefp->oflags);
 		}
 		if (dfrefp->filedes < 0) {
 			mma_error = MMA_ERR_FILE_OPEN;
 			mma_os_error = errno;
 			free(dfrefp);
 			dfrefp = NULL;
 		} else {
 			// Successful open. Determine if we need to set the size of
 			// the file. This is necessary if we did a file create. Also
 			// necessary is to set the file mode to MANDATORY LOCKING.
 			if (oflags & O_CREAT) {
 				if (lseek(dfrefp->filedes, dfrefp->len - 1, SEEK_SET) == -1) {
 					mma_error = MMA_ERR_FILE_SET_SIZE;
 					mma_os_error = errno;
 					free(dfrefp);
 					dfrefp = NULL;
 				} else {
 					int xoflags;
 					char buff[] = {'\0','\0'};
 					if (write(dfrefp->filedes, buff,1) != 1) {
 						mma_error = MMA_ERR_FILE_SET_SIZE;
 						mma_os_error = errno;
 						free(dfrefp);
 						dfrefp = NULL;
 					} else {
	 					// Successful write ... close and reopen file.
	 					if (close(dfrefp->filedes)) {
	 						fprintf(stderr, 
	 							"mma_new_disk_file_ref: FATAL ERROR Closing File \n%s\n",
	 							strerror(errno));
	 							exit(1);
	 					}
	 					xoflags = oflags;
	 					if (oflags & O_CREAT) {
	 						xoflags ^= O_CREAT;		// clear creation bit
	 					}
	 					if (oflags & O_TRUNC) {
	 						xoflags ^= O_TRUNC;		// clear truncation bit
	 					}
 						dfrefp->filedes = open(dfrefp->str_pathname, xoflags);
 						
 						// Set the file mode up for mandatory locking
 						if (set_mandatory_locking(dfrefp->filedes)) {
 							// Error setting mandatory locking mode
 							// error codes already captured.
 							close(dfrefp->filedes);
 							free(dfrefp);
 							dfrefp = NULL;
 						}
 					}
 				}
 			} else {
 				// Just opening the file ... determine file size
 				if (fstat(dfrefp->filedes, &statbuf) < 0) {
 					mma_error = MMA_ERR_FILE_STATUS;
 					mma_os_error = errno;
 					free(dfrefp);
 					dfrefp = NULL;
 				} else {
 					dfrefp->len = statbuf.st_size;
 				}
 			}
 		}
 	} else {
 		mma_error = MMA_ERR_PATH_MAX;
 		dfrefp = NULL;
 	}
 	
 	return dfrefp;
 }
 
 
/**
 * @brief Create a new MM atom.
 *
 * @param tag An arbitrary name string identifying the atom
 * @param obj_type Identifies the type of backing object with which the
 * 	atom is bound. Currently, only disk files are supported as backing objects.
 * @param access_mode Identifies the type of access required. (E.g. read/write)
 * 	See mmacc.h
 * @param reference Void pointer to a reference structure defining the
 * 	backing object. (Remember, right now backing objects are always files.)
 * @param len Required capacity or size of the backing object in bytes.
 * @param flags Can be set to private, shared, etc. See mmatom.h
 * @return Pointer to memory mapped atom handle structure.
 */
MMA_HANDLE* mma_create(
	char* tag,					// just a logical name for the atom
	MMA_OBJECT_TYPES obj_type,
	MMA_ACCESS_MODES access_mode,
	void* reference,			// object reference pointer
	size_t len,
	MMA_MAP_FLAGS flags
) {
	MMA_HANDLE* mmahp;

	if (reference == NULL) {
		mma_error = MMA_ERR_NULL_IO_REF;
		return NULL;
	}
	if (tag == NULL) {
		// Use default tag
		tag = "NULL-TAG";
	}
	switch (obj_type) {
		default:
		case MMT_NONE:
			mma_error = MMA_ERR_UNSUPPORTED_TYPE;
			mmahp = NULL;
			break;
		case MMT_FILE:
			if (len > ((MMA_DISK_FILE_REF*)reference)->len) {
				mmahp = NULL;
				mma_error = MMA_FILE_MAP_SIZE;
			} else {
				mmahp = create_atom(
					((MMA_DISK_FILE_REF*)reference)->filedes, access_mode, 
					len, obj_type, flags);
			}
			break;
	}
	if (mmahp != NULL) {
		mmahp->obj_type = obj_type;
		mmahp->u.void_refp = reference;
		strncpy(mmahp->tag, tag, sizeof(mmahp->tag) - 1);
	}
	return mmahp;
}

/**
 * Convert MMA Access node access mode and "user group other" key
 * to the permissions bits. (See the open(2) man page)
 * <ul>
 * <li>u => user</li>
 * <li>g => group</li>
 * <li>o => other</li>
 * </ul>
 */
int mma_file_mode(MMA_ACCESS_MODES access_mode, char* ugokey) {
	int reg = 0;
	
	if (strstr(ugokey, "u")) {
		switch (access_mode) {
			case MMA_READ:
				reg |= S_IRUSR;
				break;
			case MMA_WRITE:
				reg |= S_IWUSR;
				break;
			case MMA_READ_WRITE:
				reg |= S_IRUSR;
				reg |= S_IWUSR;
				break;
			default:
				reg |= S_IRUSR;
				break;
		}
	}
	if (strstr(ugokey, "g")) {
		switch (access_mode) {
			case MMA_READ:
				reg |= S_IRGRP;
				break;
			case MMA_WRITE:
				reg |= S_IWGRP;
				break;
			case MMA_READ_WRITE:
				reg |= S_IRGRP;
				reg |= S_IWGRP;
				break;
			default:
				break;
		}
	}
	
	if (strstr(ugokey, "o")) {
		switch (access_mode) {
			case MMA_READ:
				reg |= S_IROTH;
				break;
			case MMA_WRITE:
				reg |= S_IWOTH;
				break;
			case MMA_READ_WRITE:
				reg |= S_IROTH;
				reg |= S_IWOTH;
				break;
			default:
				break;
		}
	}
	return reg;
}

/**
 * @brief Lock the atom for read.
 *
 * This locks the entire range of bytes in the memory
 * mapped region. The calling process will wait on the lock if necessary. Note
 * that N processes can lock for reading, but if 1 process tries to lock
 * for writing while the read lock is in effect, it will be blocked.
 * Conversely, if 1 process  has a write lock all other processes attempting
 * to read or write lock will be blocked until the write lock is released.
 * 
 * Note that atoms implement mandatory locking.
 *
 * @param mmahp Pointer to MMA_HANDLE structure.
 * @return 0 on success, non-zero on failure
 */
int mma_lock_atom_read(MMA_HANDLE* mmahp) {
	return lock_cntrl(mmahp, F_SETLKW, F_RDLCK);
}

/**
 * @brief Lock the atom for write.
 * This locks the entire range of bytes in the memory
 * mapped region. The calling process will wait on the lock if necessary.
 * @param mmahp Pointer to MMA_HANDLE structure.
 * @return 0 on success, non-zero on failure
 */
 int mma_lock_atom_write(MMA_HANDLE* mmahp) {
 	return lock_cntrl(mmahp, F_SETLKW, F_WRLCK);
 }
/**
 * @brief Unlock the atom.
 * @param mmahp Pointer to MMA_HANDLE structure.
 * @return 0 on success, non-zero on failure
 */
int mma_unlock_atom(MMA_HANDLE* mmahp) {
	return lock_cntrl(mmahp, F_SETLK, F_UNLCK);
}


/**
 * Retrieve data reference pointer from a handle
 * @param mmahp Pointer to MMA_HANDLE structure.
 */
void* mma_data_pointer(MMA_HANDLE* mmahp) {
	return mmahp->mm_ref.pa;
}

/*
 * Static Functions
 */

static int xlat_access_mode(MMA_ACCESS_MODES mode) {
	
	typedef struct {
		MMA_ACCESS_MODES mode;
		int prot_code;
	} MMA2PROTROW;

	static MMA2PROTROW prot_codes[] = {
		{MMA_NONE, 0},
		{MMA_WRITE, PROT_WRITE},
		{MMA_READ, PROT_READ},
		{MMA_READ_WRITE, PROT_READ | PROT_WRITE}
	};
	
	MMA_ACCESS_MODES imode;
	int prot_code = PROT_READ;			// default access is read only
	
	for (imode = MMA_NONE; imode <= MMA_MAX; imode++ ) {
		if (prot_codes[imode].mode == mode) {
			prot_code = prot_codes[imode].prot_code;
		}
	}
	return prot_code;
}

static int xlat_mmap_flags(MMA_OBJECT_TYPES obj_type, MMA_MAP_FLAGS flags) {
	typedef struct {
		MMA_MAP_FLAGS flags;
		int iflag;
	} MMAFLAGS2FLAGSROW;
	
	static MMAFLAGS2FLAGSROW flagstable[] = {
		{MMF_SHARED, MAP_SHARED},
		{MMF_PRIVATE, MAP_PRIVATE}
	};
	
	MMA_MAP_FLAGS iflag;
	int xflag = MAP_SHARED;			// Default is shared memory map
	
	for (iflag = 0; iflag < MMF_MAX; iflag++ ) {
		if (flagstable[iflag].flags == flags) {
			xflag = flagstable[iflag].iflag;
		}
	}
	switch (obj_type) {
		case MMT_FILE:
			xflag |= MAP_FILE;
			break;
		default:
			break;
	}
	
	return xflag;
}

 /*
  * Create a new MM atom based on a file descriptor. This causes creation of the file
  * and establishment of the initial memory map setup.
  */
static MMA_HANDLE* create_atom(int filedes,
 	MMA_ACCESS_MODES access_mode, size_t len, MMA_OBJECT_TYPES obj_type, MMA_MAP_FLAGS flags) {
 		
	MMA_HANDLE* mmhp;
	
	mmhp = (MMA_HANDLE*)calloc(1, sizeof(MMA_HANDLE));
	
	mmhp->mm_ref.prot = xlat_access_mode(access_mode);
	mmhp->mm_ref.flags = xlat_mmap_flags(obj_type, flags);
	mmhp->mm_ref.len = len;
	mmhp->mm_ref.addr = (void*)0;
	mmhp->mm_ref.off = 0;
	mmhp->mm_ref.filedes = filedes;
	mmhp->mm_ref.pa = mmap(
		mmhp->mm_ref.addr,
		mmhp->mm_ref.len,
		mmhp->mm_ref.prot,
		mmhp->mm_ref.flags,
		mmhp->mm_ref.filedes,
		mmhp->mm_ref.off
	);
	if (mmhp->mm_ref.pa == MAP_FAILED) {
		mma_os_error = errno;
		mma_error = MMA_ERR_MAP_FAILED;
		free(mmhp);
		mmhp = NULL;
	}
	
	return mmhp;		
 }

/*
 * Sets a memory mapped file's access mode to indicate mandatory
 * locking. This is done by a) setting set-group-id while turning
 * off group-execute bits of the file's file mode bit map.
 */
static int set_mandatory_locking(int fd) {
 	struct stat statbuf;
 	int status = 0;
 	
 	if (fstat(fd, &statbuf) < 0) {
 		mma_error = MMA_ERR_FILE_STATUS;
 		mma_os_error = errno;
 		status = 1;
 	} else {
 		if (fchmod(fd, ((statbuf.st_mode & ~S_IXGRP) | S_ISGID)) < 0) {
 			mma_error = MMA_MANDATORY_LOCK_FAIL;
 			mma_os_error = errno;
 			status = 1;
 		}
 	}
 	return status;
}

static int lock_cntrl(MMA_HANDLE* mmahp, int cmd, int type) {
	struct flock lock;
	
	lock.l_type = type;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;			// 0 means to EOF

	return (fcntl(mmahp->mm_ref.filedes, cmd, &lock));
}


/**
 * Access method ... get a disk file atom's file path. Returns NULL if atom is
 * not open or if not a disk file based memory mapped atom.
 * @param mmahp Pointer to MMA_HANDLE structure.
 * @return Full path of atom's backing disk file
 */
char* mma_get_disk_file_path(MMA_HANDLE* mmahp) {
	struct stat buff;

	if (mmahp->obj_type == MMT_FILE) {
		if (mmahp->mm_ref.filedes == mmahp->u.df_refp->filedes) {
			if (!fstat(mmahp->mm_ref.filedes, &buff)) {
				return mmahp->u.df_refp->str_pathname;
			}
		}
	}
	return NULL;
}

/**
 * Unmap the memory mapped atom's memory.
 * @param mmahp Pointer to MMA_HANDLE structure.
 * @return 0 on success, non-zero on failure
 */
int mma_destroy_atom(MMA_HANDLE* mmahp) {
	int status = 0;
	switch (mmahp->obj_type) {
	case MMT_FILE:
		close(mmahp->mm_ref.filedes);
		break;
	default:
		// No other object types defined yet!
		break;
	}
	status = munmap(mmahp->mm_ref.addr, mmahp->mm_ref.len);
	free(mmahp);
	return status;
}
