
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
 * @file mmdeque.c
 *
 * @brief Memory Mapped Double Ended Queue Support
 *
 * This is a compound construct that integrates a memory mapped file (see mmapfile.c)
 * with a double ended queue (deque) (see dqacc.c)  mmdeques are deques that are
 * persistent and shareable between processes.
 *
 * All these functions perform locking of the memory mapped deque before operating,
 * and are thus thread safe.
 *
 * The package reads the environment variable MMDQ_DIR_PATH. This should
 * be the directory that will contain the deque files. If you are using
 * sysconfig.c and ini file support (ifile.c) then this environment variable
 * can be overridden by inifile settings.
 *
 * See dequetool (dequetool.c) utility. Dequetool provides features for creating,
 * loading, reading, report on, and resetting memory mapped deques.
 */

#include <stdio.h>
#include <string.h>

#include <appenv.h>
#include <mmdeque.h>
#include <diagnostics.h>

/*
 * Given a pointer p, presume p points to first byte of deque header. 
 * Calculate offset to first byte of data buffer.
 */
static size_t buffer_start_offset(DQHEADER* p) {
	return sizeof(*p);
}

#if 0
static void* deque_bufferp(DQHEADER* dequep) {
	void* p0;
	void* buffp;
	
	p0 = (void*)dequep;
	buffp = p0 + buffer_start_offset(dequep);
	return buffp;
}
#endif

static char* lerrmsg(MMA_HANDLE* mmahp, char* opstring) {
	static char buff[4096 + 512];
	
	sprintf(buff, "%s : File: %s", opstring, mmahp->u.df_refp->str_pathname);
	return buff;
}
/*
 * Calculate total file size given the size of the deque items and
 * the number of items the deque can contain.
 */
static size_t deque_file_len(ushort item_size, ushort nitems) {
	size_t len;
	
	len = sizeof(DQHEADER) + (item_size * nitems);
	return len;
}

/**
 * @brief Return the path to the memory mapped deque directory.
 * The deque directory is where an application or system of
 * applications maintains its memory mapped deque files.
 *
 * @return full path to memory mapped deque directory.
 */
char* mmdq_dequedir() {
	char* dequedir;

	dequedir = appenv_read_env_var(MMDQ_DIR_PATH);
	if (NULL == dequedir) {
		dequedir = appenv_register_env_var(MMDQ_DIR_PATH, DEFAULT_MMDQ_DIR_PATH);
	}
	return dequedir;
}

/**
 * @brief Given a deque name, return the full path to the deque file.
 * If pathbuff is NULL, then the string returned is allocated from the heap
 * and must be released by calling free.
 * @param pathbuff Pointer to buffer to receive path buffer. Caller must
 * 	insure it is long enough. If NULL, memory is allocated from the heap.
 * 	The caller MUST eventuall free this memory.
 * @param dequename Name of the deque.
 * @return Pointer to full path name.
 */
char* mmdq_dequepath(char* pathbuff, const char* dequename) {
	char* dequedir;

	dequedir = mmdq_dequedir();
	if (pathbuff == NULL) {
		int pathbufflen;
		pathbufflen = strlen(dequedir) + strlen(dequename) + 3;
		pathbuff = (char*)calloc(pathbufflen, sizeof(char));
	}
	strcpy(pathbuff, dequedir);
	strcat(pathbuff, "/");
	strcat(pathbuff, dequename);
	strcat(pathbuff, ".dq");
	return pathbuff;
}

/**
 * @brief Given a memory mapped deque handle, retrieve the full path
 * to the deque file. A wrapper for mma_get_disk_file_path.
 * @param mmdqhp Pointer to the MMA_HANDLE structure representing the deque.
 * @return Full path of the deque file.
 */
char* mmdq_dequepath_from_handle(MMA_HANDLE* mmdqhp) {
	return mma_get_disk_file_path(mmdqhp);
}

/**
 * @brief Create a memory mapped deque.
 *
 * Creates a memory mapped file and sets up a deque header in the memory mapped
 * region. Initializes the deque header so that the deque buffer points into
 * the memory mapped region.
 * @param dequename Name of the deque
 * @param item_size Size of the items to be pushed onto the deque in bytes.
 * @param nitems Max number of items the deque is to store (deque slots).
 * @return Pointer to MMA_HANDLE structure representing the memory mapped deque.
 */
MMA_HANDLE* mmdq_create(const char* dequename, ushort item_size, ushort nitems) {
	MMA_HANDLE* mmahp = NULL;
	char tagbuff[MAX_DEQUE_NAME_LEN];
	char* dequefile;
	size_t len;
	DQHEADER* dequep = NULL;
	size_t buffx = 0;
	
	memset(tagbuff, 0, sizeof(tagbuff));
	strncpy(tagbuff, dequename, sizeof(tagbuff)-1);
	dequefile = mmdq_dequepath(NULL, dequename);
	len = deque_file_len(item_size, nitems);
	// printf("Dequefile name = %s\n", dequefile);
	mmahp = mmapfile_create(tagbuff, dequefile, len, MMA_READ_WRITE, MMF_SHARED, 0664);
	free(dequefile);		// release the dequefile string buffer
	if (mmahp == NULL) {
		return NULL;		// Error ... let application handle it.
	}
	// Now get the data pointer from the MMA_HANDLE. We're going to format
	// a deque header at that address. We will calculate the buffer offset relative
	// to the header and set up a deque on the memory mapped region.
	dequep = (DQHEADER*)mma_data_pointer(mmahp);
	buffx = buffer_start_offset(dequep);
	
	// Now initialize the memory mapped deque.
	dq_init_memmap(nitems, item_size, buffx, dequep);
	
	return mmahp;
}

