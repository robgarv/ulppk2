
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
 * @file ifile.c
 *
 * @brief Settings (.ini) file access.
 *
 * .ini files are parsed by a lex/yacc parser (see inifileparser.c) and the results
 *  are stored in a binary tree structure. The binary tree structure will contain
 *  one or more nodes representing one or more parsed ini files. Each file node
 *  roots a subtree representing the contents of that ini file.
 *
 * The elements of a .ini file are
 *
 * <ol>
 * <li>Sections which provide groupings of name/value pairs</li>
 * <li>Name/value pairs</li>
 * </ol>
 *
 * For example:
 *
 * [mysection]
 * var1=val1
 * var2=val2
 * [yoursection]
 * var3=val3
 * var1=val4
 *
 * Not that in the above example the variable name var1 appears in two distinct sections.
 * This is no problem since the section provides a name space ... mysection.var1 is
 * not yoursection.var1
 *
 * <h2>Notes on inifileparser.c</h2>
 * This module is built using lex and yacc, and is automatically generated. Consequently,
 * the parser is not documented. inifile.l contains the lex token specifications, and
 * inifileparser.y contains the grammar specification.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ifile.h"

static int name_comp_func(TREE_NODE* nodep, void* vpname) {
	INIFILE_NODE* ifnodep;
	char* szname;

	szname = (char*)vpname;
	ifnodep = (INIFILE_NODE*)nodep;
	return strcmp(szname, ifnodep->name);
}

static int node_comp_func(TREE_NODE* node1p, void* node2p) {
	INIFILE_NODE* cnodep;

	cnodep = (INIFILE_NODE*)node2p;
	return name_comp_func(node1p, cnodep->name);
}

static int print_section(TREE_NODE* tnodep, void* argbuff);
static int print_element(TREE_NODE* tnodep, void* argbuff);
static int print_inifile(TREE_NODE* tnodep, void* argbuff);

/**
 * Prepare a binary tree data structure for receiving the
 * parsed contents of a .ini file.
 * @param filepath Full path to the .ini file
 * @return Pointer to an INIFILE_NODE representing the root of the tree structure
 */
INIFILE_NODE* if_new_inifile(const char* filepath) {

	int namelen;

	namelen = strlen(filepath);
	INIFILE_NODE* nodep =  (INIFILE_NODE*)calloc(1, sizeof(INIFILE_NODE));
	nodep->node_type = IFNT_FILE;
	nodep->inifile.inifile_path = strdup(filepath);
	nodep->name = strdup(filepath);
	return nodep;
}

/**
 * Prepare an INIFILE_NODE for use as a section node.
 * @param section_name Name of the section.
 * @return Pointer to INIFILE_NODE configured as a section.
 */
INIFILE_NODE* if_new_section(const char* section_name) {


	int namelen;

	namelen = strlen(section_name);
	INIFILE_NODE* nodep =  (INIFILE_NODE*)calloc(1, sizeof(INIFILE_NODE));
	nodep->node_type = IFNT_SECTION;
	nodep->name = strdup(section_name);
	nodep->section.element_count = 0;
	return nodep;

}

/**
 * An element represents a name/value pair under a section.
 * @param element_name Name of the element
 * @return INIFILE-NODE pointer configured as an element.
 */
INIFILE_NODE* if_new_element(const char* element_name) {
	int namelen;

	namelen = strlen(element_name);
	INIFILE_NODE* nodep =  (INIFILE_NODE*)calloc(1, sizeof(INIFILE_NODE));
	nodep->node_type = IFNT_ELEMENT;
	nodep->name = strdup(element_name);
	return nodep;

}

/**
 * An element represents a name/value pair under a section.
 * Prepares an element node that stores a string value.
 * @param element_name Name of the element
 * @param strval String value to store.
 * @return INIFILE-NODE pointer configured as an element storing
 *  a string value..
 */
INIFILE_NODE* if_new_string(const char* element_name, const char* strval) {
	int namelen;

	INIFILE_NODE* nodep =  if_new_element(element_name);
	nodep->element.data_type = IFDT_STRING;
	nodep->element.szvalue = strdup(strval);
	return nodep;

}
/**
 * An element represents a name/value pair under a section.
 * Prepares an element node that stores a numeric value.
 * @param element_name Name of the element
 * @param lval Long value to store
 * @return INIFILE-NODE pointer configured as an element storing
 *  a numeric value..
 */

INIFILE_NODE* if_new_number(const char* element_name, long lval) {
	int namelen;

	INIFILE_NODE* nodep = if_new_element(element_name);
	nodep->element.data_type = IFDT_NUMBER;
	nodep->element.lvalue = lval;
	return nodep;

}

