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
#ifndef _ULPPK_LOG_H
#define _ULPPK_LOG_H

#include <syslog.h>
#include <stdarg.h>

/**
 * Logging levels.
 */
typedef enum {
    ULPPK_LOG_DEBUG = 0,		///< Debugging info
    ULPPK_LOG_INFO,				///< General info
    ULPPK_LOG_WARN,				///< Warnings
    ULPPK_LOG_ERROR,			///< Serious errors. Indicates a bug in apps or ulppk library
    ULPPK_LOG_FATAL_ERROR,		///< Errors that preclude continuing normal operation. Usual environment or setup errors.
    ULPPK_LOG_CRASH_ERROR		///< The program cannot continue. Core dump attempted.
} ULPPK_LOG_LEVEL;

static char*	log_level_text [] = {
	"DEBUG : ",
	"INFO : ",
	"WARNING : ",
	"ERROR : ",
	"FATAL : ",
	"CRASH : "
	};

/**
 * Logging destinations. SYSLOG or console.
 */
#define ULPPK_LOGDEST_CONSOLE 1
#define ULPPK_LOGDEST_SYSLOG 2
#define ULPPK_LOGDEST_ALL 3

#define LEN_ULPPK_LOG 1024

/**
 * Macro that wraps call to ulppk_log. Provide log level constant,
 * a printf format string, followed by consistent sequence of
 * arguments. (Works like printf.)
 */
#define ULPPK_LOG(level, ...) ulppk_log(__FUNCTION__,__FILE__,__LINE__,level,__VA_ARGS__)
/**
 * Macro that wraps call to ulppk_log and forces log level to ULPPK_LOG_CRASH.
 * This will cause the app to core dump. Used to handle and log catastrophic errors.
 *
 * Provide a printf format string, followed by consistent sequence of
 * arguments. (Works like printf.)
 */
#define ULPPK_CRASH(...) ulppk_log(__FUNCTION__, __FILE__, __LINE__,ULPPK_LOG_CRASH_ERROR, __VA_ARGS__)
int ulppk_log_set_logconfig(int log_dest, const char* ident, int options, int facility);
int ulppk_log(const char* func, const char* file, int line, ULPPK_LOG_LEVEL text_level, const char* fmtp, ...);
int ulppk_vlog(const char* func, const char* file, int line, ULPPK_LOG_LEVEL text_level, const char* fmtp, va_list ap);
int log_app_start(int argc, char* argv[]);
#endif
