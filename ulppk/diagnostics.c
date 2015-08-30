
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <diagnostics.h>

/**
 * @file diagnostics.c
 *
 * @brief Utilities for handling diagnostic error output
 *
 * To enable debug trace output, compile with -DULPPK_DEBUG.
 *
 */

/**
 * Format an error message. Similar to err_msg but provides
 * handling for errno and errno's related error message string.
 *
 * @param buffp Pointer to buffer to receive text. If buffp is NULL, then memory is allocated by the
 * function. This memory MUST be released by calling free. Otherwise, buffp is
 * a buffer of sufficient size provided by the caller, and which will receive 
 * the formatted error message.
 * @param label Label shows up first in the error text
 * @param func Name of function in which this function was called.
 * @param file Source file in which function is defined
 * @param line Line at which verr_msg was invoked.
 * @param fmtp A format string
 * @param ap Variable length argument list.
 */
char* verr_msg(char* buffp, const char* label, const char* func, const char* file, int line, const char* fmtp, va_list ap) {
	char* cp;
	
	// If buffp is NULL, caller is expecting us to allocate space for the error
	// message. We can estimate its size ... but this is not bullet proof. 
	// A really elavorate format string with lots of arguments could screw this up. 

	if (NULL == buffp) {
		buffp = calloc((strlen(func) + strlen(file) + strlen(fmtp) + PATH_MAX), sizeof(char));
	}
	cp = buffp;
	if (errno) { 
		cp += sprintf(buffp, "%s:	PID [%d] FUNC [%s] SRC [%s] LINE [%d]\n\terrno [%d | %s]\n\tMSG [",
			label, getpid(), func, file, line,  errno, strerror(errno));
	} else {
		cp += sprintf(buffp, "%s:	PID [%d] FUNC [%s] SRC [%s] LINE [%d]\n\tMSG [",
			label, getpid(), func, file, line);
	}
	cp += vsprintf(cp, fmtp, ap);
	strcat(buffp, "]\n");
	return buffp;
}


/**
 * Format an error message. Accepts a variable number of arguments.
 *
 * @param buffp  buffp is a buffer of sufficient size provided by the caller,
 * 	and which will receive the formatted error message.
 * @param func Name of function in which this function was called.
 * @param file Source file in which function is defined
 * @param line Line at which verr_msg was invoked.
 * @param fmtp A format string
 * @param ... variable number of arguments follows.
 */
char* err_msg(char* buffp, const char* func, const char* file, int line, const char* fmtp, ...) {
	va_list ap;
	char* msgp;

	va_start(ap, fmtp);
	msgp = verr_msg(buffp, "Error",  func, file, line, fmtp, ap);
	va_end(ap);
	return msgp;
}

#if 0 
void crash(const char* func, const char* file, int line, const char* msg) {
	static char cbuff[2048];
	char* pcrash = NULL;
	
	err_msg(cbuff, func, file, line, msg);
	fprintf(stderr, "CRASH: %s\n", cbuff);
	
	// This ought to cause a SIGSEGV ... a rude way to stop this program 
	// right in its nasty little tracks
	*pcrash = 1;
}
#endif

/**
 * Format an error message, writes to a file  and terminates the
 * application. Accepts a variable number of arguments. Normally
 * called using the APP_ERR macro in diagnostics.h.
 *
 * @param f The file stream (FILE*) to which the message is written.
 * @param func Name of function in which this function was called.
 * @param file Source file in which function is defined
 * @param line Line at which verr_msg was invoked.
 * @param fmtp A format string
 * @param ... variable number of arguments follows.
 */
void app_error(FILE* f, const char* func, const char* file, int line, const char* fmtp, ...) {
	va_list ap;
	char* msgp;

	va_start(ap, fmtp);
	msgp = verr_msg(NULL, "AppErr", func, file, line, fmtp, ap);
	// msgp = (char*)calloc(512,1); 
	// vsprintf(msgp, fmtp, ap);
	fprintf(f, "%s", msgp);
	fflush(f);
	free(msgp);
	va_end(ap);
	exit(1);
}


/**
 * Format an error message. Accepts a variable number of arguments.
 * Used to produce debug trace output. Normally called using the
 * DBG_TRACE macro in diagnostics.h
 *
 * @param f The file stream (FILE*) to which the message is written.
 * @param debug_flag If false, then debug output is suppressed.
 * @param func Name of function in which this function was called.
 * @param file Source file in which function is defined
 * @param line Line at which verr_msg was invoked.
 * @param fmtp A format string
 * @param ... variable number of arguments follows.
 */
void dbg_trace(FILE* f, int debug_flag,  const char* func, const char* file, int line, const char* fmtp, ...) {
	va_list ap;
	char* msgp;

	va_start(ap, fmtp);
	if (debug_flag) {
		msgp = verr_msg(NULL, "TRACE", func, file, line, fmtp, ap);
		// msgp = (char*)calloc(512,1); 
		// vsprintf(msgp, fmtp, ap);
		fprintf(f, "%s", msgp);
		fflush(f);
		free(msgp);
	}
	va_end(ap);
}