/**
 * Add an ini file to the settings structure. This allows the structure
 * to store additional secctions and name/value pairs from multiple
 * .ini files.
 * @param inifilerootpp Pointer to a pointer to root of the tree structure.
 * @param inifilep Pointer to a node configured to represent an .ini file.
 * @return 0 on success, 1 on failure.
 */
int if_add_inifile(INIFILE_NODE** inifilerootpp, INIFILE_NODE* inifilep) {

	if (inifilep->node_type != IFNT_FILE) {
		fprintf(stderr, "Attempt to add object other than ini file to ini file tree |$s|\n", inifilep->name);
		return 1;
	}
	bt_add((TREE_NODE**)inifilerootpp, inifilep, node_comp_func);
	return 0;
}

/**
 * Adds a section node to the tree structure.Creates a new
 * section using the section_name provided.
 *
 * @param inifilep Pointer to a node configured to represent an .ini file.
 * @param section_name Name of the new section
 * @return 0 on success, 1 on failure.
 */
int if_add_section(INIFILE_NODE* inifilep, const char* section_name) {
	TREE_NODE** sectionspp;

	sectionspp = &(inifilep->inifile.sections);
	bt_add(sectionspp, if_new_section(section_name), node_comp_func);
	return 0;
}

/**
 * Adds the given section node to the tree structure.
 * @param inifilep Pointer to a node configured to represent an .ini file.
 * @param section_nodep Pointer to a section node to add
 * @return 0 on success, 1 on failure.
 */
int if_add_section_node(INIFILE_NODE* inifilep, INIFILE_NODE* section_nodep) {
	TREE_NODE** sectionspp;

	if (section_nodep->node_type != IFNT_SECTION) {
		fprintf(stderr, "Attempt to add section node fails: object is not a section |%s|\n",
				section_nodep->name);
		return 1;
	}
	sectionspp = &(inifilep->inifile.sections);
	bt_add(sectionspp, section_nodep, node_comp_func);
	return 0;
}

/**
 * Adds an element node to the tree structure.
 * @param inifilep Pointer to a node configured to represent an .ini file.
 * @param section_name Name of the new section.
 * @param newnodep Pointer to the new node to add.
 * @return 0 on success, 1 on failure.
 */

int if_add_element(INIFILE_NODE* inifilep, const char* section_name, INIFILE_NODE* newnodep) {

	INIFILE_NODE* sectionp;
	int retstatus = 1;

	// Find the section
	
	sectionp = (INIFILE_NODE*)bt_search(inifilep->inifile.sections, (char*)section_name, name_comp_func);
	if (sectionp != NULL) {
		if (sectionp->node_type == IFNT_SECTION) {
			bt_add(&(sectionp->section.elements), newnodep, node_comp_func);
			retstatus = 0;
		}
	}
	return retstatus;


}

/**
 * Gets the named INIFILE_NODE of given IN_NODE_TYPE.
 *
 * @param rootp Pointer to the root note of the inifile settings datastructure.
 * @param name String containing the name of the target node
 * @param type Type of the node required:
 * <ol>
 * <li>IFNT_FILE -- File node</li<>
 * <li>IFNT_SECTION -- Section node</li<>
 * <li>IFNT_ELEMENT -- Element (name/value pair) node</li<>
 * </ol>
 * @return NULL on failure to find the specified node, otherwise a pointer to the
 * 	node structure.
 */
INIFILE_NODE* if_get_node(INIFILE_NODE* rootp, const char* name, IF_NODE_TYPE type) {
	INIFILE_NODE* nodep =  NULL;

	nodep = (INIFILE_NODE*)bt_search((TREE_NODE*)rootp, (char*)name, name_comp_func);
	if (NULL == nodep) {
		return nodep;
	}

	if (nodep->node_type != type)  {
		// Name matches but not the node type
		nodep = NULL;
	}
	return nodep;
}

/**
 * Search the subtree under an ini file node for the named element in the named section.
 * @param inifilep Pointer to INIFILE_NODE representing the ini file. This is the root of
 *  the subtree for that inifile.
 * @param section_name Character string containing the target section name
 * @param element_name Character string containing the name of the target element.
 * @return NULL on failure to find the specified element node. On success, a pointer
 * 	to the specified target node.
 */
INIFILE_NODE* if_get_element(INIFILE_NODE* inifilep, const char* section_name, const char* element_name) {
	INIFILE_NODE* sectionp = NULL;
	INIFILE_NODE* elementp = NULL;

	sectionp = if_get_node((INIFILE_NODE*)(inifilep->inifile.sections), section_name, IFNT_SECTION);
	if ((sectionp != NULL) && (sectionp->node_type == IFNT_SECTION)) {
		elementp = if_get_node((INIFILE_NODE*)sectionp->section.elements, element_name, IFNT_ELEMENT);
	}
	return elementp;
}

