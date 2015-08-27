/*
 *****************************************************************

<GPL>

Copyright: © 2001-2012 Robert C Garvey

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
 * msgdeque.c
 *
 *  Created on: Dec 22, 2012
 *      Author: robgarv
 */

/**
 * @file msgdeque.c
 *
 * @brief Message Deque Facility for Inter-process Communications
 *
 *  This module provides a simple process-to-process communications
 *  mechanism using memory mapped dequeues and message cells (POSIX
 *  semaphores). For the message cell implementation, see msgcell.h
 *  and msgcell.c
 */

#include <msgdeque.h>
#include <msgcell.h>
#include <stdlib.h>
#include <string.h>
#include <ulppk_log.h>

#define DEQUEPREFIX "msgdeque-"
#define LOCKPREFIX "msgdeque-lock-"


/**
 * @brief Data check function for message deques.
 *
 * Principally for internal use by this module.
 *
 * @param datap Pointer to a MSGDEQUE structure.
 * @return 1 if fresh data is available. 0 if data not available.
 */
int msgdeque_datacheck(void* datap) {
	MMA_HANDLE* mmahp;
	DQSTATS dqstats;
	DQSTATS* dqstatsp;
	int data_avail = 0;

	mmahp = ((MSGDEQUE*)datap)->deque;
	dqstatsp = mmdq_stats(mmahp, &dqstats);
	if (dqstatsp->dquse > 0) {
		data_avail = 1;
	}
	return data_avail;
}

/*
 * Make a dequename from a message cell name. Dequenames will
 * be of the form prefix-<msg cell name> ... so dequefiles will
 * be of the form prefix-<msg cellname>.dq
 *
 * If buff is NULL, memory is allocated from the heap. In this
 * case the calling app must free the memory. If the
 * buffer is too small, ULPPK_CRASH is called to shutdown the
 * calling application and log this error.
 */
static char* make_name(char* buff, size_t buffsize, const char* name) {
	int dqnamelen;
	static char* prefix = "msgdeque-";

	dqnamelen = strlen(prefix) + strlen(name) + 1;
	if ((buff != NULL) && (buffsize < dqnamelen)) {
		// Caller must supply a sufficiently larger buffer
		ULPPK_CRASH("Buffer too small: Have %d need %d", buffsize, dqnamelen);
	} else {
		buff = (char*)calloc(dqnamelen, sizeof(char));
	}
	strcpy(buff, prefix);
	strcat(buff, name);
	return buff;
}

/**
 * @brief Create a message deque structure. This will consist of at msgcell and
 * a memory mapped dequeue. This form sets up a message dequeue that accepts
 * records of a fixed length specified by item_size.
 *
 * To send to the dequeue use the msgdeque_send function. The receive
 * from the dequeue call the msgdeque_rec function.
 *
 * @param name  name of the dequeue
 * @param permissions  access permissions (see open(2)
 * @param item_size  size of the records accepted by this dequeue.
 * @param nitems  maximum capacity of the dequeue.
 * @return  pointer to the created MSGCELL structure.
 */
MSGCELL* msgdeque_create(const char *name, uint permissions, ushort item_size, ushort nitems) {
	MSGCELL* msgcellp;
	MMA_HANDLE* mmahp;
	MSGDEQUE* msgdqp;
	char* dequename;
	char* dequedir;
	char* lockfilepath;
	char lockfile[1024];
	int dqnamelen;
	int lflen;

	dequename = make_name(NULL, 0, name);
	dequedir = mmdq_dequedir();
	strcpy(lockfile, dequename);
	strcat(lockfile, ".lock");
	lflen = strlen(dequedir) + strlen(lockfile) + 3;
	lockfilepath = (char*)calloc(lflen, sizeof(char));
	strcpy(lockfilepath, dequedir);
	strcat(lockfilepath, "/");
	strcat(lockfilepath, lockfile);
	msgdqp = (MSGDEQUE*)calloc(1, sizeof(MSGDEQUE));
	msgdqp->deque = mmdq_create(dequename, item_size, nitems);
	msgdqp->lock = mmapfile_create(lockfile, lockfilepath, 8, MMA_READ_WRITE, MMF_SHARED, permissions);
	msgcellp = msgcell_create(name, permissions, msgdqp, msgdeque_datacheck);
	free(dequename);
	free(lockfilepath);
	return msgcellp;
}

/**
 * @brief Create a message deque structure for "byte streams". This will consist of at msgcell and
 * a memory mapped dequeue. This function sets up a message dequeue that accepts
 * variable length byte sequences.
 *
 * To send to the dequeue use the msgdeque_send_bytes function. The receive
 * from the dequeue call the msgdeque_rec_bytes function.
 *
 * @param name  name of the dequeue
 * @param permissions  access permissions (see open(2)
 * @param byte_capacity  maximum capacity of the dequeue in bytes
 * @return  pointer to the created MSGCELL structure.
 */
