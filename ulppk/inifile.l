
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
#include <string.h>
#include <stdarg.h>
#include "inifileparser.h"

static char* get_text();
static char* append_text(char* text);
static void clear_text();
static int debug_print(char* fmtp, ...);

char* textbuff = NULL;

/*


*/
%}

%option yylineno
%s SLSECTION
%s SLASSIGNMENT

%%

[ \t]*\#.*\n			{ ; }

<SLSECTION>[-a-zA-Z0-9_.]+	{
					append_text(yytext);
					debug_print("Section: %s\n", get_text());
					yylval.sval = strdup(get_text());
					return SECTION_DECL;
				}

[ \t]*"[" 				{ 
					BEGIN SLSECTION;
					debug_print("BEGIN SLSECTION\n");
					clear_text();
				}

"0x"[0-9a-fA-F]+ |
"0X"[0-9a-fA-F]+			{
					debug_print("HEXNUMBER: %s\n", yytext);
					yylval.nval = strtol(yytext, NULL, 16);
					BEGIN SLASSIGNMENT;	
					return HEXNUMBER;
				}

<SLSECTION>"]"			{
					BEGIN SLASSIGNMENT;	
					debug_print("END SLSECTION\n");
				}

[0-9]+				{
					debug_print("Number: %s\n", yytext); 
					yylval.nval = atoi(yytext);
					BEGIN SLASSIGNMENT;	
					return NUMBER;
				}

[0-9]+.[0-9].+[0-9]+.[0-9]+	{
					debug_print("IP String:%s\n", yytext);
					yylval.sval = strdup(yytext);
					BEGIN SLASSIGNMENT;
					return IPSTRING;
				}

<SLASSIGNMENT>[a-zA-Z]+[-a-zA-Z0-9._]+	{
					debug_print("Identifier: %s\n", yytext);
					yylval.sval = strdup(yytext);
					BEGIN 0;
					return IDENTIFIER;
				}
\"[^"\n]*[\"\n] |
[^][ \t\n=]+			{
					debug_print("String: %s\n", yytext);
					yylval.sval = strdup(yytext);
					BEGIN SLASSIGNMENT;
					return STRING;
				}


"="				{ 
					debug_print("=TOKEN: %d / %c", yytext[0], yytext[0]);
					return yytext[0]; 
				}

[\n\t ]			;

%%

int yywrap() {
	return 1;
}

static void clear_text() {
	if (textbuff != NULL) {
		free(textbuff);
		textbuff = NULL;
	}
}

static char* get_text() {
	return textbuff;
}

static char* append_text(char* text) {
	if (textbuff == NULL) {
		textbuff = strdup(text);
	} else {
		int newsize;
		char* p;

		newsize = (strlen(textbuff) + 1) + sizeof(text);
		p = calloc(newsize, sizeof(char));
		strcpy(p, textbuff);
		strcat(textbuff, text);
		free(textbuff);
		textbuff = p;
	}
	return textbuff;
}

static int debug_print(char* fmtp, ...) {
	va_list ap;
	va_start(ap, fmtp);
#ifdef _LEXER_DEBUG
	vfprintf(stdout, fmtp, ap);
#endif
	va_end(ap);
	return 0;
}

//#define _STANDALONE_TEST
#ifdef _STANDALONE_TEST

YYSTYPE yylval;

int main(int argc, char* argv[]) {

	int token;
	token = yylex();
	printf("LEXER returned: %d\n", token);
	while (token != 0) {
		token = yylex();
		printf("LEXER returned: %d\n", token);
		switch (token) {
		case SECTION_DECL:
			printf("\tSECTION: %s\n", yylval.sval);
			break;
		case STRING:
			printf("\tSTRING: %s\n", yylval.sval);
			break;
		case NUMBER:
			printf("\tNUMBER: %d\n", yylval.nval);
			break;
		case HEXNUMBER:
			printf("\tHEXNUMBER: %x\n", yylval.nval);
			break;
		default:
			printf("\tToken: %d / %c\n", token, token);
		}
	}
	return 0;
}

#endif

/*
 **** SCRAP ****

\.				{ 
					debug_print(".TOKEN: %d / %c", yytext[0], yytext[0]);
					return yytext[0]; 
				}
**********
*/
