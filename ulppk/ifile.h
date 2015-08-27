
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

#ifndef _IFILE_H
#define _IFILE_H

#include <stdio.h>
#include <btacc.h>

/**
 * @file ifile.h
 *
 * @brief Data structure type definitions for the ini file data structure.
 * See ifile.c for details.
 */
/**
 * Datastructure node types.
 */
typedef enum {
	IFNT_FILE = 1,		///< Node represents an ini file and is root of a subtree
	IFNT_SECTION,		///< Node represents a section
	IFNT_ELEMENT		///< Node represents a name/value pair.
} IF_NODE_TYPE;

/**
 * Identifies the node data type. Numbers
 * and strings are currently supported.
 */
typedef enum {
	IFDT_STRING = 1,
	IFDT_NUMBER
} IF_DATA_TYPE;

/**
 * Node describing an ini file
 */
typedef struct {
	char* inifile_path;			///< Path to the inifile
	TREE_NODE* sections;		///< a tree of sections defined under this inifile
} IF_INIFILE;

/**
 * Node describing a section within an inifile.
 */
typedef struct {
	int element_count;			///< Count of elements under this section
	TREE_NODE* elements;		///< a tree of elements defined under this section
} IF_SECTION_NODE;


/**
 * Node describing a name/value pair under a section
 */
typedef struct {
	IF_DATA_TYPE data_type;		///< One of the supported data type codes.
	union {
		char* szvalue;			///< String value
		long lvalue;			///< Long integer (number) value
	};
} IF_ELEMENT_NODE;

/**
 * Format of the nodes in the ini file data structure.
 */
typedef struct {
	TREE_NODE node;				///< Tree node element. See btacc.h
	IF_NODE_TYPE node_type;		///< Node type
	char* name;					///< Name of the node
	union {
		IF_INIFILE inifile;				///< For IFNT_FILE types
		IF_SECTION_NODE section;		///< For IFNT_SECTION types
		IF_ELEMENT_NODE element;		///< For IFNT_ELEMENT types
	};
} INIFILE_NODE;

/**
 * Structure that contains specificatios for a search
 * of an ini file subtree.
 */
typedef struct {
	char* name;					///< Name of the target node
	IF_NODE_TYPE node_type;		///< Type of the target node
} IF_FILTER_SPEC;

INIFILE_NODE* if_new_inifile(const char* filepath);
INIFILE_NODE* if_new_section(const char* section_name);
INIFILE_NODE* if_new_element(const char* element_name);
INIFILE_NODE* if_new_string(const char* element_name, const char* strval);
INIFILE_NODE* if_new_number(const char* element_name, long lval);
int if_add_inifile(INIFILE_NODE** rootpp, INIFILE_NODE* inifilep);
int if_add_section(INIFILE_NODE* inifilep, const char* section_name);
int if_add_section_node(INIFILE_NODE* inifilep, INIFILE_NODE* section_nodep);
int if_add_element(INIFILE_NODE* inifilep, const char* section_name, INIFILE_NODE* newnodep);
INIFILE_NODE* if_get_node(INIFILE_NODE* rootp, const char* name, IF_NODE_TYPE type);
INIFILE_NODE* if_get_element(INIFILE_NODE* inifilep, const char* section_name, const char* element_name);

int if_walk_file(INIFILE_NODE* inifilep);

// These functions are actually defined in the inifileparser.y yacc source module

// Instructs the parser to parse the ini file at fpath
int if_parse_inifile(FILE* fp, char* fpath);

// Gets the root of the inifile tree 
INIFILE_NODE* if_get_root();

// Set error reporting file stream
void if_set_err_file(FILE* fp);

// Read out a long integer value from a value element
// Return 0 if succes, non-zero if nodep is not a value element.
int if_get_intval(INIFILE_NODE* nodep, long* lvalp);

// Read out a string value from a value element. strp is a point
// to a char*. The pointer value written is owned by the inifile node.
// Don't free it! If you need to edit it, strdup the string and
// edit the duplicate.
// Return 0 if succes, non-zero if nodep is not a value element.
int if_get_strval(INIFILE_NODE* nodep, char** strp);
#endif
