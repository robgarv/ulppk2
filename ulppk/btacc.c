
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
#include <stdio.h>
#include <string.h>
#include "btacc.h"

/**
 * @file btacc.c
 * @brief Binary Tree Access: btacc .

    This package provides facilities for manipulating binary trees. Records
    of arbitray form and content may be added, searched for, or removed from
    binary trees by this package.

    The following is a summary of user callable routines:
<ul>
    <li>bt_add : add node to binary tree</li>
    <li>bt_del : delete node from binary tree</li>
	<li>bt_search: search binary tree based on</li>
    <li>bt_leftmost: return pointer to left most node of tree</li>
    <li>bt_rightmost: return pointer to right most node of tree</li>
    <li>bt_inorder: In order traversal of the binary tree executing an action on
    	each node visited</li>
    <li>bt_replace: Use with caution. Replace a node in a binary tree with a new node.
    	This is a raw replace: It is up to the caller to insure that the
    	ordering of the tree is not violated. (That is, the value of the
    	old node must be consistent in sort order with the value of the new node).
    	If this constraint is unacceptable or infeasible, simply do a bt_del of
    	the old node followed by a bt_add of the new.
    </li>
</ul>
    User supplied comparison routines are supplied to bt_add and bt_search.
    These routines obey the following call convention:

    result = function(tree_node,arglist);

    Where:

            ushort result [return value] is a result code:
<ul>
            <li>-1 => current tree node has key less than record key</li>
            <li>0 => current tree node has key equal to current record key</li>
            <li>1 => current tree node has key greater than current record key</li>
</ul>
        TREE_NODE tree [input] is a pointer to a tree node.

        void* arglist [input] is a pointer to a user defined argument record.
*/


/**
 * @brief Add a node to the binary tree in proper order -- bt_add.
 *
 * This function is called to add an element to the binary tree. The insertion
 * process is governed by the result code returned from the user supplied
 * comparison function:
 * @return result is the result code from the user supplied
 * comparison function.
 * @param tree is the pointer to the root tree node
 * @param new is a pointer to a user defined node to be added
 *          to the tree structure. It is presumed that one or more fields
 *          of this node are used by the comparison function to determine
 *          where the insertion should occur within the tree.
 * @param  compare_function is a pointer to the user defined comparison function.
 *
 * @return result is the result code from the user supplied comparison function.
 * <ul>
 * <li>result = -1 => left branch insertion</li>
 * <li>result = 1 => right branch insertion</li>
 * <li>result = 0 => duplicate entry, insertion not performed but occurrance
 * count of tree node header incremented</li>
 * </ul>
 *
 *
 */
int bt_add(PTREE_NODE *tree, /*pointer to tree node */
	void* new, /* pointer to user defined node */
	int (*compare_function)(PTREE_NODE cnode, void* new)) {

	PTREE_NODE newnode; /* node to be added viewed as tree node */
	int result; /* comparison function result code */
	int rslt; /* code returned from bt_insert */

	/* BEGIN */

	newnode = (PTREE_NODE) new; /* cast user's node as tree node */
	if (*tree == NULL ) { /* Empty tree ... add node */
		*tree = newnode; /* link new node to designated tree ptr */
		newnode->tr_left = NULL; /* Setup linkages ... left leaf */
		newnode->tr_right = NULL; /* right leaf */
		newnode->tr_parent = NULL; /* 1st node has no parent */
		result = -1; /* pretend it was a left leaf insertion */
	} else {
		/*
		 * Install new node in tree. Call user supplied comparison function to
		 * determine if left or right subtree exploration is required.
		 */

		result = (*compare_function)(*tree, newnode);
		if (result < 0) { /* explore left subtree */
			if ((*tree)->tr_left == NULL ) { /* at the end of the left subtree */
				newnode->tr_parent = *tree; /* link to parent */
				newnode->tr_left = NULL; /* NULL out newnode's leafs */
				newnode->tr_right = NULL;
				(*tree)->tr_left = newnode; /* install in left leaf field */
			} else {
				rslt = bt_insert((*tree)->tr_left, new, compare_function);
				result = rslt;              // for return of result code
			}
		} else if (result > 0) { /* explore right subtree */
			if ((*tree)->tr_right == NULL ) { /* at the end of the right subtree */
				newnode->tr_parent = *tree; /* link to parent */
				newnode->tr_left = NULL; /* NULL out newnode's leafs */
				newnode->tr_right = NULL;
				(*tree)->tr_right = newnode; /* install in left leaf field */
			} else {
				rslt = bt_insert((*tree)->tr_right, new, compare_function);
				result = rslt;              // for return of result code
			}
		} else {
			/*
			 * Result = 0 ,Got a double hit on the 1st node ... inc use counter
			 */
			(*tree)->tr_count++;
		}
	}
	return (result);
}
/**
 *
 * @brief Insert node into binary tree --- bt_insert.
 *
 *   This function is called by bt_add to insert an element into the binary tree. The insertion
 *   process is governed by the result code returned from the user supplied
 *   comparison function:
 * <ul>
 *      <li>result = -1 => left branch insertion</li>
 *      <li>result = 1 => right branch insertion</li>
 *      <li>result = 0 => duplicate entry, insertion not performed but occurrance
 *			count of tree node header incremented</li>
 * </ul>
 *
 * @param tree is the pointer to a tree node.
 * @param new is a pointer to a user defined node to be added
 *			to the tree structure. It is presumed that one or more fields
 *			of this node are used by the comparison function to determine
 *			where the insertion should occur within the tree.
 * @param	compare_function is a pointer to the user
 *			defined comparison function.
 * @return One of the comparison function result codes described above.
 */
