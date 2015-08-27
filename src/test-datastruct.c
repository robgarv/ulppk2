
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
#include <stdlib.h>
#include <string.h>

#include <btacc.h>
#include <dqacc.h>
#include <llacc.h>

#define NODE_STR_LENGTH 256

typedef struct {
	TREE_NODE node;
	char node_string[NODE_STR_LENGTH];
} BT_TEXT_NODE;

static BT_TEXT_NODE* new_bt_text_node(char* textp) {

	BT_TEXT_NODE* text_nodep;
	
	text_nodep = (BT_TEXT_NODE*)calloc(sizeof(BT_TEXT_NODE),1);
	strcpy(text_nodep->node_string, textp);
	return text_nodep;	
}

static int btnode_action_print(PTREE_NODE treep, void* argbuff) {

	BT_TEXT_NODE* text_nodep;
	
	text_nodep = (BT_TEXT_NODE*)treep;
	if (treep == NULL) {
		return FALSE;
	}
	printf("TREE_NODE: text = %s count = %d\n", text_nodep->node_string, text_nodep->node.tr_count);
	return 1;
	
}
static int btnode_cmp_text(PTREE_NODE nodep, void* testnodep) {
	BT_TEXT_NODE* tree_nodep;
	BT_TEXT_NODE* test_nodep;
	int cmp_result;
	
	tree_nodep = (BT_TEXT_NODE*)nodep;
	test_nodep = (BT_TEXT_NODE*)testnodep;
	cmp_result = strcmp(test_nodep->node_string, tree_nodep->node_string);
	
	return cmp_result;
}	
	
	

int test_binary_trees(int argc, char* argv[]) {
	int retval = 0;
	static char* the_strings[] = {
		"now", "is", "the", "time", "that", "tries", "men's", "souls",
		"twas", "brillig", "and", "all", "the", "mimsey", "toves"		
	};
	
	PTREE_NODE rootp = NULL;
	PTREE_NODE leftmostp;
	PTREE_NODE rightmostp;
	
	int nstrings;
	int i;
	
	nstrings = sizeof(the_strings)/sizeof(char*);
	
	for (i = 0; i < nstrings; i++) {
		BT_TEXT_NODE* new_nodep;
		new_nodep = new_bt_text_node(the_strings[i]);
		bt_add(&rootp, new_nodep, btnode_cmp_text);
	}
	
	bt_inorder(rootp, NULL, btnode_action_print);
	leftmostp = bt_leftmost(rootp);
	rightmostp = bt_rightmost(rootp);
	printf("LEFTMOST = %s %d \n", ((BT_TEXT_NODE*)leftmostp)->node_string, leftmostp->tr_count);
	printf("RIGHTMOST = %s %d \n", ((BT_TEXT_NODE*)rightmostp)->node_string, rightmostp->tr_count);
	return retval;
}

static int deque_as_stack_bottom() {
	int retval = 0;
	int i;
	int nitems;
	unsigned char c;
	unsigned char cpop;
	int deque_size;
	DQHEADER the_deque;

	deque_size = 32;	
	dq_init(deque_size, sizeof(char), &the_deque) ;
	
	printf("Push/Pop at queue bottom: deque size = %d\n", deque_size);
	
	for (c='a'; c <='z'; c++) {
		printf("dq_abd: %c\n",c);
		dq_abd(&the_deque, &c);
	}
	nitems = the_deque.dquse;
	printf("Popping nitems = %d\n", nitems);
	
	i = 0;
	while (!dq_rbd(&the_deque,  &cpop)) {
		i++;
		printf("dq_rbd: item i = %d | %c\n", i, cpop);
	}
	if (i != nitems) {
		printf("Warning: popped %d items deque header stated %d\n", i, nitems);
		retval += 1;
	}
	if (the_deque.dquse != 0) {
		printf("Deque header reports %d items still in use\n", the_deque.dquse);
		retval += 1;
	}
	
	if (retval != 0) {
		printf("Recorded %d errors ... aborting test deque_as_stack_bottom\n", retval);
		return retval;
	}
	
	printf("Overflow test\n");
	 
	for (c='a'; c < ('a' + deque_size + 10); c++) {
		printf("dq_abd: %c\n",c);
		if (dq_abd(&the_deque, &c)) {
			printf("Deque overflow at %d items\n", (int)(c - 'a'));
			break;
		}
	}
	nitems = the_deque.dquse;
	printf("Popping nitems = %d\n", nitems);
	
	i = 0;
	while (!dq_rbd(&the_deque,  &cpop)) {
		i++;
		printf("dq_rbd: item i = %d | %c\n", i, cpop);
	}
	if (i != nitems) {
		printf("Warning: popped %d items deque header stated %d\n", i, nitems);
		retval += 1;
	}
	if (the_deque.dquse != 0) {
		printf("Deque header reports %d items still in use\n", the_deque.dquse);
		retval += 1;
	}
	
	if (retval != 0) {
		printf("Recorded %d errors ... aborting test deque_as_stack\n", retval);
		return retval;
	}
	
	dq_close(&the_deque);
	
	printf("Deque closed/released\n");
		
	return retval;
}

