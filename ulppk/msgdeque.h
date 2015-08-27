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
 * msgdeque.h
 *
 *  Created on: Dec 22, 2012
 *      Author: robgarv
 */

/**
 * @file msgdeque.h
 * @brief Message Deque Facility for Inter-process Communications
 *
 *  This module provides a simple process-to-process communications
 *  mechanism using memory mapped dequeues and message cells (POSIX
 *  semaphores). For the message cell implementation, see msgcell.h
 *  and msgcell.c
 */

#ifndef MSGDEQUE_H_
#define MSGDEQUE_H_

#include <sys/types.h>

#include <mmdeque.h>
#include <mmapfile.h>
#include <msgcell.h>

/**
 * @brief Message deque structure definition.
 *
 * This is a "application defined data structure" provided to
 * message cell creation and attachment functions.
 * (See msgcell.c)
 */
typedef struct _MSGDEQUE {
	MMA_HANDLE* deque;			///< Memory mapped atom handle of the memory mapped deque
	MMA_HANDLE* lock;			///< Memory mapped atom handle of the memory mapped lock file
} MSGDEQUE;

MSGCELL* msgdeque_create(const char*name, uint permissions, ushort item_size, ushort nitems);
MSGCELL* msgdeque_create_byte_stream(const char*name, uint permissions,ushort byte_capacity);
MSGCELL* msgdeque_attach(const char *name);
int msgdeque_datacheck(void* datap);
int msgdeque_reset(MSGCELL* msgcellp);
int msgdeque_send(MSGCELL* msgcellp, void* sendp);;
void* msgdeque_rec(MSGCELL* msgcellp);
int msgdeque_send_byte_stream(MSGCELL* msgcellp, void* pdata, size_t datalen);
void* msgdeque_rec_byte_stream(MSGCELL* msgcellp, size_t* bytes_received);


#endif /* MSGDEQUE_H_ */