MSGCELL* msgdeque_create_byte_stream(const char *name, uint permissions,  ushort byte_capacity) {
	return msgdeque_create(name, permissions, sizeof(unsigned char), byte_capacity+4);
}

/**
 * @brief Attach to a previously created message deque.
 * @param name  name of the dequeue
 * @return  pointer to the created MSGCELL structure.
 */
MSGCELL* msgdeque_attach(const char *name) {
	char* dequename;
	char* dequedir;
	MSGCELL* msgcellp;
	MSGDEQUE* msgdqp;
	char* lockfilepath;
	char lockfile[1024];
	int lflen;

	msgdqp = (MSGDEQUE*)calloc(1, sizeof(MSGDEQUE));
	dequename = make_name(NULL, 0, name);
	dequedir = mmdq_dequedir();
	strcpy(lockfile, dequename);
	strcat(lockfile, ".lock");
	lflen = strlen(dequedir) + strlen(lockfile) + 3;
	lockfilepath = (char*)calloc(lflen, sizeof(char));
	strcpy(lockfilepath, dequedir);
	strcat(lockfilepath, "/");
	strcat(lockfilepath, lockfile);
	msgdqp->deque = mmdq_open(dequename);
	msgdqp->lock = mmapfile_open(lockfile, lockfilepath, MMA_READ_WRITE, MMF_SHARED);
	msgcellp = msgcell_attach(name, msgdqp, msgdeque_datacheck);
	free(dequename);
	free(lockfilepath);
	return msgcellp;
}

/**
 * @brief Reset a message deque to the empty state and clear
 * the sempahore.
 *
 * This can have unpredictable results if unruly senders are calling msgdeque_send
 *
 * @param msgcellp pointer to a MSGCELL structure returned by create or attach functions.
 * @return 0 on success, non-zero on failure
 */
int msgdeque_reset(MSGCELL* msgcellp) {
	int retval;
	// Reset the deque
	mmdq_reset((MMA_HANDLE*)msgcellp->datap);
	// Reset the message cell
	retval = msgcell_reset(msgcellp);
	return retval;
}

static unsigned char* encode_uint(unsigned char buff[], unsigned int datalen) {
	buff[0] = (datalen & 0x000000FF);
	buff[1] = (datalen & 0x0000FF00) >> 8;
	buff[2] = (datalen & 0x00FF0000) >> 16;
	buff[3] = (datalen & 0xFF000000) >> 24;
	return buff;
}

static unsigned int decode_uint(unsigned char* buff) {
	unsigned int x;
	unsigned int reg;

	x = 0;
	reg = buff[3];
	reg = reg  << 24;
	x |= reg;
	reg = buff[2];
	reg = reg  << 16;
	x |= reg;
	reg = buff[1];
	reg = reg  << 8;
	x |= reg;
	reg = buff[0];
	x |= reg;
	return x;
}

/**
 * Send a fixed length record. Size was defined when the
 * message deque was created by msdeque_create.
 *
 * @param msgcellp  pointer to the message cell
 * @param sendp  void pointer to the record to be sent
 * @return  0 on success, non-zero if deque is full. In
 *  that case, the data pointed to by sendp was not inserted.
 */
int msgdeque_send(MSGCELL* msgcellp, void* sendp) {
	MMA_HANDLE* deque;
	int retval = 0;

	/*
	 * This is an atomistic insertion into the deque ...
	 * mmdq_abd locks/unlocks the deque header, so
	 * no further protection is required. Add to the
	 * deque an signal the deque reader
	 */
	deque = ((MSGDEQUE*)msgcellp->datap)->deque;
	retval = mmdq_abd(deque, sendp);
	if (0 == retval) {
		msgcell_send(msgcellp);
	}
	return retval;
}

/**
 * Receive a fixed length record. Size was defined when
 * the deque was created (see msgdeque_create). The
 * function blocks until a record is available.A return
 * value of NULL indicates an error has occurred.
 *
 * @param msgcellp  pointer to the message cell
 * @return Pointer to received record.
 */
void* msgdeque_rec(MSGCELL* msgcellp) {
	int retval = 0;
	void* rec = NULL;
	MMA_HANDLE* deque;
	MMA_HANDLE* lock;
	DQSTATS dqstats;
	DQSTATS* dqstatsp = NULL;
	size_t recordlen = 0;

	deque = ((MSGDEQUE*)msgcellp->datap)->deque;
	lock = ((MSGDEQUE*)msgcellp->datap)->lock;

	while ((NULL == rec ) && (0 == retval)) {
		// Lock access to the message deque and get the
		// current dequeue stats

		mma_lock_atom_write(lock);
		dqstatsp = mmdq_stats(deque, &dqstats);
		if (dqstatsp->dquse != 0) {
			// We have data
			recordlen = (size_t)dqstatsp->dqitem_size;
			rec = (void*)calloc(1, recordlen);
			if (mmdq_rtd(deque, rec) != 0) {
				// This should never happen. But we can take some action to
				// possibly recover from the error.
				free(rec);
				rec = NULL;
				ULPPK_LOG(ULPPK_LOG_ERROR, "Error retrieving from message deque: %s", msgcellp->name);
			}
		}
		// Unlock access to the message deque
		mma_unlock_atom(lock);

		// If we still don't have a record
		if (NULL == rec) {
			// Wait for a signal on the message cell.
			retval = msgcell_rec(msgcellp);
			if (retval) {
				ULPPK_LOG(ULPPK_LOG_ERROR, "Error receiving on message cell [%d / %s",
						msgcellp->errcode, strerror(msgcellp->errcode));
			}
		}
	}
	return rec;
}

