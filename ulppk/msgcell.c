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
 * msgcell.c
 *
 *  Created on: Dec 22, 2012
 *      Author: robgarv
 */

/**
 * @file msgcell.c
 *
 * @brief Inter-process Message Cell Synchronization Construct
 *
 * A "message cell" is a simple construct that facilitates inter-process communications.
 * A data object is logically associated with a POSIX semaphore. The data object is
 * normally a memory mapped (an mmatom construct), so it can be safely accessed by multiple
 * processes.
 *
 * A server process receives data through the data object. When it wants to
 * receive data, the process calls msgcell_rcv, passing a check function.
 * The check function checks the data object and if new data is present the call
 * returns immediately. Otherwise, the calling process is blocked on a sem_wait.
 *
 * A client process writes data into the data object and calls msgcell_xmt to signal
 * delivery of data. msgcell_xmt calls sem_post to signal a pending server process.
 *
 * This package makes no assumptions about the type or access methods of the data
 * object. The calling application is responsible for creating/opening the data object,
 * and closing it properly when access is no longer required.
 *
 * Data check functions, of course, incorporate knowledge of the data object and
 * its access methods. These functions accept a single void* as input and return
 * an int. A return of 0 (MSGCELL_NODATA_AVAIL) means no data available ... a return of
 * 1 (MSGCELL_DATA_AVAIL) means data is available.
 */


#include <stdlib.h>
#include <errno.h>
#include <msgcell.h>

/**
 * A stub data check function that always returns true.
 * This is written to the message cell by msgcell_create
 * and msgcell_attach if datacheckfuncp is NULL.
 */
static int datacheckfunc_stub(void* datap) {
	return 1;
}

/**
 * @brief Create a message cell. Typically called by a message cell server
 * (receiver).
 * @param name  Character string containing the unique name of the semaphore
 * @param permissions  Unsigned interger. If 0xFFFF, then use default
 * 	permissions of S_IRWXU | S_IRWXG. Otherwise permissions contains permissions
 * 	bits as specified in man page open(2)
 * @param datap  Pointer to data structure/object. This may be NULL.
 * @param datacheckfuncp Data check function. returns 1 if data is available,
 * 0 otherwise. If NULL, the functions msgcell_rec and msgcell_timedrec presume
 * data	is always available.
 * @return pointer to a MSGCELL structure. If an error occurred, the errcode
 * 	field will contain a errno value. This pointer references memory allocated
 * 	from the heap. The msgcell_delete and msgcell_close functions both return
 * 	this memory.
 */
MSGCELL* msgcell_create(const char* name, uint permissions,  void* datap, MSGCELL_DATACHECK_FUNC* datacheckfuncp) {
	MSGCELL* msgcellp;

	msgcellp = (MSGCELL*)calloc(1, sizeof(MSGCELL));
	msgcellp->datap = datap;
	if (datacheckfuncp == NULL) {
		msgcellp->datacheckfuncp = datacheckfunc_stub;
	} else {
		msgcellp->datacheckfuncp = datacheckfuncp;
	}
	// We set the semaphore value to 0 so a msgcell_rec call will
	// block until someone posts.
	msgcellp->semp = sem_open(name, (O_RDWR | O_CREAT), permissions, 0);
	if (SEM_FAILED == msgcellp->semp) {
		msgcellp->errcode = errno;
	}
	return msgcellp;
}

/*
 * @brief Attach to a message cell. Typically called by a message cell client (sender)
 * @param name  Character string containing the unique name of the semaphore
 * @param datap  Pointer to data structure/object. This may be NULL.
 * @param datacheckfuncp Data check function. returns 1 if data is available,
 * 0 otherwise. If NULL, the functions msgcell_rec and msgcell_timedrec presume
 * data	is always available.
 * @return pointer to a MSGCELL structure. If an error occurred, the errcode
 * 	field will contain a errno value. This pointer references memory allocated
 * 	from the heap. The msgcell_delete and msgcell_close functions both return
 * 	this memory.
 */
MSGCELL* msgcell_attach(const char* name, void* datap, MSGCELL_DATACHECK_FUNC* datacheckfuncp) {
	MSGCELL* msgcellp;

	msgcellp = (MSGCELL*)calloc(1, sizeof(MSGCELL));
	msgcellp->datap = datap;
	if (datacheckfuncp == NULL) {
		msgcellp->datacheckfuncp = datacheckfunc_stub;
	} else {
		msgcellp->datacheckfuncp = datacheckfuncp;
	}
	msgcellp->semp = sem_open(name, O_RDWR);
	if (SEM_FAILED == msgcellp->semp) {
		msgcellp->errcode = errno;
	}
	return msgcellp;

}
/**
 * @brief Destroy the message cell. Presumes it has already been closed.
 *
 * @param msgcellp Pointer to MSGCELL structure
 * @return 0 on success, non-zero on failure.
 */
int msgcell_delete(MSGCELL* msgcellp) {
	int retval;

	retval = sem_unlink(msgcellp->name);
	return retval;
}

