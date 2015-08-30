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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "cmdargs.h"

/**
 * @file cmdargs.c
 *
 * @brief Command argument parsing and access functions.
 *
 * cmadargs provides methods for defining command line option syntax,
 * parsing command line options, and retrieving their values.
 * The mechanism also provides for error reporting and command line
 * help features in a convenient fashion.
 *
 * The file cmdargs.h contains some useful top level commentary.
 */


/**
 * Root option. The root option will receive the program name
 */
CMD_ARG root;

static int register_error = 0;
static int parsing_complete = 0;

// forward declarations

static CMD_ARG* find_option(CMD_ARG* argp, char* name);
static int valid_option(char* opt_name);
static int add_option(CMD_ARG* argp, char* opt_name, char* opt_vname, CA_MODE mode, 
	char* description, char* default_value);
static int show_help(CMD_ARG* argp);
static int verify_required_args(CMD_ARG* argp);
static char* clean_option_text(char* buff, char* opt_name);
static char* mode2string(CA_MODE mode);
static void mark_child_options(CMD_ARG* argp);

#define ROOT_DESCRIPTION "program name"

/**
 * Initializes the command line argument data structures.
 *
 * @param argc Command line argument count (as given to the main function)
 * @param argv[] Command line argument array of strings (as given to the main function)
 */
void cmdarg_init(int argc, char* argv[]) {
	parsing_complete = 0;
	register_error = 0;
	memset(&root, 0, sizeof(root));
	ll_init(&root.options);	
	root.opt_name[0] = '@';
	root.opt_name[1] = '\0';
	strncpy(root.opt_vname, argv[0], sizeof(root.opt_vname)-1);
	root.description = (char*)calloc(strlen(ROOT_DESCRIPTION) + 1, 1);
	strcpy(root.description, ROOT_DESCRIPTION);
	root.option_provided = 1;
	root.mode = CA_ROOT;
}

/**
 * Define a command line option and its syntax
 * @param opt_name Short name of the option. (If "x", then is typed as "-x" on the command line)
 * @param verbose_name Long name of the option. (If "xray" then is typed as "--xray" on the command line)
 * @param mode CA_MODE value specifying semantics to be enforced on this option.
 * @param description String used to describe the option when using command line help features.
 * @param default_value Default value if required, may be NULL
 * @param parent_opt_name Name of the parent option. NULL if this is a top levell option.
 * @return 0 on success, non-zero on error. An error indicates that an option of that name
 * 	has been previously registered.
 */
int cmdarg_register_option(char* opt_name, char* verbose_name, 
	CA_MODE mode, char* description, char* default_value, char* parent_opt_name) {
	
 	// 1. Verify this option has not yet been defined.
 	if (find_option(&root, opt_name)) {
 		fprintf(stderr, "Option %s has already been registered\n", opt_name);
 		register_error = 1;
 		return 1;
 	}
 	
 	// 2. If parent_option is null, add to the root option. Otherwise, search for
 	// the named option.
 	if (NULL == parent_opt_name) {
 		add_option(&root, opt_name, verbose_name, mode, description, default_value);
 	} else {
 		CMD_ARG* optionp;
 		
 		optionp = find_option(&root, parent_opt_name);
 		if (NULL == optionp) {
 			fprintf(stderr, "Error registering child option %s -- Parent Option %s has not been registered!\n",
 				opt_name, parent_opt_name);
 			register_error = 1;
 			return register_error;
 		}
 		add_option(optionp, opt_name, verbose_name, mode, description, default_value);		
 	} 
 	return 0;
}

/**
 * Once options have been registered, the application calls cmdarg_parse to
 * parse the command line arguments provided in this current invocation of the
 * application.
 *
 * @param argc Argument count as provided to the main function
 * @param argv[] Array of strings containing command line arguments as
 * 	provided to the main function
 * @return 0 on success, non-zero on failure.
 */
