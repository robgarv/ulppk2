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
/* 
 *
 !!! Modified 11 Feb 93 by RCG !!!!

 ll_match_link and other match function declarations.

 Changed current_node type from PLL_NODE to void* to avoid compiler
 errors and warnings with Turbo C++ and Microsoft 7.0 compilers, which
 apply more stringent type checking.

 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "llacc.h"
#include "bool.h"

/**
 * @file llacc.c
 * @brief Linked List Access Facility (llacc)

 This package implements a library of linked list access and manipulation
 functions. A standardized linked list header format is defined.  This format
 is given in the description of llacc.h, which must be included in the code of
 any program unit invoking llacc functions.
*/

 /**
  * @brief Initialize Linked List Header --- ll_init

 This function sets the header to an empty state.

 @param header is a pointer to a linked list header.
 */
void ll_init(PLL_HEAD header) {

	header->ll_fptr = NULL;
	header->ll_bptr = NULL;
	header->ll_cnt = 0;

}
/**
 * @brief Primitive level insert node to list --- ll_ins0

 @param list_node points to the node at which the new node
 is being inserted. (These nodes will appear in the order list_node
 followed by new_node upon completion of the operation.)

 @param new_node  points to the node being inserted.
 */
void ll_ins0(PLL_NODE list_node, PLL_NODE new_node) {

	new_node->link = list_node->link;
	list_node->link = new_node;

}

/**
 * @brief Primitive level pluck object from list --- ll_plu0

 This is a "primitive level" function in the same sense ll_ins0 is; that is,
 it does not manipulate the header structure and probably will never be
 called directly by an application.

 Accepts as input the pointer to a node. The successor to this node is
 removed from the list. Thus if node A is linked to Node B which is linked
 to Node C, and ll_plu0 is called passing a pointer to Node A, Node B
 is removed from the list, producing a list in which Node A is linked to Node C.


 @param list_node Pointer to the predecessor of the
 	 node to be removed from the list.
 @return  pointer to the node which has
 	 been removed from the list.
 */
PLL_NODE ll_plu0(PLL_NODE list_node) {
	PLL_NODE target; /* link word of list_node (points to target node) */

	target = list_node->link; /* get ptr to node to remove */
	if (target != NULL ) { /* there is at least 1 successor to target node */
		list_node->link = target->link; /* write link to target's successor to list_node */
	}

	return (target);

}

/**
 * @brief Add Node to front of list --- ll_addfront

 This function is called by an application to add a buffer to the front of the list.

 @param header is a pointer to a standard linked list header.

 @param node is a pointer to a linked list node to be added to
 the front of the linked list.

*/
void ll_addfront(PLL_HEAD header, PLL_NODE node) {
	PLL_NODE fptr;

	if (header->ll_fptr == NULL ) { /* dealing with empty list */
		node->link = NULL; /* Null out link word to indicate end of list */
		header->ll_fptr = node; /* front and back pointers point to node */
		header->ll_bptr = node;
		header->ll_cnt++;
	} else { /* insert at front of list */
		fptr = header->ll_fptr; /* get ptr to front node */
		header->ll_fptr = node; /* write link to new front node */
		node->link = fptr; /* insert at front of list */
		header->ll_cnt++; /* increment node count */
	}

}

/**
 * @brief Add node to back of list --- ll_addback

 This function is called by an application in order to add a node to the back of
 a linked list.


 @param header is a pointer to a standard linked list header.
 @param node is a pointer to a linked list node to be added to
 the front of the linked list.
 */
void ll_addback(PLL_HEAD header, PLL_NODE node) {
	PLL_NODE bptr;

	if (header->ll_fptr == NULL ) { /* empty list */
		node->link = NULL; /* null out link word to indicate list end */
		header->ll_fptr = node; /* install first node in list */
		header->ll_bptr = node;
		header->ll_cnt++;
	} else { /* insert node at back of list */
		bptr = header->ll_bptr; /* get ptr to back node */
		header->ll_bptr = node; /* write link to new back node */
		bptr->link = node; /* insert new node after former back */
		node->link = NULL; /* terminate the list */
		header->ll_cnt++;
	}

}

/**
 * @brief Get node from front of list --- ll_getfront

 This function is called by an application to obtain the pointer to the object
 at the front of the list, and remove it from the list.

 Where

 @param header is a pointer to a standard linked list header.
 @return Pointer to a linked list node to be added to
 	 the front of the linked list.
 */
PLL_NODE ll_getfront(PLL_HEAD header) {

	PLL_NODE node; /* node retrieved from list */

	if (header->ll_fptr == NULL ) { /* Then list is empty */
		node = NULL; /* return with NULL pointer */
	} else { /* list has at least one item */
		node = header->ll_fptr; /* get ptr to front object */
		if (header->ll_fptr == header->ll_bptr) {
			/* then have one item on list */
			ll_init(header); /* get back to initial condition */
		} else {
			ll_plu0((PLL_NODE) &(header->ll_fptr)); /* pluck node from front of list */
			header->ll_cnt--;
		}
	}
	return (node);
}