int bt_insert(PTREE_NODE tree, /*pointer to pointer to tree node */
	void* new, /* pointer to user defined arg list */
	int (*compare_function)(PTREE_NODE cnode, void* new)) {

	int result; /* returned from user comparison fn */
	PTREE_NODE newnode; /* new node viewed as a tree node */

	/* BEGIN */

	newnode = (PTREE_NODE) new; /* cast new node as PTREE_NODE */
	result = (*compare_function)(tree, newnode); /* perform comparison */
	if (result < 0) { /* examine left subtree */
		if (tree->tr_left == NULL ) { /* at end of left subtree ... insert */
			newnode->tr_parent = tree; /* link to newnode's parent */
			newnode->tr_left = NULL; /* NULL out left and right leaf links */
			newnode->tr_right = NULL;
			tree->tr_left = newnode; /* install newnode in left subtree */
		} else { /* travel down left subtree */
			result = bt_insert(tree->tr_left, new, compare_function);
		}
	} else if (result > 0) { /* examine right subtree */
		if (tree->tr_right == NULL ) { /* at end of right subtree */
			newnode->tr_parent = tree; /* link newnode's parent */
			newnode->tr_left = NULL; /* NULL out left and right leaf links */
			newnode->tr_right = NULL;
			tree->tr_right = newnode; /* install newnode in right subtree */
		} else { /* travel down right subtree */
			result = bt_insert(tree->tr_right, new, compare_function);
		}
	} else {
		/*
		 * Have a double hit ... increment occurrance count in tree node header
		 */

		tree->tr_count++;
	}
	return (result);
}
            

        
/**
 *
 * @brief Delete from binary tree -- bt_del.

    This function is called to delete an item from a binary tree. The caller must
    provide a pointer to the node to be deleted. Typically, this node would be
    discovered by means of the binary tree search method, bt_search.


   @param tree is a pointer to the root of the tree.
   @param node is a pointer to the node to be removed.
 */

    void bt_del(PTREE_NODE  *tree,PTREE_NODE node){

    PTREE_NODE parent;      /* pointer to a parent node */
    PTREE_NODE repnode; /* ptr to node chosen to replace node to delete */
    PTREE_NODE subtree; /* ptr to a subtree hanging off node to delete */
    

    /* BEGIN */

    if (*tree == NULL) {                /* asked to remove from empty tree */
        return;
    }

    /*
     * Deleting an arbitrary node without massive re-arrangement of the tree
     * depends on the following observation:
     *  IF there is a left subtree depending on the node, obtaining the 
     *      rightmost leaf of that subtree provides a node whose tag 
     *      value is between those of the current left and right children 
     *      of the node we are deleting. Call this node the "replacement node".
     *      (It thus has a value that suits it to occupy the position of the
     *       node we are deleting.)
     *      IF there is a left subtree to the replacement node, its left
     *          child takes its place as the right child of the replacement
     *          node's parent.
     *  ELSE IF there is a right subtree, obtaining the leftmost leaf
     *      of that subtree produces a similar node.
     *      IF there is a right subtree to the replacement node, its
     *          right child takes its place as the left child of the replacement
     *          node's parent.
     *  ELSE no subtrees dependent on the node to be deleted exist,
     *      our job is childlike in simplicity in that all we have to
     *      do is NULL out the appropriate linkage in the parent.
    */

    if (node->tr_left != NULL) {            /* we have a left subtree */
        subtree = node->tr_left;        /* get a pointer to it */
        repnode= bt_rightmost(subtree); /* get the rightmost  as replacement */
        /*
         * If repnode is NULL, then the subtree node is the rightmost
         * node of the left subtree, and thus becomes the replacement
         * node.
         */
        if (repnode == NULL) {
            repnode = subtree;
        }
        /*
         * We are at the rightmost node of the subtree left of the node to be
         * removed. The left child of this node (repnode) becomes the right
         * child of repnode's parent. Note the left child may be NULL if the
         * is no left subtree hanging off repnode. (In other words, if it has one,
         * repnode's left subtree becomes its parent's right subtree.) HOWEVER
         * If repnode's parent is node (the node we're deleting), then repnode
         * must retain its left subtree. 
        */
        parent = repnode->tr_parent;        /* get pointer to its parent */
        if (parent != node) {           /* adjust parent's right linkage */
            parent->tr_right = repnode->tr_left;    /* parents right becomes repnodes left */
            if (parent->tr_right != NULL) {
                parent->tr_right->tr_parent = parent;   /* adjust child's parent ptr */
            }
        }
        /* 
         * Now replace node with repnode ... step 1 adjust node's parent's
         * linkages. (node's parent becomes repnode's parent)
        */
        if (node->tr_parent == NULL) {      /* replacing the root node */
            *tree = repnode;        /* adjust root ptr accordingly */
        } else {
            if (node->tr_parent->tr_left == node) {
                node->tr_parent->tr_left = repnode;
            } else {
                node->tr_parent->tr_right = repnode;
            }
        }
        /*
         * Step 2: adjust repnode's linkages. Repnode inherits the left and right
         * subtrees of node (the node we are deleting). However, repnode may
         * be the left child of node, in which case we must leave its left pointer
         * alone.
        */
        repnode->tr_parent = node->tr_parent;   /* write parent link */
        if (node->tr_left != repnode) {         /* repnode is left child of node */
            repnode->tr_left = node->tr_left;   /* repnode gets node's left subtree */
        }
        repnode->tr_right = node->tr_right; /* write the right child link */
        /*
         * Step 3: Adjust parent linkages of repnode's new children
        */
        if (repnode->tr_right != NULL) {
            repnode->tr_right->tr_parent = repnode;
        }
        if (repnode->tr_left != NULL) {
            repnode->tr_left->tr_parent = repnode;
        }
    } else if (node->tr_right != NULL) {        /* at least we have a right subtree */
        subtree = node->tr_right;       /* get a pointer to it */
        repnode= bt_leftmost(subtree);  /* get the leftmost member */
        /*
         * If repnode is NULL, then the subtree node is the leftmost
         * node of the right subtree, and thus becomes the replacement
         * node.
         */
        if (repnode == NULL) {
            repnode = subtree;
        }
        /*
         * We are at the leftmost node of the subtree right of the node to be
         * removed. The right child of this node (repnode) becomes the left
         * child of repnode's parent. Note the right child may be NULL if there
         * is no right subtree hanging off repnode. (In other words, if it has one,
         * repnode's right subtree becomes its parent's left subtree.) HOWEVER
         * If repnode's parent is node (the node we're deleting), then repnode
         * must retain its right subtree. 
        */
        parent = repnode->tr_parent;        /* get pointer to its parent */
        if (parent != node) {           /* adjust parent's left linkage */
            parent->tr_left = repnode->tr_right;    /* change repnode's parent */
            if (parent->tr_left != NULL) {
                parent->tr_left->tr_parent = parent;    /* change child's parent ptr */
            }
        }
        /* 
         * Now replace node with repnode ... step 1 adjust node's parent's
         * linkages. (node's parent becomes repnode's parent)
        */
        if (node->tr_parent == NULL) {  /* replaced the root node */
            *tree = repnode;        /* adjust root ptr accordingly */
        } else {
            if (node->tr_parent->tr_left == node) {
                node->tr_parent->tr_left = repnode;
            } else {
                node->tr_parent->tr_right = repnode;
            }
        }
        /*
         * Step 2: adjust repnode's linkages. Repnode inherits the left and right
         * subtrees of node (the node we are deleting). However, repnode may
         * be the right child of node, in which case we must leave its right pointer
         * as it is.
        */
        repnode->tr_parent = node->tr_parent;   /* write parent link */
        if (node->tr_right != repnode) {        /* repnode is right child of node */
            repnode->tr_right = node->tr_right; /* repnode gets node's right subtree */
        }
        repnode->tr_left = node->tr_left;
        /*
         * Step 3: Adjust parent linkages of repnode's new children
        */
        if (repnode->tr_right != NULL) {
            repnode->tr_right->tr_parent = repnode;
        }
        if (repnode->tr_left != NULL) {
            repnode->tr_left->tr_parent = repnode;
        }
    } else {
        /*
         * The node to removed is a leaf. Life is suddenly simple. Were it so always ...
        */
        parent = node->tr_parent;       /* get pointer to parent */
        if (parent == NULL) {           /* Wow! deleting only node in tree */
            *tree = NULL;           /* Null out root pointer */
        } else {                    /* Deleting leaf of a subtree */
            if (parent->tr_left == node) {      /* we're left child of parent */
                parent->tr_left = NULL; /* NULL left child link of parent */
            } else {                    /* must be the right child */
                parent->tr_right = NULL;    /* so NULL right child link of parent */
            }
        }
    }


    /* END .... let's blow this pop stand and go home, buddy */

    }