int cmdarg_parse(int argc, char* argv[]) {
	int iarg;
	int parse_error = 0;
	char* valp;
	CMD_ARG* argp;
	char opt_text[MAX_OPTION_NAME_LEN+1];
	char* opt_textp;
	
	if (parsing_complete) {
		// We've already parsed these arguments ... no need to do it again
		// This supports standard definitions of command line args by
		// ulppk components, like socketserver.c
		return 0;
	}

	if (register_error) {
		fprintf(stderr, "Command Line Option Parse Error: Option registration error previously detected\n");
		fprintf(stderr, "Program %s aborting!\n", argv[0]);
		exit(1);
	}
	
	// Mark all options defined as immediate children of
	// the root (program name) as enabled. After all,
	// the program name has clearly been provided.
	mark_child_options(&root);
	for (iarg = 1; iarg < argc && !parse_error; iarg++) {
		opt_textp = clean_option_text(opt_text, argv[iarg]);
		if (opt_textp == NULL) {
			// Did not find option when expected ... flag warning and continue loop
			fprintf(stderr, "Command Line Option Parse Error:"
				"Found text %s when expecting an option\n", argv[iarg]);
			continue;
		}
		argp = find_option(&root, opt_textp);
		if (argp == NULL) {
			fprintf(stderr, "Command Line Option Parse Error: Option %s not registered\n",
				argv[iarg]);
			parse_error = 1;
			break;
		}
		// We found a registered option that matches the command line argument
		switch (argp->mode) {
		case CA_SWITCH:
			argp->option_provided = 1;
			break;
		case CA_DEFAULT_ARG:
		case CA_OPTIONAL_ARG:
			if (iarg+1 >= argc) {
				fprintf(stderr, "Command Line Option Parse Error:\n"
					"Command Line Missing Value for Option %s\n", argv[iarg]);
				parse_error = 1;
			} else {
				if (valid_option(argv[iarg+1])) {
					// Next cmd line argument is an option parse error
					fprintf(stderr, "Command Line Option Parse Error:"
						"Option %s requires value before option %s\n",
						argv[iarg], argv[iarg+1]);
						parse_error = 1;
				} else {
					// Next cmd line argument is option value ... use it
					argp->option_provided = 1;
					valp = (char*)calloc(strlen(argv[iarg+1])+1, 1);
					strcpy(valp, argv[iarg+1]);
					argp->opt_value = valp;
					iarg++;
				}
			}
			break;
		case CA_REQUIRED_ARG:
			if (iarg+1 >= argc) {
				fprintf(stderr, "Command Line Option Parse Error:\n"
					"Command Line Missing Value for Required Option %s\n", argv[iarg]);
				parse_error = 1;
			} else {
				if (valid_option(argv[iarg+1])) {
					// Next cmd line argument is an option parse error
					fprintf(stderr, "Command Line Option Parse Error:"
						"Required Option %s requires value before option %s\n",
						argv[iarg], argv[iarg+1]);
						parse_error = 1;
				} else {
					// Next cmd line argument is option value ... use it
					argp->option_provided = 1;
					valp = (char*)calloc(strlen(argv[iarg+1])+1, 1);
					strcpy(valp, argv[iarg+1]);
					argp->opt_value = valp;
					iarg++;
				}
			}
			break;
		case CA_ROOT:
			fprintf(stderr, "Unexpected visitation of root node!\n");
			break;
		}
		// Now visit any child nodes of this node and mark them to indicate
		// the parent option has been provided. This allows required
		// options of "submenus" to be enforced ONLY when the parent
		// option has been provided.
		mark_child_options(argp);
	}
	if (!verify_required_args(&root)) {
		parse_error = 1;
	} else {
		parsing_complete = 1;
	}
 	return parse_error;
}

static char* mode2string(CA_MODE mode) {
	char* modestr;
	switch (mode) {
 		case CA_ROOT:
 			modestr = "PROGNM";
 			break;
 		case CA_SWITCH:
 			modestr = "SWITCH";
 			break;
 		case CA_DEFAULT_ARG:
 			modestr = "DEFARG";
 			break;
 		case CA_REQUIRED_ARG:
 			modestr = "REQARG";
 			break;
 		case CA_OPTIONAL_ARG:
 			modestr = "OPTARG";
 			break;
 	}
 	return modestr;
}

 static void print_cmdarg(CMD_ARG* argp) {
 	char fmtstr[256];
 	char *valstr;
 	char* modestr;
 	
 	modestr = mode2string(argp->mode);
 	
 	valstr = (argp->opt_value != NULL) ? argp->opt_value : "no-value"; 
 	if (strlen(argp->opt_vname) > 0) {
 		sprintf(fmtstr,"%s (%s)\t%s\t[%s]\t[%s]\n",argp->opt_name, argp->opt_vname,
 			valstr, modestr, (argp->description != NULL) ? argp->description : "no description");
 	} else {
 		sprintf(fmtstr,"%s (no vname)\t%s\t[%s]\t[%s]\n",argp->opt_name, 
 			valstr, modestr, (argp->description != NULL) ? argp->description : "no description");
 	}
 	fprintf(stderr, "%s", fmtstr);
 }
 
 /**
  * Show currently provided arguments, including defaults.
  * (Default values will be shown when no command line argument was
  * provided to override the default.  Otherwise, the override value
  * will be shown.)
  *
  * @param argp Pointer to command line argument node structure. Applications
  *  usually pass NULL so that the display begins with the root argument
  *  (program name)
  * @return count of active arguments/options.
  */