/**
 * @brief Get node from back of list --- ll_getback

 This function is called by an application in order to get the pointer to the
 object at the back of the list and delete that object from the list.

 @param header is a pointer to a standard linked list header.
 @return Pointer to a linked list node to be added to
 	 the back of the linked list.
 */
PLL_NODE ll_getback(PLL_HEAD header) {

	PLL_NODE node, pred;

	if (header->ll_fptr == NULL ) { /* list is empty */
		node = NULL;
	} else {
		node = header->ll_bptr; /* get ptr to node on back of list */
		if (header->ll_fptr == header->ll_bptr) { /* only one item on list */
			ll_init(header); /* set list to empty condition */
		} else {
			/*
			 * have more than one node on list ... need to find
			 * back node's predecessor on list
			 */
			pred = ll_search(header, (char*) node, ll_match_link);
			/*
			 * Have predecessor to target node;
			 * pluck target node from the list.
			 */
			ll_plu0(pred); /* remove target node from list */
			/*
			 * now adjust back pointer
			 */
			header->ll_bptr = pred;
			header->ll_cnt--;
		}
	}
	return (node);
}

/**
 * @brief Match on link word --- ll_match_link

 This function is available to applications, but is specifically intended for the use
 of the function ll_getback. This function must find the next to last node on the
 linked list in order to properly set up the back pointer of the linked list header
 and write the value NULL to the link word of this node, which becomes the new
 tail node of the list.

 The search is accomplished by a call to ll_search, passing a pointer to this
 function as described in paragraph.



 @param lpvTargetNode is a pointer to a node, and the search
 criteria. A match will be detected if the link word of the current_node
 is equal to the value of target_node.
 @param current_node is a pointer to the current node under
 examination.
 @return is zero if no match, non-zero otherwise.

 */
unsigned char ll_match_link(void* lpvTargetNode, PLL_NODE current_node) {
	unsigned char flag;
	PLL_NODE target_node;

	target_node = (PLL_NODE) lpvTargetNode;
	if (current_node->link == target_node) { /*match on link word */
		flag = TRUE;
	} else {
		flag = FALSE;
	}
	return (flag);
}

/**
 * @brief Extract (pluck)  node from list --- ll_pluck

 This function is called to remove a specific node from a linked list.
 The node may be installed anywhere in the list.

 PLL_HEAD header [input] is a pointer to a standard linked list header.
 PLL_NODE node [input] is a pointer to the node to be removed from
 the linked list.
 @return [return value] non-zero if the node was not on the
 	 list (failure), zero otherwise (success).

 */
unsigned char ll_pluck(PLL_HEAD header, PLL_NODE node) {
	PLL_NODE predecessor; /* predecessor to target node */
	unsigned char flag;

	/*
	 *  Test for special case 1: node at front of list
	 */

	if (header->ll_fptr == node) {
		flag = FALSE; /* clear error flag */
		ll_getfront(header); /* pull the node off the front of the list */
	} else {
		/*
		 *  Test for special case 2: node at back of list
		 */
		if (header->ll_bptr == node) {
			flag = FALSE; /* clear error flag */
			ll_getback(header); /* pull the node off the back of the list */
		} else {
			/*
			 * Node is either in the middle of the list or not present
			 */
			predecessor = ll_search(header, (char*) node, ll_match_link);

			if (predecessor == NULL ) {
				/*
				 * Node is not on list ... set flag accordingly
				 */
				flag = TRUE;
			} else {
				/*
				 * Node is on list ... set flag and remove it
				 */
				flag = FALSE; /* clear error flag */
				ll_plu0(predecessor); /* remove node from list */
			}
		}
	}
	return (flag);
}

/**
 * @brief Insert node into linked list --- ll_insert

 This function is called by an application to insert a node into the linked
 list so that it immediately follows the given node.


 @param header is a pointer to a standard linked list header.
 @param pnode is a pointer to the intended predecessor
 of the new node
 @param new_node is a pointer to the node to be inserted
 after pnode.
 @return TRUE (non-zero) if an error occurred
 (pnode not found on list)
 */
unsigned char ll_insert(PLL_HEAD header, PLL_NODE pnode, PLL_NODE new_node) {

	unsigned char flag; /* error flag */
	PLL_NODE tnode; /* temp for ptr to predecessor node */

	/*
	 * verify  presence of pnode on list using ll_search
	 */

	tnode = ll_search(header, pnode, ll_match_link);
	if (tnode == NULL ) { /* pnode not on list ... set error flag */
		flag = TRUE;
	} else { /* pnode is on list ... perform the insert */
		ll_ins0(pnode, new_node); /* insert new node on list */
		flag = FALSE; /* clear error flag */
	}
	return (flag);
}