/**
 *
 * @brief Search through Binary Tree --- bt_search.

    The function bt_add adds nodes to the tree based on an insertion key.
    (The insertion key and its comparison operations are expressed in the
    comparison function passed to bt_add.) Thus, the tree is sorted on insertion
    key. The function bt_search provides the means by which the tree may be
    searched for a node with insertion key matching user defined criteria.
    (See bt_inorder for a method by which the tree may be scanned for
    nodes based on other data fields than the insertion key.)

    This function provides an engine by which the user may conduct searches
    for nodes matching specific criteria through the binary tree. The caller
    provides a user defined comparison function, as described above in the
    discussion of bt_add. The return codes from this comparison
    function are employed as follows:
<ul>
        <li>-1 => travel down left subtree of current node</li>
        <li>0 => search criteria match ... return pointer to matching node</li>
        <li>1 => travel down right subtree of current node</li>
</ul>

   @param tree is a pointer to a tree node.

   @param criteria is a pointer to a user defined record containing
            match criteria, which is interpreted by the user defined comparison
            function.

   @param compare_function is a
            pointer to a match function.
   @return matchnode is a pointer to a node matching
            the user defined criteria. A value of NULL indicates that no
            match was discovered.

 */
PTREE_NODE bt_search(PTREE_NODE tree,void* criteria,
	int (*compare_function)(PTREE_NODE tree,  void* criteria)){

    PTREE_NODE matchnode;           /* ptr to matching node */
	int result;					/* comparison function result */

    /* BEGIN */

    if (tree == NULL) {             /* at end of a branch */
        return (NULL);
    }

    /*
     * Check current node for match
     */

    result = (*compare_function)(tree,criteria);        /* call the user define comparison func */
    if (result < 0) {             /* visit left subtree */
        matchnode = bt_search(tree->tr_left,criteria,compare_function);
    } else if (result == 0) {               /* current node satisifies criteria */
        matchnode = tree;
    } else {                        /* visit right subtree */
        matchnode = bt_search(tree->tr_right,criteria,compare_function);
    }

    return (matchnode);

}