int cmdarg_print_args(CMD_ARG* argp) {
	int argcount = 0;
	CMD_ARG* child_argp;
	PLL_NODE nodep;
	
	if (NULL == argp) {
		argp = &root;
	}
	
	if (argp->option_provided) {
		print_cmdarg(argp);
		argcount++;
		nodep = argp->options.ll_fptr;
		while (nodep != NULL) {
			child_argp = (CMD_ARG*)nodep->data;
			argcount += cmdarg_print_args(child_argp);
			nodep = nodep->link;
		}
	}
	return argcount;
}

/**
 * Print the definitions of registered command line options.
 *
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @return count of active arguments/options.
 *
 */
int cmdarg_print_option_defs(CMD_ARG* argp) {
	int argcount = 0;
	CMD_ARG* child_argp;
	PLL_NODE nodep;

	if (NULL == argp) {
		argp = &root;
	}

	print_cmdarg(argp);
	argcount++;
	nodep = argp->options.ll_fptr;
	while (nodep != NULL) {
		child_argp = (CMD_ARG*)nodep->data;
		argcount += cmdarg_print_option_defs(child_argp);
		nodep = nodep->link;
	}
	return argcount;
}

/**
 * Show command line help, using definitions and descriptions
 * provided during command line option syntax registration.
 * (See cmdarg_register_option.)
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 */
int cmdarg_show_help(CMD_ARG* argp) {
	if (argp == NULL) {
		argp = &root;
	}
	return show_help(&root);
}

/**
 * Fetch a command argument. Return pointer to
 * CMD_ARG structure.
 *
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @param opt_name Short or long name of the option to be fetched.
 * @return Pointer to the matching option node, or NULL if not found.
 */
CMD_ARG* cmdarg_fetch(CMD_ARG* argp, char* opt_name) {
	if (NULL == argp) {
		argp = &root;
	}
	return find_option(argp,opt_name);
}

/**
 * Determine if a switch is present in the command line arguments.
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @param opt_name Short or long name of the option to be fetched.
 * @return 1 if switch was provided on command line, 0 if not.
 */
 int cmdarg_fetch_switch(CMD_ARG* argp, char* opt_name) {
 	int haveit = 0;
 	CMD_ARG* xargp = NULL;
 	if (NULL == argp) {
 		argp = &root;
 	}
 	xargp = find_option(argp, opt_name);
 	if (xargp) {
 		if (xargp->option_provided) {
 			haveit = 1;
 		}
 	}
 	return haveit;
 }
 
/**
 * Fetch a command argument. Parse the value
 * string as a long integer.
 *
 * This function will abort the application on various error conditions:
 * <ul>
 * 	<li>Null or zero length value when expecting a numeric string</li>
 * 	<li>String found but could not be parsed as an integer</li>
 * </ul>
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @param opt_name Short or long name of the option to be fetched.
 * @return long integer value of the option.
 */
