
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
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <ulppkproc/ifile.h>

static INIFILE_NODE* inifilep;

extern int yylineno;
extern char* yytext;

IF_NODE_TYPE node_type;
IF_DATA_TYPE data_type;
INIFILE_NODE* nodep = NULL;;
INIFILE_NODE* current_sectionp = NULL;
INIFILE_NODE* inifilerootp = NULL;

FILE* ferr = NULL;		// error reporting file stream

int debug_print(char* fmtp, ...) {
	va_list ap;
	va_start(ap, fmtp);
#ifdef _PARSER_DEBUG
	vfprintf(stdout, fmtp, ap);
#endif
	va_end(ap);
	return 0;
}

%}


%union {
	char* sval;
	int nval;
	double dval;
}


%token <sval> SECTION_DECL
%token <sval> IDENTIFIER
%token <sval> STRING
%token <nval> NUMBER
%token <nval> HEXNUMBER
%token <sval> IPSTRING

%%

inifile:	section
	|	inifile section			{ debug_print("INIFILE\n"); }
	;

section:	section_decl
	|	section assignment
	;

section_decl:	SECTION_DECL			{ 
							debug_print("SECTION: %s\n", $1); 
							current_sectionp = if_new_section($1);
							if_add_section_node(inifilep, current_sectionp);
						}
	;

assignment:	identifier '=' expression	{
	  						debug_print("IDENTIFIER: %s|%s=", current_sectionp->name, nodep->name);
							switch (nodep->element.data_type) {
							case IFDT_STRING:
								debug_print("|%s|\n", nodep->element.szvalue);
								break;
							case IFDT_NUMBER:
								debug_print("|%d(%x)|\n", nodep->element.lvalue, nodep->element.lvalue);
								break;
							default:
								debug_print("UNKNOWN\n");
								break;
							}
	  						if_add_element(inifilep, current_sectionp->name,  nodep);
						}
	;

identifier:	IDENTIFIER			{ 
	  						debug_print("IDENTIFIER: %s\n", $1); 
							nodep = if_new_element($1);
						}
	;
expression:	STRING				{ 
	  						debug_print("STRINGEXP: %s\n", $1);
							nodep->element.data_type = IFDT_STRING; 
							nodep->element.szvalue = strdup($1); 
						} 
	|	IPSTRING			{ 
	  						debug_print("IPSTRINGEXP: %s\n", $1);
							nodep->element.data_type = IFDT_STRING; 
							nodep->element.szvalue = strdup($1); 
						} 

	|	NUMBER				{ 
							debug_print("NUMBEREXP: %d\n", $1);
							nodep->element.data_type = IFDT_NUMBER;
							nodep->element.lvalue = $1;
						}
	|	HEXNUMBER			{ 
							debug_print("HEXNUMBEREXP: %x\n", $1);
							nodep->element.data_type = IFDT_NUMBER;
							nodep->element.lvalue = $1;
						}

	;

%%



extern FILE* yyin;

#ifdef _LIBINIFILE_TEST_PARSER
int main(int argc, char* argv[]) {

	yyin = stdin;
	inifilep = if_new_inifile("stdin");
	if_add_inifile(&inifilerootp, inifilep);
	while (!feof(yyin)) {
		yyparse();
	}
	if_walk_file(inifilep);
	return 0;
}
#else 

int if_parse_inifile(FILE* fp, char* fpath) {

	char* filepath;

	if (fpath == NULL) {
		yyin = stdin;
		filepath = "stdin";
	} else {
		filepath = fpath;
		yyin = fp;
	}
	if (ferr == NULL) {
		ferr = stderr;
	}

	inifilep = if_new_inifile(filepath);
	if_add_inifile(&inifilerootp, inifilep);
	while (!feof(yyin)) {
		yyparse();
	}
	return 0;
}

#endif

int yyerror(char* msg) {
	fprintf(ferr, "%d: %s at [%s]\n", yylineno, msg, yytext);
	return 0;
}

INIFILE_NODE* if_get_root() {
	return inifilerootp;
}

void if_set_err_file(FILE* fp) {
	ferr = fp;
}

