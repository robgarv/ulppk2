
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

/**
 * @file appenv.c
 *
 * @brief Application environment access methods.
 *
 * This package provides a robust facility for accessing and managing
 * environment variables.
 *
 * Environment variables that are of significance to the application
 * should be identified by calling appenv_register_env_var. At registration
 * sensible defaults can be established, if the environment does not
 * establish an overriding value. Applications code can then call
 * appenv_read_env_var and be assured that a reasonable value is returned.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appenv.h"
#include "diagnostics.h"

/**
 * Top level of the environment data structure.
 * The environment data structure is implemented as
 * a binary tree of environment variable nodes.
 */
typedef struct {
	ENV_NODE* rootp;	///< pointer to the root of the environment variable tree
	int nodecnt;		///< Count of nodes (and hence registere environment variables)
} ENV_TREE;

static ENV_TREE root = { NULL, 0 };
static char errbuff[1024];

/**
 * A btacc action function for in order traversal. This one
 * just prints text and returns a count.
 * @param treep Pointer to a binary tree node
 * @param vfout Output FILE* as a void pointer
 * @return Always returns 1 to force continuation of traversal.
 */
 
static int print_node_action(PTREE_NODE treep, void* vfout) {
	ENV_NODE* nodep;
	
	nodep = (ENV_NODE*)treep;
	
	fprintf((FILE*)vfout, "[%s] = [%s]\n", nodep->varname, nodep->value);
	
	return 1;	
} 
/**
 * A btacc compare function. Returns 
 * 		< 0 for left branch (less than)
 * 		== 0 for equality
 * 		> 0 for right branch (greater than)
 */
static int varname_cmpfunc(PTREE_NODE nodep, void* match) {
	int result = 0;
	ENV_NODE* envnodep;
	ENV_NODE* matchnodep;
	
	envnodep = (ENV_NODE*)nodep;
	matchnodep = (ENV_NODE*)match;
	result = strcmp(envnodep->varname, matchnodep->varname);
	DBG_TRACE(stderr, "envnode %s compare match %s result = %d", envnodep->varname, matchnodep->varname, result);
	return result;
}

static ENV_NODE* search_tree(char* varname) {
	ENV_NODE* matchnodep;
	matchnodep = (ENV_NODE*)calloc(1, sizeof(ENV_NODE));
	strncpy(matchnodep->varname, varname, sizeof(matchnodep->varname)-1);
	return (ENV_NODE*)bt_search((PTREE_NODE)root.rootp, (void*)matchnodep, varname_cmpfunc);
}

/**
 * Register an environment variable. If the environment does not define the
 * variable, create an ENV_NODE and assign the default value. (A NULL value is allowed,
 * but will be interpreted as "set it to empty string".) If the environment does
 * define the variable, create a new ENV_NODE, do a getenv and store the value in the
 * node. Return the established (default or retrieved) value to the caller.
 *
 * Note that the calling application must NOT alter the data referenced by
 * the returned pointer, nor free it.
 *
 * @param varname Name of the environment variable to register
 * @param defaultval Default value to assigned if not provided by the shell environment
 * @return String value of the environment variable
 */
char* appenv_register_env_var(const char* varname, const char* defaultval) {
	ENV_NODE* varnodep = NULL;
	char* env_valp = NULL;
	int add_node = 0;
	
	// First, determine if the variable has already been stored in a 
	// ENV_NODE in the tree.
	if (NULL != root.rootp) {
		// Tree is not empty ... do search
		varnodep = search_tree((char*)varname);
		
	}
	if (NULL == varnodep) {
		add_node = 1;
		varnodep = (ENV_NODE*)calloc(1, sizeof(ENV_NODE));
	}
	
	// Now check the environment to see if the variable has been set 
	// by the invoking shell
	env_valp = getenv(varname);
	
	// Write the environment node. If env_val is NULL ... enforce the default
	// if provided. If no default was provided, enforce the compile time
	// default value
	strncpy(varnodep->varname, varname, sizeof(varnodep->varname) - 1);
	if (NULL == env_valp) { 
		if (defaultval != NULL) {
			strncpy(varnodep->value, defaultval, sizeof(varnodep->value) - 1);
		} else {
			strcpy(varnodep->value, "");
		}
	} else {
		strncpy(varnodep->value, env_valp, sizeof(varnodep->value) - 1) ;
	}
	
	if (add_node) {
		// Add the variable to the tree
		bt_add(((PTREE_NODE*)&root.rootp), varnodep, varname_cmpfunc);
	}
	return varnodep->value;
}

/**
 * Set the value of an environment variable. Unlike appenv_register_env_var,
 * this function will cause an value set by the shell to be overriden.
 * @param varname Name of the environment variable to register
 * @param val Value to assign to environment variable
 * @return String value assigned to environment variable
 */
char* appenv_set_env_var(const char* varname, const char* val) {
	ENV_NODE* varnodep = NULL;
	int add_node =0;
	
	// First, determine if the variable has already been stored in a 
	// ENV_NODE in the tree.
	if (NULL != root.rootp) {
		// Tree is not empty ... do search
		varnodep = search_tree((char*)varname);
		
	}
	if (NULL == varnodep) {
		add_node = 1;
		varnodep = (ENV_NODE*)calloc(1, sizeof(ENV_NODE));
	}
	
	
	// Write the environment node. If env_val is NULL ... enforce the default
	// if provided. If no default was provided, enforce the compile time
	// default value
	strncpy(varnodep->varname, varname, sizeof(varnodep->varname) - 1);
	if (val != NULL) {
		strncpy(varnodep->value, val, sizeof(varnodep->value) - 1);
	} else {
		strcpy(varnodep->value, "");
	}
	// Now write it back to the environment so child processes
	// can inherit this setting.
	setenv(varnodep->varname, varnodep->value, 1);
	if (add_node) {
		// Add the variable to the tree
		bt_add(((PTREE_NODE*)&root.rootp), varnodep, varname_cmpfunc);
	}
	
	return varnodep->value;
}

/**
 * Obtain the value of a registered environment variable.
 * @param varname Name of the environment variable to register
 * @return String value of environment variable
 */
char* appenv_read_env_var(const char* varname) {
	ENV_NODE* nodep;
	char* strval;
	nodep = search_tree((char*)varname);
	// (NULL == nodep) ? strval = NULL : strval = nodep->value;
	if (NULL == nodep) {
		strval = NULL;
	} else {
		strval = nodep->value;
	}
	return strval;
}

/**
 * Report on all registered environment variables. Print to the
 * file stream provided.
 * @param fout File stream for writing the report out.
 */
int appenv_report(FILE* fout) {
	fprintf(fout, "============ ENVIRONENT VARIABLE DUMP ===============\n");
	if (bt_inorder((PTREE_NODE)root.rootp, fout, print_node_action) != NULL) {
		ERR_MSG(errbuff, "bt_inorder prematurely terminated traversal!");
	}
	fprintf(fout, "======== END  ENVIRONENT VARIABLE DUMP ===============\n");
	
	return 0;
}