long cmdarg_fetch_long(CMD_ARG* argp, char* opt_name) {
	long x;
	CMD_ARG* xargp;
	char* valp = NULL;
	
	if (NULL == argp) {
		argp = &root;
	}
	xargp = find_option(argp, opt_name);
	if ((xargp->opt_value == NULL) || (strlen(xargp->opt_value) == 0)) {
		if (xargp->mode == CA_DEFAULT_ARG) {
			valp = xargp->default_value;
		}
	} else {
		valp = xargp->opt_value;
	}
	if (valp == NULL) {
		fprintf(stderr, "ABORTING: NULL or zero length option value when expecting integer\n"
			"Option is %s (%s)\n",
			xargp->opt_name, xargp->opt_vname
		);
		exit(1);
	}
	if (valp != NULL) {
		errno = 0;
		x = strtol(valp, NULL, 0);
		if (errno != 0) {
			fprintf(stderr, "ABORTING: Unable to parse option value as integer\n"
				"Option is %s (%s), value is %s errno = %d (%s)\n",
				xargp->opt_name, xargp->opt_vname, xargp->opt_value, errno, 
				strerror(errno)
			);
			exit(1);
		}
	}
	return x;
}
/**
 * Fetch a command argument. Parse the value
 * string as an integer.
 *
 * This function will abort the application on various error conditions:
 * <ul>
 * 	<li>Null or zero length value when expecting a numeric string</li>
 * 	<li>String found but could not be parsed as an integer</li>
 * </ul>
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @param opt_name Short or long name of the option to be fetched.
 * @return integer value of the option.
 */

int cmdarg_fetch_int(CMD_ARG* argp, char* opt_name) {
	return (int)cmdarg_fetch_long(argp, opt_name);
}

/**
 * Fetch a command argument. Return the value string
 *
 * This function will abort the application on various error conditions:
 * <ul>
 * 	<li>Null or zero length value when expecting a numeric string</li>
 * 	<li>String found but could not be parsed as an integer</li>
 * </ul>
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @param opt_name Short or long name of the option to be fetched.
 * @return Pointer to the value string, or NULL if not found.
 */
char* cmdarg_fetch_string(CMD_ARG* argp, char* opt_name) {
	CMD_ARG* xargp;
	
	if (NULL == argp) {
		argp = &root;
	}
	xargp = find_option(argp, opt_name);
	if (xargp != NULL) {
		if ((NULL == xargp->opt_value) && (xargp->mode == CA_DEFAULT_ARG)) {
			return xargp->default_value;
		} else {
			return xargp->opt_value;
		}
	}
	return NULL;
}

/**
 * Convenience function. Buffer is a char array of size
 * buffer_size. It will receive the command argument
 * value string. Returns size of loaded string or < 0
 * if error.
 *
 * @param buffer Buffer to receive an option value string
 * @param buffer_size Max number of characters to copy to buffer.
 * @param argp Pointer to command line argument node structure. Applications
 *  usually pass NULL so that the display begins with the root argument
 *  (program name)
 * @param opt_name Short or long name of the option to be fetched.
 * @return length of the loaded string or < 0 on error.
 */
int cmdarg_load_string(char* buffer, size_t buffer_size, CMD_ARG* argp, char* opt_name) {
	char* valp;
	int retval = -1;
	memset(buffer, 0, buffer_size);
	valp = cmdarg_fetch_string(argp, opt_name);
	if (valp != NULL) {
		strncpy(buffer, valp, buffer_size - 1);
		retval = strlen(buffer);
	}
	return retval;
}

static void show_node_help(CMD_ARG* argp) {
	char* modestr;
	char* descstr;
	char namebuff[256];
	char defbuff[256];
	
	if (strlen(argp->opt_vname) != 0) {
		sprintf(namebuff, "-%s (--%s)", argp->opt_name, argp->opt_vname);
	} else {
		sprintf(namebuff, "-%s", argp->opt_name);
	}
	modestr = mode2string(argp->mode);
	if (argp->mode == CA_DEFAULT_ARG) {
		sprintf(defbuff, "%s -- [%s]", modestr, argp->default_value);
		modestr = defbuff;
	}
	if ((argp->description != NULL) && (strlen(argp->description) != 0)) {
		descstr = argp->description;
	} else {
		descstr = "No Description";
	}
					
	fprintf(stderr,
		"%s | %s | %s\n",
		namebuff,
		modestr,
		descstr
	);
}
static int show_help(CMD_ARG* argp) {
	PLL_NODE nodep;
	CMD_ARG* list_argp;
	int retval= 0;
	
	// Print this node's description
	show_node_help(argp);
	
	// Now travel down the list of sub nodes
	nodep = argp->options.ll_fptr;
	while (nodep != NULL) {
		list_argp = (CMD_ARG*)nodep->data;
		retval = show_help(list_argp);			// recursive call
		nodep = nodep->link;
	}
	return retval;
}

