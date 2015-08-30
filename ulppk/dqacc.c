
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
//
// 1996 by Rob Garvey
//

#include <string.h> 
#include <stdlib.h>
#include "dqacc.h"
/**
 @file dqacc.c

 @brief Double Ended Queue (deque) Access -- dqacc.

    This package implements a collection of functions by which double ended queues
    or deques may be defined, initialized, and accessed. A double ended queue consists
    of a header structure and an array of slots. The size of the array is detemined by
    the maximum number of items to be pushed into the deque and the size of the
    item type.
    

*/

void* map_slot(PDQHEADER deque,ushort index) {

PBYTE lpslot;
PBYTE pb;

/* BEGIN */

if (deque->memmapped) {
	pb = ((PBYTE)deque) + deque->dqbuffx;
} else {
	pb = ((PBYTE)deque->dqbuff);
} 
lpslot = pb +  (size_t)(index * (deque->dqitem_size));

return ((void*)lpslot);

/* END */

}

/**
 * The "classic" dq_init form. Deque slot buffer is completely managed by
 * dqacc ... it is allocated from the heap and is freed by dq_close.
 * @param deque_size Number of items to be stored in the new deque.
 * @param item_size Size of each of the items. (Consider this to be a maximum size.)
 * @param header Pointer to the deque header to be initialized.
 */
void dq_init(ushort deque_size, ushort item_size, PDQHEADER header) {

ushort buffsize;

/* BEGIN */

	header->dqslots = deque_size;
	header->dquse = 0;
	header->dqtop = 0;
	header->dqbottom = 0;
	header->dqitem_size = item_size;
	header->memmapped = 0;
	header->buffctrl = 0;
	buffsize = item_size * deque_size;
	header->dqbuff = (void*)calloc(buffsize,1);
	header->dq_open = TRUE;

/* END */

}

/**
 * Sometimes, an application just wants to use its own buffer for the deque slots.
 * This form is given a buffer managed by the applications layer ... dq_close 
 * will NOT free the buffer provided.
 * @param deque_size Number of items to be stored in the new deque.
 * @param item_size Size of each of the items. (Consider this to be a maximum size.)
 * @param buffp Pointer to buffer to use to store deque items
 * @param header Pointer to the deque header to be initialized.
 */
void dq_init_buffer(ushort deque_size, ushort item_size, void* buffp, PDQHEADER header) {
    ushort buffsize;
    
    header->memmapped = 0;
    header->buffctrl = 1;
	header->dqslots = deque_size;
	header->dquse = 0;
	header->dqtop = 0;
	header->dqbottom = 0;
	header->dqitem_size = item_size;
	buffsize = item_size * deque_size;
	header->dqbuff = buffp;
	header->dq_open = TRUE;
}

/**
 * This form of dq_init supports initialization in deque headers in memory 
 * mapped I/O regions. In this case, the slot buffer is an array in memory
 * mapped I/O space. It must be located at a higher address than the deque
 * header. The index passed by the caller is relative to the start of this 
 * deque header.
 * @param deque_size Number of items to be stored in the new deque.
 * @param item_size Size of each of the items. (Consider this to be a maximum size.)
 * @param index Index relative to the start of the header of the first byte in the
 *  memory mapped region to store item data.
 * @param header Pointer to the deque header to be initialized. This header
 *  is located within a memory mapped region.
 */
void dq_init_memmap(ushort deque_size, ushort item_size, size_t index, PDQHEADER header) {
	dq_init(deque_size, item_size, header);		// perform classic initialization
	free(header->dqbuff);				// free the allocated deque buffer
	header->dqbuff = NULL;				// NULL the buffer pointer
	header->memmapped = 1;				// set memory mapped region bit
	header->dqbuffx = index;			// index is relative to first byte of header
}

/**
 * Close a double ended queue.
 *
 * If the deque uses a user supplied buffer, it is up to the user to release
 * that memory (if required). Such deques are created by dq_init_buffer.
 *
 * If the deque was created using the dq_init function, then the deque buffer
 * is released by this function.
 *
 * If the deque was created using dq_init_memap, not memory resources are released.
 * See mmdqacc.c for details.
 *
 * @param header Pointer to the double ended queue header.
 */
void dq_close(PDQHEADER header) {
	if (!header->dq_open) {				// deque not open .. don't close it!
		return;
	}
	header->dq_open = FALSE;			// set dq_open FALSE
	if ((!header->memmapped) && (!header->buffctrl)) {
		if (header->dqbuff != NULL) {
			free(header->dqbuff);		// call free
			header->dqbuff = NULL;
		}
	}
}

/**
 * Determines if the double ended queue is empty.
 * @param deque Deque header to be checked.
 * @return TRUE if empty, FALSE otherwise
 */
