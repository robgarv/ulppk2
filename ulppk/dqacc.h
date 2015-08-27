
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


#ifndef DQACC_DEF
#define DQACC_DEF
    
#include <sys/types.h>
#include <bool.h>
#define PBYTE unsigned char*

	/**
	 * @file dqacc.h
	 *
	 * @brief Double ended queue access support.
	 *
	 * Deque is pronounced "deck".
	 *
	 * Double ended queue (deque) features<br>
	 * <ol>
	 * 	<li>Fixed maximum capacity</li>
	 *  <li>Insertions at top or bottom of deque</li>
	 *  <li>Removals from top or bottom of deque</li>
	 *  <li>Supports memory mapped IO implementation (see mmdeque.c)</li>
	 * </ol>
	 *
	 * FIFOs are implemented by, for example adding items to the bottom and removing
	 * from the top. A stack can be implemented by adding to the bottom (or top) and
	 * removing from the bottom (or top).
	 *
	 * For deques implemented in UNIX memory mapped files, the field
	 * dqbuffx has been added to the classic deque header structure.
	 * If the memmaped bit is set, then deque algorithms are to use
	 * rules apropos to memory mapped files. Otherwise, classic behavior
	 * is followed.<p>
	 * See function dq_init_memmap
	*/

	/**
	 * Deque header structure.
	 */
    typedef struct {
    	int dq_open;		///< FALSE means deque not ready for use
    	int dqglobal_heap;	///< TRUE means allocate from global heap, FALSE from local
        ushort dqslots;     ///< max number of items in deque
        ushort dquse;       ///< number of items currently in deque
        ushort dqtop;       ///< index to top of deque
        ushort dqbottom;        ///< index to bottom of deque
        ushort dqitem_size; ///< item size in bytes
        ushort memmapped : 1;	///< TRUE => deque in memory mapped IO space
        ushort buffctrl : 1;	///< TRUE => deque buffer was provided via dq_init_butter ... do NOT free
        void* dqbuff;           ///< ptr to buffer containing deque slots
        size_t dqbuffx;		///<  Index relative to first byte of the header of slot buffer
    } DQHEADER;
    
    /**
     * Deque status structure. Used to determine the current state of the
     * double ended queue.
     */
    typedef struct _dq_statbuff {
    	int dq_open;			///< TRUE if open
    	ushort dqitem_size;		///< Size in bytes of items on the deque
    	ushort dqslots;			///< Capacity of the deque in items
    	ushort dquse;			///< Number of deque slots in use (items in the deque)
    	ushort memmapped : 1;	///< 1 if deque is memory mapped
    	ushort buffctrl : 1;	///< 1 if deque buffer was allocated from heap
    } DQSTATS;
    	

// Note that dqbuffx pertains to deques set up in memory mapped files only.

    typedef  DQHEADER * PDQHEADER;

#ifdef __cplusplus
extern "C" {
#endif
    void dq_init(ushort deque_size, ushort item_size, PDQHEADER header);
    void dq_init_buffer(ushort deque_size, ushort item_size, void* buffp, PDQHEADER header);
    void dq_init_memmap(ushort deque_size, ushort item_size, size_t index, PDQHEADER header);
    void dq_close(PDQHEADER header);
    int dq_isempty(PDQHEADER header);
    int dq_atd(PDQHEADER deque,void* itemp);
    int dq_abd(PDQHEADER deque,void* itemp);
    int dq_rtd(PDQHEADER deque,void* itemp);
    int dq_rbd(PDQHEADER deque,void* itemp);
    DQSTATS* dq_stats(PDQHEADER dequep, DQSTATS* dq_statsp);
    
#ifdef __cplusplus
}
#endif
#endif
