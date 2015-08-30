
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


#include <cmdargs.h>

static int register_cmdline(int argc, char* argv[]) {
	int status = 0;
	
	cmdarg_init(argc, argv);
	// If -a switch provided, then "submenu 1" must be provided
	status |= cmdarg_register_option("a", "alpha", CA_SWITCH, "Alpha switch", NULL, NULL);
	
	// "submenu 1 ... a switch (--ALPHA-1), a defaulted arg, a required arg, and an optional
	status |= cmdarg_register_option("A", "ALPHA-1", CA_SWITCH, "ALPHA-1 Switch", NULL, "a");
	status |= cmdarg_register_option("B",NULL,CA_DEFAULT_ARG, "Default arg B1",  "DefaultB1", "a");
	status |= cmdarg_register_option("C","CHARLIE-1", CA_REQUIRED_ARG, "Required argument C1", NULL, "a");
	status |= cmdarg_register_option("D","DELTA-1", CA_OPTIONAL_ARG, "Optional arg D1", NULL, "a");
	// end of submenu 1
	
	status |= cmdarg_register_option("b", "beta", CA_OPTIONAL_ARG, "Optional arg b0", NULL, NULL);
	
	return status;
}

int main(int argc, char* argv[]) {
	int status = 0;
	int i;
	int ival;
	char* valp;
	
	// Define and register the expected command line arguments.
	status = register_cmdline(argc, argv);
	
	if (status) {
		fprintf(stdout, "Registration error was reported! Parsing should cause abort\n");
	} else {
		fprintf(stdout, "Registration successful\n");
	}
	
	fprintf(stdout, "=================================\n");
	cmdarg_print_option_defs(NULL);
	fprintf(stdout, "=================================\n");

	// Print help
	
	cmdarg_show_help(NULL);
	
	// Print the command line 
	for (i = 0; i < argc; i++) {
		fputs(argv[i], stderr);
		fputs(" ", stderr);
	}
	fputs("\n", stderr);
	
	// Parse 'em
	status = cmdarg_parse(argc,argv);
	
	if (status) {
		fprintf(stdout, "cmdarg_parse returned an error status");
	}
	
	// Now show what we parsed
	cmdarg_print_args(NULL);
	
	// Now fetch the -D option and get value as a string
	valp = cmdarg_fetch_string(NULL, "D");
	
	printf("-D = %s\n", valp);
	
	// Fetch -C option as integer
	// ival = cmdarg_fetch_int(NULL, "CHARLIE-1");
	// printf("--CHARLIE-1 = %d\n", ival);
	
	printf("Normal exit\n");
	return 0;
}
