
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
#ifndef APPENV_H_
#define APPENV_H_

/**
 * @file appenv.h
 * @brief Definitions for the appenv.c application environment variables
 * module.
 */
/*
 * The appenv module provides simple facilitites for accessing the application
 * environemnt.
 */
#include <stdio.h>
#include <btacc.h>
 
#define MAX_VAR_NAME_SIZE 256
#define MAX_VALUE_SIZE 1024

/**
 * @brief Environment variable tree structure node.
 */
typedef struct {
	TREE_NODE tnode;			///< a binary tree node structure (btacc.h)
	char varname[MAX_VAR_NAME_SIZE];	///< environement variable name
	char value[MAX_VALUE_SIZE];			///< environment variable value
	int defaultval;				///< true if a default value was enforced
} ENV_NODE;
 

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register an environment variable. If the environment does not define the
 * variable, create an ENV_NODE and assign the default value. (A NULL value is allowed,
 * but will be interpreted as "set it to empty string".) If the environment does
 * define the variable, create a new ENV_NODE, do a getenv and store the value in the
 * node. Return the established (default or retrieved) value to the caller.
 */
char* appenv_register_env_var(const char* varname, const char* defaultval);

/*
 * Set the value of an environment variable. Another form of register, really.
 */
char* appenv_set_env_var(const char* varname, const char* val);
/*
 * Obtain the value of a registered environment variable.
 */
char* appenv_read_env_var(const char* varname);

/*
 * Report on all registered environment variables.
 */
int appenv_report(FILE* fout);

#ifdef __cplusplus
}
#endif
 
#endif /*APPENV_H_*/
