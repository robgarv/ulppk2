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

/*
 * msgcell.h
 *
 *  Created on: Dec 21, 2012
 *      Author: robgarv
 *
 * A "message cell" is a simple construct that facilitates inter process communications.
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
 *
 */

#ifndef MSGCELL_H_
#define MSGCELL_H_

/**
 * @file msgcell.h
 *
 * @brief Declarations for message cell facility.
 *
 * See msgcell.c for details.
 *
 */
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#define MAX_MSGCELL_NAME 64
#define MSGCELL_NODATA_AVAIL 0
#define MSGCELL_DATA_AVAIL 1

/**
 * The message cell data check function is provided by the caller
 * to test whether valid data has been written to the message cell.
 * It receives a pointer to the application defined data structure
 * provided by message cell creation and attachment functions, and
 * returns 1 if valid data is detected. See msgdeque.c for an
 * example of how this is used.
 */
typedef int MSGCELL_DATACHECK_FUNC(void* datap);

/**
 * Message cell structure
 */
typedef struct _MSGCELL {
	char name[MAX_MSGCELL_NAME];		///< Name of the message cell
	int errcode;						///< Error code (errno)
	sem_t* semp;						///< Pointer to a semaphore
	void* datap;						///< Pointer to application defined data structure
	MSGCELL_DATACHECK_FUNC* datacheckfuncp;	///< Pointer to data check function
} MSGCELL;

MSGCELL* msgcell_create(const char* name, uint permissions, void* datap, MSGCELL_DATACHECK_FUNC* datacheckfuncp);
MSGCELL* msgcell_attach(const char* name, void* datap, MSGCELL_DATACHECK_FUNC* datacheckfuncp);
int msgcell_delete(MSGCELL* msgcellp);
int msgcell_close(MSGCELL* msgcellp);
void* msgcell_get_data_pointer(MSGCELL* msgcellp);
int msgcell_send(MSGCELL* msgcellp);
int msgcell_rec(MSGCELL* msgcellp);
int msgcell_timedrec(MSGCELL* msgcellp, const struct timespec* timout);
int msgcell_reset(MSGCELL* msgcellp);

#endif