/**
 * @brief Open access to a previously created memory mapped deque.
 *
 * @param dequename Name of the deque
 * @return Pointer to MMA_HANDLE structure representing the memory mapped deque.
 */
MMA_HANDLE* mmdq_open(const char* dequename) {
	MMA_HANDLE* mmahp = NULL;
	char tagbuff[MAX_DEQUE_NAME_LEN];
	char* dequefile;

	memset(tagbuff, 0, sizeof(tagbuff));
	strncpy(tagbuff, dequename, sizeof(tagbuff)-1);
	dequefile = mmdq_dequepath(NULL, dequename);
	mmahp = mmapfile_open(tagbuff, dequefile, MMA_READ_WRITE, MMF_SHARED);
	free(dequefile);
	return mmahp;
}

/*
 * @brief Close a memory mapped deque.
 *
 * This causes the memory mapped region to become unmapped.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @return 0 on success.
 */
int mmdq_close(MMA_HANDLE* mmdqhp) {
	mmapfile_close(mmdqhp);
	return 0;
}

/**
 * @brief Check to see if the memory mapped deque is empty.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @return 1 if the deque is empty 0 otherwise.
 */
int mmdq_isempty(MMA_HANDLE* mmdqhp) {
	DQHEADER* dequep;
	int retval = 0;
	
	if (mma_lock_atom_read(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp,"Error locking atom!"));
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	retval = dq_isempty(dequep);
	
	if (mma_unlock_atom(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp, "Error unlocking atom!"));
	return retval;
}

/**
 * @brief Add item to top of memory mapped deque.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param itemp Pointer to the item to be added.
 * @return 0 on success.
 */
int mmdq_atd(MMA_HANDLE* mmdqhp,void* itemp) {
	DQHEADER* dequep;
	int retval = 0;
	
	if (mma_lock_atom_write(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp,"Error locking atom!"));
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	retval = dq_atd(dequep, itemp);
	if (mma_unlock_atom(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp, "Error unlocking atom!"));
	
	return retval;
}

/**
 * @brief Add item to bottom of memory mapped deque.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param itemp Pointer to the item to be added.
 * @return 0 on success.
 */
int mmdq_abd(MMA_HANDLE* mmdqhp,void* itemp) {
	DQHEADER* dequep;
	int retval = 0;
	
	if (mma_lock_atom_write(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp,"Error locking atom!"));
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	retval = dq_abd(dequep, itemp);

	if (mma_unlock_atom(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp, "Error unlocking atom!"));
	return retval;
}

/**
 * @brief Remove item from top of memory mapped deque.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param itemp Pointer to properly sized memory area that will receive a copy of the
 * 	item data.
 * @return 0 on success.
 */
int mmdq_rtd(MMA_HANDLE* mmdqhp,void* itemp) {
	DQHEADER* dequep;
	int retval = 0;
	
	if (mma_lock_atom_write(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp,"Error locking atom!"));
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	retval = dq_rtd(dequep, itemp);

	if (mma_unlock_atom(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp, "Error unlocking atom!"));
	return retval;
}

/**
 * @brief Remove item from bottom of memory mapped deque.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param itemp Pointer to properly sized memory area that will receive a copy of the
 * 	item data.
 * @return 0 on success.
 */
int mmdq_rbd(MMA_HANDLE* mmdqhp,void* itemp) {
	DQHEADER* dequep;
	int retval = 0;
	
	if (mma_lock_atom_write(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp,"Error locking atom!"));
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	retval = dq_rbd(dequep, itemp);

	if (mma_unlock_atom(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp, "Error unlocking atom!"));
	return retval;
}

/**
 * @brief Reset a memory mapped deque to the empty state.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @return 0 on success.
 */
int mmdq_reset(MMA_HANDLE* mmdqhp) {
	DQHEADER* dequep;
	int retval = 0;
	DQHEADER tempdq;
	
	if (mma_lock_atom_write(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp,"Error locking atom!"));
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	memcpy(&tempdq, dequep, sizeof(DQHEADER));
	memset(dequep, 0, mmdqhp->mm_ref.len);
	dq_init_memmap(tempdq.dqslots, tempdq.dqitem_size, tempdq.dqbuffx, dequep); 

	if (mma_unlock_atom(mmdqhp)) APP_ERR(stderr, lerrmsg(mmdqhp, "Error unlocking atom!"));
	return retval;
}