int dq_isempty(PDQHEADER deque) {

	if (deque->dquse == 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * Add to the top of the deque.
 *
 * @param deque Pointer to the deque header.
 * @param item Pointer to the item to be added
 * @return 0 on success, non-zero if the deque is full
 */
int dq_atd(PDQHEADER deque,void* item) {

int flag;          // error flag ... TRUE implies error 
void* lpslot;           // ptr to 1st byte of destination slot

/* BEGIN */

if (!deque->dq_open) {			// deque not open
	return TRUE;				// indicate error
}
if (deque->dqslots > deque->dquse) {            // have room in deque 
    if (deque->dquse != 0) {            // deque not empty
        deque->dqtop = (deque->dqtop+1) % deque->dqslots;  // adjust top ptr index
    } 
    deque->dquse++;             // increment use count
    /*
     * Copy item into slot at slot index deque->dqtop.
    */
    lpslot = map_slot(deque,deque->dqtop);      // compute ptr to slot
    memcpy(lpslot,item,deque->dqitem_size);     // copy item to deque
    flag = FALSE;                       // indicate no error
} else {                                // deque is full
    flag = TRUE;
}

return (flag);

/* END */

}

/**
 * Add to the bottom of the deque.
 *
 * @param deque Pointer to the deque header.
 * @param item Pointer to the item to be added
 * @return 0 on success, non-zero if the deque is full
 */

int dq_abd(PDQHEADER deque,void* item) {

int flag;          // error flag ... TRUE implies error 
void* lpslot;           // ptr to 1st byte of destination slot

/* BEGIN */

if (!deque->dq_open) {			// deque not open
	return TRUE;				// indicate error
}
if (deque->dqslots > deque->dquse) {            // have room in deque 
    if (deque->dquse != 0) {            // deque not empty
        /*
         * Adjust deque bottom ptr index
        */
        deque->dqbottom = (deque->dqbottom + deque->dqslots - 1) % 
            deque->dqslots;
    } 
    deque->dquse++;             // increment use count
    /*
     * Copy item into slot at slot index deque->dqbottom.
    */
    lpslot = map_slot(deque,deque->dqbottom);       // compute ptr to slot
    memcpy(lpslot,item,deque->dqitem_size);     // copy item to deque
    flag = FALSE;                       // indicate no error
} else {                            // deque is full
    flag = TRUE;
}

return (flag);

/* END */

}

/**
 * Remove an item from the top of the deque.
 * @param deque Pointer to deque header
 * @param item Pointer to memory are of at least deque item_size bytes,
 * @return 0 if successful, non-zero otherwise.
 */
int dq_rtd(PDQHEADER deque,void* item) {

int flag;          // error flag ... TRUE implies error 
void* lpslot;           // ptr to 1st byte of destination slot
int use;

/* BEGIN */

if (!deque->dq_open) {			// deque not open
	return TRUE;				// indicate error
}
if (deque->dquse > 0) {             // deque not empty
    lpslot = map_slot(deque,deque->dqtop);      // compute ptr to slot
    memcpy(item,lpslot,deque->dqitem_size);     // copy item to target area
    flag = FALSE;                       // indicate no error
    /*
     * Adjust bottom ptr index
    */
    if (deque->dquse != 1) {
        deque->dqtop = (deque->dqtop + deque->dqslots - 1) % deque->dqslots;
    }
    flag = FALSE;
    use = deque->dquse;
    deque->dquse = use - 1;         // decrement slots in use count
} else {                            // attempt to remove from empty deque
    flag = TRUE;
}

return (flag);

/* END */

}

/**
 * Remove an item from the bottom of the deque.
 * @param deque Pointer to deque header
 * @param item Pointer to memory are of at least deque item_size bytes,
 * @return 0 if successful, non-zero otherwise.
 */

int dq_rbd(PDQHEADER deque,void* item) {

int flag;          // error flag ... TRUE implies error 
void* lpslot;      // ptr to 1st byte of destination slot
int use;

/* BEGIN */

if (!deque->dq_open) {			// deque not open
	return TRUE;				// indicate error
}
if (deque->dquse > 0) {             // deque not empty
    lpslot = map_slot(deque,deque->dqbottom);       // compute ptr to slot
    memcpy(item,lpslot,deque->dqitem_size);     // copy item to target area
     /*
     * Adjust bottom ptr index
    */
    if (deque->dquse != 1) {
        deque->dqbottom = (deque->dqbottom + 1)  % deque->dqslots;
    }
    flag = FALSE;
    use = deque->dquse;
    deque->dquse = use - 1;         // decrement slots in use count
    flag = FALSE;
} else {                            // attempt to remove from empty deque
    flag = TRUE;
}

return (flag);

/* END */

}

/**
 * return status information from the given deque header.
 * If dq_statsp is not NULL, the status data is written
 * to the deque status buffer provided by the caller.
 * Otherwise, memory is allocated from the heap (and must
 * be returned by the caller.)
 * @param dequep Pointer to deque header
 * @param dq_statsp Pointer to a DQSTATS structure. If NULL,
 *  memory is allocated from the heap and must be freed by
 *  the caller.
 * @return pointer to DQSTATS structure containing the
 *  deque status information.
 */
DQSTATS* dq_stats(PDQHEADER dequep, DQSTATS* dq_statsp) {
	if (dq_statsp == NULL) {
		dq_statsp = (DQSTATS*)malloc(sizeof(DQSTATS));
	}
	memset(dq_statsp, 0, sizeof(DQSTATS));
	
	dq_statsp->dq_open = dequep->dq_open;
	dq_statsp->dqitem_size = dequep->dqitem_size;
	dq_statsp->dqslots = dequep->dqslots;
	dq_statsp->dquse = dequep->dquse;
	dq_statsp->memmapped = dequep->memmapped;
	dq_statsp->buffctrl = dequep->buffctrl;
	return dq_statsp;
}