/**
 * @brief Find leftmost node of binary tree -- bt_leftmost.
    
    This function returns a pointer to the leftmost node of a binary subtree passed
    by the caller.

 * @param subtree is a pointer to a node that is presumeably
            a root of a subtree (i.e. has left or right children). This function
            doesn't mind if this node is a leaf, but if so, why explore its
            (non-existant) children?
 * @return The leftmost node of the subtree.
            Note that if no left subtree is dependent from the node passed
            by the caller, this value will be NULL.
 */
PTREE_NODE bt_leftmost(PTREE_NODE subtree) {

	PTREE_NODE leftchild; /* pointer to left child */
	PTREE_NODE nextchild; /* pointer to next child */

	/* BEGIN */

	if (subtree == NULL ) { /* got handed a NULL tree */
		return (NULL );
	}

	leftchild = subtree->tr_left; /* get ptr to left child */
	if (leftchild == NULL ) { /* oops ... no left branch */
		return (NULL );
	} else { /* explore left branch */
		nextchild = bt_leftmost(leftchild); /* get the next child */
		if (nextchild == NULL ) { /* our leftchild is leftmost */
			return (leftchild);
		} else { /* propogate nextchild back up */
			return (nextchild);
		}
	}

	/* END */

}

/**
 *
 * @brief Find rightmost node of binary tree -- bt_rightmost.
    
    This function returns a pointer to the rightmost node of a binary subtree passed
    by the caller.



 * @param subtree is a pointer to a node that is presumeably
            a root of a subtree (i.e. has left or right children). This function
            doesn't mind if this node is a leaf, but if so, why explore its
            (non-existant) children?
 * @return  The rightmost node of the subtree.
            Note that if no right subtree is dependent from the node passed
            by the caller, this value will be NULL.

 */
