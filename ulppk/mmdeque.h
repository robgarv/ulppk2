
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
#ifndef MMDEQUE_H_
#define MMDEQUE_H_

/**
 * @file mmdeque.h
 *
 * @brief A generalized memory mapped double ended queue.  The queue is implemented
 * on top of a memory map atom ... typically a memory mapped file.
 * 
 * The package reads the environment variable MMDQ_DIR_PATH. This should
 * be the directory that will contain the deque files. If you are using
 * sysconfig.c and ini file support (ifile.c) then this environment variable
 * can be overridden by inifile settings.
 *
 */
#include <mmapfile.h>
#include <dqacc.h>

#define MAX_DEQUE_NAME_LEN 32
#define MMDQ_DIR_PATH "MMDQ_DIR_PATH"
#define DEFAULT_MMDQ_DIR_PATH "/var/ulppk/data/deques"


#ifdef __cplusplus
extern "C" {
#endif

MMA_HANDLE* mmdq_create(const char* dequename, ushort item_size, ushort nitems);

MMA_HANDLE* mmdq_open(const char* dequename);

int mmdq_close(MMA_HANDLE* mmdqhp);

int mmdq_isempty(MMA_HANDLE* mmdqhp);
int mmdq_atd(MMA_HANDLE* mmdqhp,void* itemp);
int mmdq_abd(MMA_HANDLE* mmdqhp,void* itemp);
int mmdq_rtd(MMA_HANDLE* mmdqhp,void* itemp);
int mmdq_rbd(MMA_HANDLE* mmdqhp,void* itemp);
int mmdq_reset(MMA_HANDLE* mmdqhp);
DQSTATS* mmdq_stats(MMA_HANDLE* mmdqhp, DQSTATS* dq_statsp);

// Return path to deque directory.
char* mmdq_dequedir();
// Given a deque name, retrieve the full path to
// the memory mapped deque file
char* mmdq_dequepath(char* pathbuff, const char* dequename);

// Return the full path name to the deque file
char* mmdq_dequepath_from_handle(MMA_HANDLE* mmdqhp);


// Packet write and read functions. In packet write, a header and the contents of a structure
// are pushed one byte at a time onto a deque, In packet read, the header is popped off and the 
// number of structure bytes written to the deque is retrieved. This is used to pop the structure bytes
// into a buffer allocated for that purpose. The returned packet buffer must be freed.
int mmdq_write_packet(MMA_HANDLE* mmdqhp, void* packetp, size_t packet_len);
void* mmdq_read_packet(MMA_HANDLE* mmdqhp, size_t* packet_lenp);
#ifdef __cplusplus
}
#endif

#endif /*MMDEQUE_H_*/