static int deque_as_stack_top() {
	int retval = 0;
	int i;
	int nitems;
	unsigned char c;
	unsigned char cpop;
	int deque_size;
	DQHEADER the_deque;

	deque_size = 32;	
	dq_init(deque_size, sizeof(char), &the_deque) ;
	
	printf("Push/Pop at dequeue top: deque size = %d\n", deque_size);
	
	for (c='a'; c <='z'; c++) {
		printf("dq_atd: %c\n",c);
		dq_atd(&the_deque, &c);
	}
	nitems = the_deque.dquse;
	printf("Popping nitems = %d\n", nitems);
	
	i = 0;
	while (!dq_rtd(&the_deque,  &cpop)) {
		i++;
		printf("dq_rtd: item i = %d | %c\n", i, cpop);
	}
	if (i != nitems) {
		printf("Warning: popped %d items deque header stated %d\n", i, nitems);
		retval += 1;
	}
	if (the_deque.dquse != 0) {
		printf("Deque header reports %d items still in use\n", the_deque.dquse);
		retval += 1;
	}
	
	if (retval != 0) {
		printf("Recorded %d errors ... aborting test deque_as_stack\n", retval);
		return retval;
	}
	
	printf("Overflow test\n");
	 
	for (c='a'; c < ('a' + deque_size + 10); c++) {
		printf("dq_atd: %c\n",c);
		if (dq_atd(&the_deque, &c)) {
			printf("Deque overflow at %d items\n", (int)(c - 'a'));
			break;
		}
	}
	nitems = the_deque.dquse;
	printf("Popping nitems = %d\n", nitems);
	
	i = 0;
	while (!dq_rtd(&the_deque,  &cpop)) {
		i++;
		printf("dq_rtd: item i = %d | %c\n", i, cpop);
	}
	if (i != nitems) {
		printf("Warning: popped %d items deque header stated %d\n", i, nitems);
		retval += 1;
	}
	if (the_deque.dquse != 0) {
		printf("Deque header reports %d items still in use\n", the_deque.dquse);
		retval += 1;
	}
	
	if (retval != 0) {
		printf("Recorded %d errors ... aborting test deque_as_stack_top\n", retval);
		return retval;
	}
	
	dq_close(&the_deque);
	
	printf("Deque closed/released\n");
		
	return retval;
}

static int deque_as_stack() {
	int retval = 0;
	
	retval += deque_as_stack_bottom();
	retval += deque_as_stack_top();
	return retval;	
}

static int deque_as_fifo() {
	int retval = 0;
	int i;
	int nitems;
	unsigned char c;
	unsigned char cpop;
	int deque_size;
	DQHEADER the_deque;

	deque_size = 32;	
	dq_init(deque_size, sizeof(char), &the_deque) ;
	
	printf("FIFO test: Push on deque back / pop at dequeue top: deque size = %d\n", deque_size);
	
	for (c='a'; c <='z'; c++) {
		printf("dq_abd: %c\n",c);
		dq_abd(&the_deque, &c);
	}
	nitems = the_deque.dquse;
	printf("Popping nitems = %d\n", nitems);
	
	i = 0;
	while (!dq_rtd(&the_deque,  &cpop)) {
		i++;
		printf("dq_rtd: item i = %d | %c\n", i, cpop);
	}
	if (i != nitems) {
		printf("Warning: popped %d items deque header stated %d\n", i, nitems);
		retval += 1;
	}
	if (the_deque.dquse != 0) {
		printf("Deque header reports %d items still in use\n", the_deque.dquse);
		retval += 1;
	}
	
	if (retval != 0) {
		printf("Recorded %d errors ... aborting test deque_as_fifo\n", retval);
		return retval;
	}
	
	return retval;
}

int test_deques(int argc, char* argv[]) {
	int retval = 0;
	
	retval += deque_as_stack();
	
	retval += deque_as_fifo();
	
	return retval;
}

static void llprint_node(PLL_NODE nodeptr) {
	
	if (nodeptr != NULL) {
		printf("%s\n", (char*)nodeptr->data);
	} else {
		printf("NODE-IS-NULL\n");
	}
}