PTREE_NODE bt_rightmost(PTREE_NODE subtree) {

	PTREE_NODE rightchild; /* ptr to the right child */
	PTREE_NODE nextchild; /* ptr to the next child */

	/* BEGIN */

	rightchild = subtree->tr_right; /* get ptr to right child */
	if (rightchild == NULL ) { /* oops ... no right branch */
		return (NULL );
	} else { /* explore right branch */
		nextchild = bt_rightmost(rightchild); /* get the next child */
		if (nextchild == NULL ) { /* our rightchild is rightmost */
			return (rightchild);
		} else { /* propogate nextchild back up */
			return (nextchild);
		}
	}

	/* END */

}
/**
 * @brief Inorder Traversal of Binary Tree --- bt_inorder.

    This function performs an inorder traversal of a binary tree. At each node visited,
    the user supplied action function is called. The calling format of the action function
    is given by:

        flag = action(PTREE_NODE tree,void* argbuff);

        Where

			ushort flag [return value] is TRUE if the traversal is to continue
                and FALSE otherwise.

            PTREE_NODE tree [input] is a pointer to the node being visited.
    
            void* argbuff [input] is a user defined record interpreted by the
                action function.

    The function bt_inorder can thus be used to visit and process each node of
    a tree, or to search for a matching data field other than the insertion key.
    (Since a binary tree is sorted on its insertion key, one would use bt_search
    to search for an insertion key of a given value or within a range of values.)

    lastnode = bt_inorder(tree,argbuff,action);



 * @param tree is a pointer to a tree node/ the branch to be searched

 * @param argbuff is a pointer to the user defined argument record
            for the action function.

 * @param action is a pointer
            to the user defined action function.

 * @return A pointer to the last node visited during the inorder traversal.
 * 	Note that if this pointeris NULL on return, the entire tree was traversed
 * 	WITHOUT the action routine ever indicating a termination condition.
 */
PTREE_NODE bt_inorder(PTREE_NODE tree, void* argbuff,
		int (*action)(PTREE_NODE tree, void* argbuff)) {

	ushort flag; /* termination flag from action routine */
	PTREE_NODE lnode; /* last node visited */

	/* BEGIN */

	/*
	 * Visit the left branch of the tree.
	 */
	if (tree->tr_left != NULL ) {
		lnode = bt_inorder(tree->tr_left, argbuff, action);
	} else {
		lnode = NULL;
	}

	/*
	 * Visit root
	 */

	if (lnode == NULL ) { /* traversal continues ... visit root */
		flag = (*action)(tree, argbuff); /* action action routine */
		if (flag) {
			/*
			 * Continue traversal ... visit right branch of tree
			 */
			if (tree->tr_right != NULL ) {
				lnode = bt_inorder(tree->tr_right, argbuff, action);
			} else {
				lnode = NULL;
			}
		} else {
			lnode = tree; /* stop the traversal */
		}
	}

	return (lnode);

}
/**
 * @brief Replace binary tree node --- bt_replace.

    This function replaces a node of the binary tree with a new node.
    The disposition of the displaced node is the responsibility of the
    caller. The caller is also required to insure that the search key
    of the replaced node is consistent with that of the new one, so that
    the tree doesn't suddenly become unsorted.

 * @param tree Pointer to the root of the tree
 * @param oldnode Pointer to the node to be replaced
 * @param newnode Pointer to the replacement node
 *
 */
void bt_replace(PTREE_NODE *tree,          // ptr to root of the tree
		PTREE_NODE oldnode,                    // record to replace
		PTREE_NODE newnode) {                   // new record

	/* BEGIN */

	if (*tree == NULL ) {                    // empty tree ... can't replace
		return;
	}

	/*
	 * 19 March 92 Rob Garvey Mod
	 * Link the new node into the tree first, Rob, you flaming Irish asshole!
	 */

	if (*tree == oldnode) {                  // replaced node is root
		*tree = newnode;                    // update root ptr
	} else {                                // find old node's linkage in parent
		if (oldnode->tr_parent->tr_left == oldnode) { // update parent left child
			oldnode->tr_parent->tr_left = newnode;
		} else {
			oldnode->tr_parent->tr_right = newnode; // update parent right child
		}
	}
	/*
	 * NOW update new node with old node's header information.
	 */
	newnode->tr_parent = oldnode->tr_parent;
	newnode->tr_left = oldnode->tr_left;
	newnode->tr_right = oldnode->tr_right;
	newnode->tr_count = oldnode->tr_count;

	/* END */

}