/**
 * Send to a byte stream deque. .A byte stream dequeue was created by calling
 * msgdeque_create_byte_stream.
 *
 * @todo Investigate using the packet send/receive functions of mmdeque.c
 * as the basis for this function.
 *
 * @param msgcellp  pointer to the message cell
 * @param pdata  pointer to the bytes to be send
 * @param datalen  number of bytes to send
 * @return  0 on success, non-zero on failure.
 */
int msgdeque_send_byte_stream(MSGCELL* msgcellp, void* pdata, size_t datalen) {
	unsigned char lenbuff[4];
	unsigned char* pbyte;
	MMA_HANDLE* lock;
	MMA_HANDLE* deque;
	int i;
	int retval = 0;

	pbyte = (void*)lenbuff;
	encode_uint(lenbuff, datalen);
	lock = ((MSGDEQUE*)msgcellp->datap)->lock;
	deque = ((MSGDEQUE*)msgcellp->datap)->deque;
	mma_lock_atom_write(lock);
	// Push the 4 bytes of encoded size onto the deque
	for (i = 0; ((i < sizeof(lenbuff)) && (retval ==0)); i++, pbyte++) {
		retval = mmdq_abd(deque, pbyte);
	}
	// Now push the data
	if (0 == retval) {
		pbyte = (unsigned char*)pdata;
		for (i = 0; ((i < datalen) && (retval ==0)); i++, pbyte++) {
			retval = mmdq_abd(deque, pbyte);
		}
	}
	mma_unlock_atom(lock);

	// Send to the message cell (post to semaphore)
	msgcell_send(msgcellp);
	return retval;
}

/**
 * Receive bytes on a deque created by msgdeque_create_byte_stream
 * The buffer returned by this function must be released by calling
 * free.
 *
 * @todo Investigate using the packet send/receive functions of mmdeque.c
 * as the basis for this function.
 *
 * @param msgcellp  pointer to the message cell
 * @param bytes_received  pointer to size_t to receive number of
 * 	bytes received.
 * @return  pointer to received data bytes
 */
void* msgdeque_rec_byte_stream(MSGCELL* msgcellp, size_t* bytes_received) {
	MMA_HANDLE* deque;
	MMA_HANDLE* lock;
	DQSTATS dqstats;
	DQSTATS* dqstatsp = NULL;
	unsigned char* databuffp = NULL;
	unsigned char* pbyte = NULL;
	int i;
	int retval = 0;
	int datalen = 0;
	char sizebuff[4];

	deque = ((MSGDEQUE*)msgcellp->datap)->deque;
	lock = ((MSGDEQUE*)msgcellp->datap)->lock;

	while ((NULL == databuffp) && (0 == retval)) {
		// lock access to the message deque
		mma_lock_atom_write(lock);

		// Get current deque stats
		dqstatsp = mmdq_stats(deque, &dqstats);
		if (dqstatsp->dquse >= 4) {
			// we got some data!
			// Pop the four bytes of encoded size of the deque
			for (i = 0; ((i < sizeof(sizebuff)) && (0 == retval)); i++) {
				retval = mmdq_rtd(deque, &sizebuff[i]);
			}
			// Check for error condition
			if (retval) {
				ULPPK_LOG(ULPPK_LOG_ERROR, "Error popping encoded size of byte stream deque %s", msgcellp->name);
			} else {
				datalen = decode_uint(sizebuff);
				if (datalen > 0) {
					databuffp = (unsigned char*)calloc(datalen, sizeof(unsigned char));
					for (i = 0, pbyte = databuffp; ((i < datalen) && (0 == retval)); i++, pbyte++ ) {
						retval = mmdq_rtd(deque, pbyte);
					}
					if (retval) {
						free(databuffp);
						databuffp = NULL;
						ULPPK_LOG(ULPPK_LOG_ERROR, "Error popping encoded size of byte stream deque %s", msgcellp->name);
					}
				}
			}
		}
		mma_unlock_atom(lock);

		// If after all that, we have no data ... wait
		// for some data!
		if (NULL == databuffp) {
			retval = msgcell_rec(msgcellp);
			if (retval) {
				ULPPK_LOG(ULPPK_LOG_ERROR, "Error receiving on message cell [%d / %s",
						msgcellp->errcode, strerror(msgcellp->errcode));
			}
		}
	}
	return databuffp;
}