/**
 * Performs an in order traversal of a subtree representing the contents of an inifile.
 * @param inifilep Pointer to INIFILE_NODE representing the ini file. This is the root of
 * @return 0 on success, non-zero otherwise.
 */
int if_walk_file(INIFILE_NODE* inifilep) {
	// Perform in-order traversal of inifile ... represented as a binary tree of sections
	// ordered on section name. A each section, visit its tree of elements.
	bt_inorder((TREE_NODE*)inifilep, NULL, print_inifile);
	return 0;
}


static int print_inifile(TREE_NODE* tnodep, void* argbuff) {
	INIFILE_NODE* inifilep;
	FILE* outfilep;

	inifilep = (INIFILE_NODE*)tnodep;
	if (argbuff == NULL) {
		outfilep = stdout;
	} else {
		outfilep = (FILE*)argbuff;
	}
	
	fprintf(outfilep, "INIFILE: %s\n", inifilep->name);
	bt_inorder(inifilep->inifile.sections, argbuff, print_section);
	return 0;

}
static int print_section(TREE_NODE* tnodep, void* argbuff) {
	INIFILE_NODE* sectionp;
	FILE* outfilep;

	sectionp = (INIFILE_NODE*)tnodep;
	if (argbuff == NULL) {
		outfilep = stdout;
	} else {
		outfilep = (FILE*)argbuff;
	}
	if (sectionp->node_type != IFNT_SECTION) {
		fprintf(stderr, "Fatal Error: Exepcted a SECTION Node!\n");
		fprintf(stderr, "INIFILE binary tree structures are corrupt\n");
		return 0;		// indicate traversal abort
	}

	// Perform in-order traversal of the tree of elements
	fprintf(outfilep, "SECTION|%s|\n", sectionp->name);
	
	if (sectionp->section.elements != NULL) {
		bt_inorder(sectionp->section.elements, argbuff, print_element);
	}
	return 1;			// indicate traversal continue
}

static int print_element(TREE_NODE* tnodep, void* argbuff) {
	INIFILE_NODE* elementp;
	FILE* outfilep;

	elementp = (INIFILE_NODE*)tnodep;
	if (argbuff == NULL) {
		outfilep = stdout;
	} else {
		outfilep = (FILE*)argbuff;
	}

	if (elementp->node_type != IFNT_ELEMENT) {
		fprintf(stderr, "Fatal Error: Exepcted a ELEMENT Node!\n");
		fprintf(stderr, "INIFILE binary tree structures are corrupt\n");
		return 0;		// indicate traversal abort
	}

	fprintf(outfilep, "ELEMENT|%s|VALUE|", elementp->name);
	switch (elementp->element.data_type) {
	case IFDT_STRING:
		fprintf(outfilep, "%s|\n", elementp->element.szvalue);
		break;
	case IFDT_NUMBER:
		fprintf(outfilep,"%ld|%lx|\n", elementp->element.lvalue, elementp->element.lvalue);
		break;
	default:
		fprintf(stderr, "Fatal Error: Undefined ELEMENT Node Data Type!\n");
		fprintf(stderr, "INIFILE binary tree structures are corrupt\n");
		return 0;		// indicate traversal abort
	}
	return 1;
}
/**
 * Read out a long integer value from a value element
 * Return 0 if succes, non-zero if nodep is not a value element
 * or nodep was NULL.
 * @param nodep Pointer to an element (name/value pair) node.
 * @param lvalp Pointer to a long to receive the value
 * @return 0 on success, 1 on failure. Fails if nodep is NULL
 *  or if nodep is not of type IFNT_ELEMENT
 */
int if_get_intval(INIFILE_NODE* nodep, long* lvalp) {
	int retval = 0;

	if (NULL == nodep) {
		// Certainly, we can't succeed.
		retval = 1;
	} else {
		if (nodep->node_type == IFNT_ELEMENT) {
			*lvalp = nodep->element.lvalue;
		} else {
			retval = 1;
		}
	}
	return retval;
}
/**
 * Read out a string value from a value element. strp is a point
 * to a char*. The pointer value written is owned by the inifile node.
 * Don't free it! If you need to edit it, strdup the string and
 * edit the duplicate.
 *
 * @param nodep Pointer to an element (name/value pair) node.
 * @param strp Receives pointer to array of char containing the string value.
 * @return 0 if success, non-zero if nodep is not a value element or
 * nodep was NULL.
 *
 */
int if_get_strval(INIFILE_NODE* nodep, char** strp) {
	int retval = 0;

	if (NULL == nodep) {
		// Certainly we can't succeed
		retval = 1;
	} else {
		if (nodep->node_type == IFNT_ELEMENT) {
			*strp = nodep->element.szvalue;
		} else {
			retval = 1;
		}
	}
	return retval;
}