/**
 * Obtains memory mapped data pointer to deque and
 * calls dq_stats.
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param dq_statsp Pointer to a DQSTATS structure to receive deque status info.
 * 	If NULL, memory is allocated from the heap and MUST be freed by the caller.
 * @return Pointer to DQSTATS structure.
 */
DQSTATS* mmdq_stats(MMA_HANDLE* mmdqhp, DQSTATS* dq_statsp) {
	DQHEADER* dequep;
	
	dequep = (DQHEADER*)mma_data_pointer(mmdqhp);
	return dq_stats(dequep, dq_statsp);
}

// TODO: Implement sync check at some point.

static unsigned char sync_bytes[] = {0xF1, 0x0E, 0xA5, 0x5A};

typedef struct {
	unsigned char sync_bytes[sizeof(sync_bytes)];
	size_t packet_len;
} MMDQ_PACKET_HEADER;

static int sync_check(MMDQ_PACKET_HEADER* headerp) {
	int i;
	int flag;

	
	for (i = 0, flag = 1; i < sizeof(headerp->sync_bytes); i++) {
		flag &= (headerp->sync_bytes[i] == sync_bytes[i]);
	}
	return flag;
}

static MMDQ_PACKET_HEADER write_header = {
	{0xF1, 0x0E, 0xA5, 0x5A},
	0
};

static int push_packet(MMA_HANDLE* mmdqhp, void* packetp, size_t len) {
	int i;
	void* cp;
	
	for (i = 0, cp = packetp; i < len; i++, cp++) {
		if( mmdq_abd(mmdqhp, cp)) {
			DBG_TRACE(stderr, "Packet Deque overflow!: %s", mma_get_disk_file_path(mmdqhp));
			return 1;
		}
	}
	return 0;

}

static int pop_packet(MMA_HANDLE* mmdqhp, void* packetp, size_t packet_len) {
	int i;
	void* cp;

	for (i = 0, cp = packetp; i < packet_len; i++, cp++) {
		if (mmdq_rtd(mmdqhp, cp)) {
			DBG_TRACE(stderr, "Packet Deque underflow!: %s", mma_get_disk_file_path(mmdqhp));
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Write a packet of data to a memory mapped deque.
 *
 * In order to be used as a packet exchange deque, the deque must
 * be created by a item size of 1. This function will APP_ERR
 * if that is not properly set up.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param packetp Pointer to packet data.
 * @param packet_len Size of the packet in bytes.
 * @return 0 on success.
 */
int mmdq_write_packet(MMA_HANDLE* mmdqhp, void* packetp, size_t packet_len) {
	void * cp;
	int i;
	DQSTATS statsbuff;
	DQSTATS* dqstatsp;

	dqstatsp = mmdq_stats(mmdqhp,&statsbuff);
	if (dqstatsp->dqitem_size != sizeof(*cp)) {
		APP_ERR(stderr, "For use as packet deque, deque item size must be size of single byte!");
	}
	// Push header packet. Write length to pre-setup header structure
	write_header.packet_len = packet_len;
	if (!push_packet(mmdqhp, &write_header, sizeof(write_header))) {
		return push_packet(mmdqhp, packetp, packet_len);
	}
	return 1;
}

/**
 * @brief Read a packet of data from memory mapped deque.
 *
 * In order to be used as a packet exchange deque, the deque must
 * be created by a item size of 1. This function will APP_ERR
 * if that is not properly set up.
 *
 * @param mmdqhp Pointer to MMA_HANDLE structure representing the memory mapped deque.
 * @param packet_lenp Pointer to a size_t variable that will receive
 *  size of the received packet in bytes.
 * @return Pointer to received packet data.
 */
void* mmdq_read_packet(MMA_HANDLE* mmdqhp, size_t* packet_lenp) {
	MMDQ_PACKET_HEADER header;
	void* packetp;
	DQSTATS statsbuff;
	DQSTATS* dqstatsp;
	int i;

	dqstatsp = mmdq_stats(mmdqhp, &statsbuff);
	*packet_lenp = 0;
	if (dqstatsp->dqitem_size != sizeof(unsigned char)) {
		APP_ERR(stderr, "For use as packet deque, deque item size must be size of single byte!");
	}

	if (pop_packet(mmdqhp, &header, sizeof(MMDQ_PACKET_HEADER))) {
		DBG_TRACE(stderr, "Error reading packet header from packet deque at %s", mma_get_disk_file_path(mmdqhp));
		return NULL;
	}
	packetp = calloc(header.packet_len, sizeof(void*));
	if (! sync_check(&header)) {
		DBG_TRACE(stderr, "mmdq_read_packet detected packet sync issue!");
	}
	for (i = 0; i < header.packet_len; i++) {
		if (pop_packet(mmdqhp, packetp, header.packet_len)) {
			DBG_TRACE(stderr, "Error reading packet data from packet deque at %s", mma_get_disk_file_path(mmdqhp));
			free(packetp);
			return NULL;
		}
	}
	return packetp;
}
	
