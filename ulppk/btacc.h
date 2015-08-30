
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
#ifndef _BTACC_H
#define _BTACC_H
    /**
     * @file btacc.h
     *
     * @brief Declarations and definitions for btacc
     *
     * This include file contains declarations necessary to invoke the
     * functions of the binary tree handler package btacc.c
    */
#include <sys/types.h>
#include <bool.h>

	/**
	 * A tree node structure. All nodes on the tree must incorporate
	 * a TREE_NODE. The value of the node and its semantics are
	 * supplied by applications layer definitions and code.
	 * See appenv.h for an example of use.
	 */
    typedef struct _tree {
        struct _tree *tr_left;		///< left branch pointer
        struct _tree *tr_right;		///< right branch pointer
        struct _tree *tr_parent;	///< pointer to parent
        ushort tr_count;			///< Number of times the application value was inserted.
    } TREE_NODE;        
 
 #ifdef __cplusplus
 extern "C" {
 #endif
    typedef TREE_NODE * PTREE_NODE;

	int bt_add(PTREE_NODE *tree, /*pointer to tree node */
                void* new,      /* pointer to new node */
				int (*compare_function)(PTREE_NODE cnode,void* new));


	int bt_insert(PTREE_NODE tree,   /*pointer to pointer to tree node */
                void* new,      /* pointer to new node */
				int (*compare_function)(PTREE_NODE cnode, void* new));

    PTREE_NODE bt_search(PTREE_NODE tree,void* criteria,
					int (*compare_function)(PTREE_NODE tree,void* criteria));

    void bt_del(PTREE_NODE *tree,PTREE_NODE node);
    PTREE_NODE bt_inorder(PTREE_NODE tree, void* argbuff,
					int (*action)(PTREE_NODE tree,void* argbuff));

    PTREE_NODE bt_leftmost(PTREE_NODE subtree);
    PTREE_NODE bt_rightmost(PTREE_NODE subtree);
    void bt_replace(PTREE_NODE *tree,          // ptr to root pointer
        PTREE_NODE oldnode,                    // record to replace
        PTREE_NODE newnode);                   // new record


#ifdef __cplusplus
 }
#endif


#endif
