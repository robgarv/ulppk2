
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
 * @file llacc.h
 *
 * @brief This include file contains all definitions necessary for utilitizing the
 * linked list handling package llacc.c.
 * 
 * llacc provides function for management and access of linked lists of nodes.
 *
 * A linked list node contains a pointer to a user defined data buffer.
*/

#ifndef DEF_LLACC_H
#define DEF_LLACC_H

#include <sys/types.h>

	/**
	 * Linked list node definition.
	 */
    typedef struct node_tag {
        struct node_tag  *link;	///< Points to next node on the list
        void* data;				///< user specified data
    } LL_NODE;
    
    typedef LL_NODE  * PLL_NODE;


    /**
     * Definition of a linked list header. A linked list is in the
     * empty state when ll_fptr == ll_bptr == NULL and ll_cnt = 0
     */
    typedef struct {
        PLL_NODE ll_fptr;       ///< pointer to front object on list
        PLL_NODE ll_bptr;   	///< pointer to back object on list
        ushort ll_cnt;  		///< count of objects on list
    } LL_HEAD ;
    
    typedef LL_HEAD  * PLL_HEAD; 

    void ll_init(PLL_HEAD header);
    void ll_ins0(PLL_NODE list_node,PLL_NODE new_node);
    PLL_NODE ll_plu0(PLL_NODE list_node);
    void ll_addfront(PLL_HEAD header,PLL_NODE node);
    void ll_addback(PLL_HEAD header,PLL_NODE node);
    PLL_NODE ll_getfront(PLL_HEAD header);
    PLL_NODE ll_getback(PLL_HEAD header);

	PLL_NODE ll_search(
		PLL_HEAD header,
		void* match_ptr,
		unsigned char (*match_func) (void* match_ptr,PLL_NODE current_node)
	);

	unsigned char ll_match_link(void* target_node, PLL_NODE current_node);
	unsigned char ll_pluck(PLL_HEAD header,PLL_NODE node);
	unsigned char ll_insert(PLL_HEAD header,PLL_NODE pnode,PLL_NODE new_node);
	void ll_drain (PLL_HEAD header,void (*returnfunc)(void* buf));
	ushort ll_getptrs(PLL_HEAD header,PLL_NODE *fptr, PLL_NODE *bptr);
	PLL_NODE ll_iterate(PLL_NODE snodeptr);
	PLL_NODE ll_new_node(void* user_dataptr, ulong data_size);
	void ll_destroy_node(PLL_NODE nodeptr);

#endif