static int add_option(CMD_ARG* argp, char* opt_name, char* opt_vname, CA_MODE mode, 
	char* description, char* default_value) {

	CMD_ARG *newargp;
	PLL_NODE nodep;
	
	// If parent is NULL, presume root
	if (NULL == argp) {
		argp = &root;
	}
	
	// Construct the new argument node.
	
	newargp = (CMD_ARG*)calloc(sizeof(CMD_ARG), 1);
	newargp->opt_name[0] = opt_name[0];
	if ((opt_vname != NULL) && (strlen(opt_vname) > 0)) {
		strncpy(newargp->opt_vname, opt_vname, sizeof(argp->opt_vname) - 1);
	}
	if ((description != NULL) && (strlen(description) > 0)) {
		newargp->description = (char*)calloc(strlen(description) + 1, 1);
		strcpy(newargp->description, description);
	}
	newargp->mode = mode;
	if ((default_value != NULL) && (strlen(default_value) > 0)) {
		 newargp->default_value = (char*)calloc(strlen(default_value) + 1, 1);
		 strcpy(newargp->default_value, default_value);
	}
	newargp->parentp = argp;
	
	// Add to the list of parent node options
	
	nodep = (PLL_NODE)calloc(sizeof(LL_NODE), 1);
	nodep->data = newargp;
	ll_addback(&argp->options, nodep);
	return 0;
}

static int test_option(CMD_ARG* argp, char* name) {
	int is_match = 0;
	if (NULL != argp) {
		if (strcmp(argp->opt_name, name) == 0) {
			is_match = 1;
		} else if (strcmp(argp->opt_vname, name) == 0) {
			is_match = 1;
		}
	}
	return is_match;
}

static CMD_ARG* find_option(CMD_ARG* argp, char* name) {
	PLL_NODE nodep;
	CMD_ARG* child_argp;
	CMD_ARG* match_argp = NULL;

	if (test_option(argp, name)) {
		return argp;
	}
	nodep = argp->options.ll_fptr;
	while ((nodep != NULL) && (match_argp == NULL)) {
		child_argp = (CMD_ARG*)nodep->data;
		match_argp = find_option(child_argp, name);
		nodep = nodep->link;
	}
	return match_argp;
}

static int valid_option(char* opt_name) {
	char* cp;
	int is_valid = 1;
	
	cp = opt_name;
	if (*cp != '-') {
		is_valid = 0;
	}
	return is_valid;
}		

/*
 ***************************
 * Verify that all required arguments have been provided.
 ***************************
 */
static int required_status_ok(CMD_ARG* argp) {
	int status = 1;
	if (argp->mode == CA_REQUIRED_ARG) {
		if (argp->option_enabled) {
			status = argp->option_provided ;
		}
		if (!status) {
			fprintf(stderr, "Command Line Parse Error:"
				"Required option %s not provided!\n", argp->opt_name);
		}
	}
	return status;
}
 
static int verify_required_args(CMD_ARG* argp) {
	int status = 0;
	PLL_NODE nodep;
	CMD_ARG* child_argp;
	
	status = required_status_ok(argp);
	if (status) {
		nodep = argp->options.ll_fptr;
		while ((nodep != NULL) && (status)) {
			child_argp = (CMD_ARG*)nodep->data;
			if ((child_argp->option_enabled) && (child_argp->option_provided)) {
				status = verify_required_args(child_argp);
			}
			nodep = nodep->link;
		}
	}
	return status;	
}

static char* clean_option_text(char* buff, char* opt_name) {
	char* cp;

	cp = opt_name;
	if (*cp == '-') {
		// Leading '-' indicates short or verbose option
		cp++;
		if (*cp == '-') { 
			// A second '-' indicates a verbose option
			cp++;		// point to 1st char of option text
		}
		strcpy(buff, cp);
		cp = buff;
	} else {
		// Not a valid option
		cp = NULL;
	}
	return cp;
}

static void mark_child_options(CMD_ARG* argp) {
	LL_HEAD* listp;
	PLL_NODE fptr;
	PLL_NODE bptr;
	PLL_NODE nodep;
	CMD_ARG* childp;

	listp = &argp->options;
	ll_getptrs(listp, &fptr, &bptr);

	nodep = fptr;
	while (nodep != NULL) {
		childp = (CMD_ARG*)nodep->data;
		childp->option_enabled = 1;
		nodep = ll_iterate(nodep);
	}
}