/**
 * @brief Drain linked list of nodes--- ll_drain

 This function pulls all the nodes off a linked list and returns these memory spaces
 to the free pool.

 @param header is a pointer to a linked list header.
 @param returnfunc is a pointer to the user provided memory
 return function. If NULL, the standard destroy node function ll_destroy_node
 is used. ll_destroy_node presumes the node was created by ll_new_node, and
 may do horrible things if that is not the case.
*/
void ll_drain(PLL_HEAD header, void (*returnfunc)(void* buf)) {

	PLL_NODE ptr;

	/* BEGIN */

	ptr = ll_getfront(header); /* get object at front of list */
	while (ptr != NULL ) {

		if (returnfunc != NULL ) {
			(*returnfunc)((void*) ptr); /* return node to pool */
		} else {
			ll_destroy_node(ptr);
		}
		ptr = ll_getfront(header); /* get object at front of list */
	}

}
/**
 * @brief Obtain header front and tail pointers ll_getptrs

 This function is called to read the front and back pointers of the linked
 list header. It returns the node count to the caller.


 @param header is the linked list header structure to be
 examined.
 @param fptr receives the front pointer from the header.
 @param bptr receives the back pointer from the header.
 @return the number of items currently on the list.
*/
ushort ll_getptrs(PLL_HEAD header, PLL_NODE *fptr, PLL_NODE *bptr) {

	*fptr = header->ll_fptr; /* read address of front object */
	*bptr = header->ll_bptr; /* read address of back object */
	return (header->ll_cnt); /* read list count and return */

}

/**
 * @brief Search linked list -- ll_search

 This function is called by an application to search the nodes on the given linked
 list for a node matching a user defined criteria. Note that this function presumes
 that a pointer to a user defined match detection function is provided by the caller.
 One such function, ll_match_link, is defined for the internal use of this package,
 but is available to use by external users also.

 @param header pointer to the linked list header to search.
 @param match_ptr pointer to an object which contains the
 matching criteria. These criteria and the format of this object
 are completely up to the user, as the pointer will simply be
 passed to the user defined match function. This match function,
 of course, must be able to interpret the contents of the match
 criteria object.
 @param match_func is a pointer to a user defined match
 detection function. The calling sequence convention for these
 functions is given as follows:

 flag = function(match_ptr,node);

 where match_ptr is as defined above, and

 unsigned char flag [return value] is non-zero if a match was detected and
 zero otherwise.

 PLL_NODE node [input] is a pointer to a node of the linked list being
 searched.

@param header pointer to linked list header
@param match_ptr pointer to application defined matching criteria structure
@param match_func pointer to application defined matching function
@return is a pointer to the first node on the
 	 list satisfying the user defined match criteria. A value of NULL
 	 is returned if no match was found.

*/
PLL_NODE ll_search(PLL_HEAD header, void* match_ptr,
		unsigned char (*match_func)(void*, PLL_NODE)) {

	PLL_NODE cptr; /* pointer to current node */
	unsigned char match; /* match detected flag */

	match = FALSE; /* initialize match detected flag */
	cptr = header->ll_fptr; /* point to object at front of list */
	while ((cptr != NULL )&& (!match)){ /* while more objects to scan */
		match = (*match_func)(match_ptr,cptr); /* check for a match */
		if (!match) { /* match not detected */
			cptr = cptr->link; /* get next node on list */
		}
	}
	/*
	 * At this point, we are either at the end of the list or we have found
	 * the first node on the list which matches the search criteria.
	 */
	return (cptr);
}

/**
 * @brief ll_iterate
 * 
 * Give the pointer to an LL_NODE structure, return the 
 * next item in the list.
 * 
 * 	@param snodeptr	Node at which to start iteration
 * 	@return Pointer to LL_NODE that is the next item in the list
 * 
 */
PLL_NODE ll_iterate(PLL_NODE snodeptr) {
	if (snodeptr == NULL ) {
		return NULL ;
	} else {
		return snodeptr->link;
	}
}

/**
 * ll_new_node
 *
 * Create a new linked list node, linking in a copy of the user
 * data provided by the caller. Note that because the user
 * data is copied, the user is free to use automatic variable
 * buffers, reuse the memory buffer,  or otherwise dispose
 * of the memory is it requires.
 *
 * A call to ll_destroy_node will properly deallocate the node
 * and linked data.
 *
 *
 * @param user_dataptr pointer to user specified memory.
 * @param data_size size of the user data buffer in bytes
 * @return Pointer to a new LL_NODE structure with user data
 * 		properly linked in.
 */
PLL_NODE ll_new_node(void* user_dataptr, ulong data_size) {
	PLL_NODE the_node;
	void* buff;

	buff = (void*) malloc(data_size);
	memcpy(buff, user_dataptr, data_size);
	the_node = (PLL_NODE) calloc(1, sizeof(LL_NODE));
	the_node->data = buff;
	return the_node;
}

/*
 * ll_destroy_node
 * 
 * Returns a node and its user data memory to the heap.
 */
void ll_destroy_node(PLL_NODE nodeptr) {
	if (nodeptr != NULL ) {
		if (nodeptr->data != NULL ) {
			free(nodeptr->data);
		}
		free(nodeptr);
	}
}