/**
 * @brief Close the message cell. The caller is responsible for closing
 * the application specific data object BEFORE calling this function.
 *
 * @param msgcellp Pointer to MSGCELL structure
 * @return 0 on success, non-zero on failure.
 */
int msgcell_close(MSGCELL* msgcellp) {
	int retval;

	retval = sem_close(msgcellp->semp);
	return retval;
}

/**
 * @brief Retrieve the data pointer from the message cell.
 *
 * @param msgcellp Pointer to MSGCELL structure
 * @return  Pointer to message cell data structure
 */
void* msgcell_get_data_pointer(MSGCELL* msgcellp) {
	return msgcellp->datap;
}

/**
 * @brief Transmit to a message cell.
 *
 * @param msgcellp  The message cell to transmit to.
 * @return 0 on success, 1 on failure. msgcellp->errcode receives the
 * 	value of errno on failure. If the datacheck function returns
 * 	false, then this function fails with msgcellp->errcode = ENODATA
 */
int msgcell_send(MSGCELL* msgcellp) {
	int retval = 0;

	if ((*msgcellp->datacheckfuncp)(msgcellp->datap)) {
		if (sem_post(msgcellp->semp) != 0) {
			msgcellp->errcode = errno;
			retval = 1;
		}
	} else {
		msgcellp->errcode = ENODATA;
		retval = 1;
	}
	return retval;
}

/**
 * Receive via message cell. Data is not retrieved by this
 * function. This function will not return until data is
 * available as determined by the data check function.
 *
 * 1) Calls the application specific datacheck function provided when msgcell_create
 * 	was called. If the data check function returns true, this function returns
 * 	immediately.
 * 2) If the data check functions returns false, blocks on sem_wait
 * 3) When another process posts to the message cell, sem_wait returns
 * 	and the data check function is again called. If the data check
 * 	again returns false, the function calls sem_wait again. Etc.
 *
 * @param msgcellp pointer to the message cell structure returned by msgcell_create
 * 	or msgcell_attach.
 * @return 0 if successful, 1 if error. If error, errno is written to msgcellp->errcode
 */
int msgcell_rec(MSGCELL* msgcellp) {
	int retval = 0;
	while (! (*msgcellp->datacheckfuncp)(msgcellp->datap)) {
		if (sem_wait(msgcellp->semp) != 0) {
			msgcellp->errcode = errno;
			retval = 1;
		}
	}
	return retval;
}
/**
 * Receive via message cell with timeout. Unlike msgcell_rec, will
 * not keep suspending on a sem_wait or sem_timedwait until data
 * is available.
 *
 * 1) Calls the application specific datacheck function provided when msgcell_create
 * 	was called. If the data check function returns true, this function returns
 * 	immediately.
 * 2) If the data check functions returns false, blocks on sem_timedwait
 * 3) When another process posts to the message cell, or when the timeout
 * 	period defined in the timespec timeout argument expires, sem_timedwait returns
 * 	and the data check function is again called. If the data check
 * 	again returns false, the function returns 1 and sets msgcellp->errcode
 * 	to ENODATA. If the data check returns true,
 *
 * @param msgcellp  pointer to the message cell structure returned by msgcell_create
 * 	or msgcell_attach.
 * @param timeout specifies how long to wait for a signal before giving up.
 * @return -0 if successful, 1 if error. If error, errno is written to msgcellp->errcode
 * 	errcode ===  ENODATA means no data was detected by the data check function.
 */
int msgcell_timedrec(MSGCELL* msgcellp, const struct timespec* timeout) {
	int retval = 0;

	if (! (*msgcellp->datacheckfuncp)(msgcellp->datap)) {
		if (sem_timedwait(msgcellp->semp, timeout) != 0) {
			if (ETIMEDOUT == retval) {
				if (!(*msgcellp->datacheckfuncp)(msgcellp->datap)) {
					msgcellp->errcode = ENODATA;
					retval = 1;
				}
			} else {
				msgcellp->errcode = errno;
				retval = 1;
			}
		}
	}
	return retval;
}

/**
 * This functions forces the semaphore to zero. (This will cause a
 * subsequent call to msgcell_rec to block.)  It can be defeated
 * by unruly msgcell_xmt callers.
 *
 * @param msgcellp  pointer to the message cell structure returned by msgcell_create
 * 	or msgcell_attach.
 * @return 0 on success, non-zero on failure. If an error is detected, errno is written
 * 	to msgcellp->errcode
 */
int msgcell_reset(MSGCELL* msgcellp) {
	int retval = 0;

	retval = sem_trywait(msgcellp->semp);
	while (retval == 0) {
		retval = sem_trywait(msgcellp->semp);
	}
	if (EAGAIN == errno) {
		// this is good ... just set retval to zero
		retval = 0;
	} else {
		// This is not good. set errcode
		msgcellp->errcode = errno;
	}
	return retval;
}
