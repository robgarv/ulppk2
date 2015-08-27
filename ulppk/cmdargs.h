
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
#ifndef CMDARGS_H_
#define CMDARGS_H_

/**
 * @file cmdargs.h
 *
 * @brief Definitions and declarations for the command line parser and access
 *  library cmdargs
 *
 * For the purposes of this command line argument parser
 * command arguments are ins the form of
 * 
 * <ol>
 * <li>an option without option value (empty option)</li>
 * <li>a list of options without option value</li>
 * <li>an option with an option value</li>
 * <li>a verbose option (empty or otherwise)</li>
 * </ol>
 * 
 * Options are prefixed by a '-'. Verbose options are prefixed
 * by "--".
 * Examples
 * 
 * Program myapp invoked with an empty option
 * 
 * myapp -a
 * 
 * Program myapp invoked with a list of empty options
 * myapp -aBcdfGh
 * 
 * Program mysapp invoked with a non-empty option
 * 
 * myapp -F myfile.txt
 * 
 * These can be mixed up
 * 
 * myapp -a -F myfile.txt -bght
 * 
 * Here's a verbose option for the -F option
 * 
 * myapp -a --filename myfile.txt -bght
 * 
 * Now, let's say if you supply the -a option, you must also supply additional
 * options -b -c -d.
 * 
 * myapps -a -b
 * 
 * will produce a parsing error.
 * 
 * myapps -a -b -c -d
 * 
 * will parse successfully.
 * 
 * Options can be defined as being
 * 
 * 1) required
 * 2) optional
 * 3) optional with default value
 * 
 */
 
 #include <llacc.h>
 
 /*
 typedef enum {
 	CA_OPTION = 0,
 	CA_VERBOSE_OPTION,
 	CA_VALUE
 } CA_TOKEN;
 */
 
/**
 * Command line option "mode". Defines how the option is to
 * be interpreted.
 */
 typedef enum {
 	CA_ROOT = 0,				///< special ... for "root" argument node
 	CA_SWITCH,					///< optional -- no value (simple switch like -a)
 	CA_DEFAULT_ARG,				///< default value provided (-x MYVALUE, otherwise default is applied )
 	CA_REQUIRED_ARG,			///< required, no default value provided (-x VALUE is required)
 	CA_OPTIONAL_ARG				///< optional, value must be provided (-x VALUE works, -x doesn't)
 } CA_MODE;
 
 #define MAX_OPTION_NAME_LEN 32
 
 /**
  * Internal representation of a registered command line option.
  */
 typedef struct _cmdarg{
 	char opt_name[2];			///< Short name of the option
 	char opt_vname[MAX_OPTION_NAME_LEN];	//< Long name of the option
 	char* default_value;		///< Default value
 	char* description;			///< Description. Used in printing command line help
 	char *opt_value;			///< Value of the option
 	CA_MODE mode;				///< Mode value (see CA_MODE)
 	int option_provided;		///< 1 if seen in command line arguments (provided)
 	int option_enabled;			///< 1 if option is enabled because parent option has been provided
 	struct _cmdarg* parentp;	///< ptr to parent node/option
 	LL_HEAD options;			///< linked list of child options
 } CMD_ARG;
#ifdef __cplusplus
extern "C" {
#endif
 
 void cmdarg_init(int argc, char* argv[]);
 int cmdarg_register_option(char* opt_name, char* verbose_name, 
 	CA_MODE mode, char* description, char* default_value, char* parent_opt_name);
 int cmdarg_parse(int argc, char* argv[]);
 int cmdarg_print_args(CMD_ARG* argp);
 int cmdarg_print_option_defs(CMD_ARG* argp);
 int cmdarg_show_help(CMD_ARG* argp);
 CMD_ARG* cmdarg_fetch(CMD_ARG* argp, char* opt_name);
 int cmdarg_fetch_switch(CMD_ARG* argp, char* opt_name);
 int cmdarg_fetch_int(CMD_ARG* argp, char* opt_name);
 long cmdarg_fetch_long(CMD_ARG* argp, char* opt_name);
 char* cmdarg_fetch_string(CMD_ARG* argp, char* opt_name);
 int cmdarg_load_string(char* buffer, size_t buffer_size, CMD_ARG* argp, char* opt_name);


#ifdef __cplusplus
}
#endif
 
#endif /*CMDARGS_H_*/
