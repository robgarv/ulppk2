
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
#ifndef DIAGNOSTICS_H_
#define DIAGNOSTICS_H_

/**
 * @file diagnostics.h
 *
 * @brief Utilities for handling diagnostic error output
 *
 * To enable debug trace output, compile with -DULPPK_DEBUG.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef ULPPK_DEBUG
#define ULPPK_DEBUG_FLAG 1
#else
#define ULPPK_DEBUG_FLAG 0
#endif

#define ERR_MSG(buffer,  ...) err_msg(buffer, __FUNCTION__,__FILE__, __LINE__,  __VA_ARGS__)
#define CRASH(msg) crash(__FUNCTION__, __FILE__, __LINE__, msg);
#define APP_ERR(f, ...) app_error(f, __FUNCTION__,__FILE__, __LINE__, __VA_ARGS__)
#define DBG_TRACE(f, ...) dbg_trace(f, ULPPK_DEBUG_FLAG,  __FUNCTION__,__FILE__, __LINE__, __VA_ARGS__)

/*
 * Format an error message. If buffp is NULL, then memory is allocated by the 
 * function. This memory MUST be released by calling free. Otherwise, buffp is
 * a buffer of sufficient size provided by the caller, and which will receive 
 * the formatted error message.
 */
char* err_msg(char* buffp, const char* func, const char* file, int line, const char* fmtp, ...);

/*
 * Only call function crash when the application is in a massively bad state
 * and you want to stop it in its tracks. For less radical conditions, use
 * app_error.
 * Write an error message to stderr, then force program termination 
 * and a core dump. crash deliberately causes a SIGSEGV. 
 */
void crash(const char* func, const char* file, int line, const char* msg);

/*
 * Write an error to the file f and exit the application gracefully.
 */
void app_error(FILE* f, const char* func, const char* file, int line, const char* fmtp, ...);

/*
 * Write an error to the file f and continue
 */
void dbg_trace(FILE* f, int debug_flag, const char* func, const char* file, int line, const char* fmtp, ...);

/*
 * Format an error message. If buffp is NULL, then memory is allocated by the
 * function. This memory MUST be released by calling free. Otherwise, buffp is
 * a buffer of sufficient size provided by the caller, and which will receive
 * the formatted error message.
 */
char* verr_msg(char* buffp, const char* label, const char* func, const char* file, int line, const char* fmtp, va_list ap);

#endif /*DIAGNOSTICS_H_*/