static int dump_list(PLL_HEAD the_listptr) {
	PLL_NODE cptr;
	PLL_NODE bptr;
	int dump_count = 0;
	int retval = 0;
	int list_count;
	
	
	list_count = ll_getptrs(the_listptr, &cptr, &bptr);
	printf("Dumping linked list -- %d items\n", list_count);
	while (cptr != NULL) {
		dump_count++;
		llprint_node(cptr);
		cptr = ll_iterate(cptr);
	}
	if (dump_count != list_count) {
		printf("test_ll_basic_ops: Error: list_count = %d actual dump count = %d\n", list_count, dump_count);
		retval += 1;
	}
	return retval;
}
	
static int test_ll_basic_ops() {
	int retval = 0;
	LL_HEAD the_list;
	PLL_NODE tnptr;
	PLL_NODE cptr;
	PLL_NODE bptr;
	int front_count = 5;
	int back_count = 8;
	char the_string[32];
	int i;
	int list_count;
	
	ll_init(&the_list);
	
	printf("test_ll_basic_ops\n");
	for (i = 0; i < front_count; i++) {
		sprintf(the_string,"FString%.3d", i);
		printf("ll_addfront %s\n", the_string);
		tnptr = ll_new_node(the_string, strlen(the_string) + 1);
		
		ll_addfront(&the_list, (PLL_NODE)tnptr);
	}
	printf("Dumping list after %d add_fronts\n", front_count);
	retval += dump_list(&the_list);
	for (i= front_count; i < (front_count + back_count); i++) {
		sprintf(the_string,"BString%.3d", i);
		printf("ll_addback %s\n", the_string);
		tnptr = ll_new_node(the_string, strlen(the_string) + 1);
		
		ll_addback(&the_list, (PLL_NODE)tnptr);
		
	}
	printf("Dumping list after %d add_backs\n", back_count);
	retval += dump_list(&the_list);
	
	printf("Get 3 from front\n");
	for (i = 0; i < 3; i++) {
		tnptr = ll_getfront(&the_list);
		if (tnptr == NULL) {
			printf("Unexpected NULL returned from ll_getfront\n");
			break;
		}
		llprint_node(tnptr);
	}
	dump_list(&the_list);
	
	printf("Get 3 from back\n");
	for (i = 0; i < 3; i++) {
		tnptr = ll_getback(&the_list);
		if (tnptr == NULL) {
			printf("Unexpected NULL returned from ll_getback\n");
			break;
		}
		llprint_node(tnptr);
	}
	dump_list(&the_list);

	// Add three more to front and back
	printf("Adding 3 to front and back of list -- IDs >= 100\n");
	for (i = 0; i < 3; i++) {
		sprintf(the_string,"FString%.3d", 100 + i);
		printf("ll_addfront %s\n", the_string);
		tnptr = ll_new_node(the_string, strlen(the_string) + 1);
		ll_addfront(&the_list, tnptr);
		sprintf(the_string,"BString%.3d", 100 + i);
		printf("ll_addback %s\n", the_string);
		tnptr = ll_new_node(the_string, strlen(the_string) + 1);
		ll_addback(&the_list, tnptr);
	}
	dump_list(&the_list);
	printf("Get 3 from front\n");
	for (i = 0; i < 3; i++) {
		tnptr = ll_getfront(&the_list);
		if (tnptr == NULL) {
			printf("Unexpected NULL returned from ll_getfront\n");
			break;
		}
		llprint_node(tnptr);
	}
	dump_list(&the_list);
	
	printf("Get 3 from back\n");
	for (i = 0; i < 3; i++) {
		tnptr = ll_getback(&the_list);
		if (tnptr == NULL) {
			printf("Unexpected NULL returned from ll_getback\n");
			break;
		}
		llprint_node(tnptr);
	}
	
	dump_list(&the_list);
	printf("Draining linked list\n");
	ll_drain(&the_list, NULL);
	
	list_count = ll_getptrs(&the_list, &cptr, &bptr);
	if (list_count != 0) {
		printf("test_ll_basic_ops: list_count nonzero after drain: %d\n", list_count);
		retval += 1;
	}
	
	return retval;
}
int test_linked_lists(int argc, char* argv[]) {
	int retval = 0;
	
	retval += test_ll_basic_ops();
	
	return retval;
}

int main(int argc, char* argv[]) {
	
	int retval = 0;
	
	retval += test_binary_trees(argc, argv);
	
	retval += test_deques(argc, argv);
	
	retval += test_linked_lists(argc, argv);
	
	return retval;
	
}
