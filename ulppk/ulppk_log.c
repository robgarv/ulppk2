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

/**
 * @file
 * a logging mechanism that .ini file settings Logging level
 * can be changed "on the fly" but setting the [logging] variable log_level
 * to the desired numeric value. (See ulppk_log.h for valid values and
 * their meanings.)
 *
*/

/* Includes */
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <bool.h>

#include "ifile.h"
#include "ulppk_log.h"

/*
 * Logging destination. A bit map of log destinations
 * Bit 0: (val 1) => standard error
 * Bit 1: (val 2) => syslog
 * Bit 0+1: (val 3) => standard error and syslog
 *
 * Clearing all bits disables logging.
 */
static int log_dest = ULPPK_LOGDEST_CONSOLE;

static ULPPK_LOG_LEVEL  log_level;
static int have_log_vars = FALSE;
static char* ulppk_corepath;

static void get_log_vars( )
{
	INIFILE_NODE* inifilep;
	INIFILE_NODE* varp;

	log_level = ULPPK_LOG_DEBUG;
	ulppk_corepath = "/var/crash";

	inifilep = if_get_root();
	if (NULL == inifilep) {
		return;
	}
	varp=if_get_element(inifilep, "logging","log_level");
	if (varp == NULL) {
		syslog(LOG_INFO, "default log_level  used %d", log_level);
	} else {
		log_level = varp->element.lvalue;
		syslog(LOG_INFO, "log_level set to %d", log_level);
	}

	varp = if_get_element(inifilep, "environment","core_dir");
	if (varp == NULL) {
		// Can't call ULPPK_LOG from here ... recurses without limit
		syslog(LOG_INFO, "if_get_element returned NULL for core_dir using default %s", ulppk_corepath);
	}
	return;
}

/*
 * Maps a ULPPK log level to a syslog priority code
 */
static int syslog_priority(ULPPK_LOG_LEVEL level) {
	static int pri[] = {
			LOG_DEBUG,
			LOG_INFO,
			LOG_WARNING,
			LOG_ERR,
			LOG_CRIT,
			LOG_CRIT
	};
	if ((level > ULPPK_LOG_CRASH_ERROR) || (level < ULPPK_LOG_DEBUG)) {
		level = ULPPK_LOG_ERROR;
	}
	return pri[level];
}

static void route_log(ULPPK_LOG_LEVEL level, char* text) {
	if (log_dest & ULPPK_LOGDEST_SYSLOG) {
		int syslog_pri;

		syslog_pri= syslog_priority(level);
		syslog(syslog_pri, "%s", text);
	}
	if (log_dest & ULPPK_LOGDEST_CONSOLE) {
		fprintf(stderr, "%s\n", text);
	}
}

/**
 * @brief Set the log destination and if SYSLOG is a destination, sets
 * openlog option and facility codes. Returns previous log destination
 * bit mask.
 *
 * This function should be called BEFORE calling sysconfig_parse_inifile.
 *
 * log_dest specifies the log destination and
 * is one of the following values:
 * <ul>
 *	<li>ULPPK_LOGMODE_CONSOLE 1</li>
 *	<li>ULPPK_LOGMODE_SYSLOG 2</li>
 * 	<li>ULPPK_LOGMODE_ALL 3</li>
 * </ul>
 *
 * @param new_dest (see ulppk_log.h for details)
 * @param ident Identify string, usually the app name
 * @param options  See syslog(3). Meaningless if log_dest is
 * 	ULPPK_LOGDEST_CONSOLE
 * @param facility See syslog(3). Meaningless if log_dest is
 * 	ULPPK_LOGDEST_CONSOLE
 */
int ulppk_log_set_logconfig(int new_dest, const char* ident, int options, int facility) {
	int old_dest;

	new_dest = new_dest & ULPPK_LOGDEST_ALL;
	old_dest = log_dest;
	log_dest = new_dest;

	if (new_dest & ULPPK_LOGDEST_SYSLOG) {
		openlog(ident, options, facility);
	}
	return old_dest;
}


/**
 * Write to the log(s). Intended to be called from the ULPPK_LOG macro (see ulppk_log.h).
 *
 * Only log text with text level >= the established log level is printed to the log(s).
 * The log level is set in the .ini file for the calling app (section [logging], variable
 * log_level).If no value was provided, the default if ULPPK_LOG_DEBUG (0) is used.
 *
 *
 * @param func Name of the calling function
 * @param file File in which calling function is implemented.
 * @param line Line number at which ulppk_log was called.
 * @param text_level The logging (text) level of the log write request.
 * @param fmtp A printf style format string
 * @param ... variable number of arguments consistent with the fmtp format string
 * @return non-zero if text_level >= logging level (printed to log).
 */
int  ulppk_log(const char* func, const char* file, int line, ULPPK_LOG_LEVEL text_level, const char* fmtp, ...) {
	va_list	ap;
	int	prefix_len;
	char text_out[LEN_ULPPK_LOG];
	int cd_result;
	int syslog_pri;

	get_log_vars();

	if ( text_level >= log_level ) {
		va_start(ap, fmtp);
	 	prefix_len = sprintf(text_out, "pid [%d] %s/%s/%d\t %s", getpid(), file, func, line, log_level_text[text_level]);
	 	vsnprintf(text_out + prefix_len, sizeof(text_out)-prefix_len, fmtp, ap);
		va_end(ap);

		syslog_pri = syslog_priority(text_level);
		route_log(syslog_pri, text_out);

		// Kill this process with SIGQUIT to force a core dump
		if (text_level == ULPPK_LOG_CRASH_ERROR) {

			cd_result = chdir(ulppk_corepath);
			// @TODO -- put some error handling here

	 		kill(getpid(), SIGQUIT);
	 		// abort();
		}	
		return (TRUE);
	}

	return (FALSE);
}

/**
 * Form of log_ulppk_text that takes va_list argument. Used by web service logging.
 * @param func Name of the calling function
 * @param file File in which calling function is implemented.
 * @param line Line number at which ulppk_log was called.
 * @param text_level The logging (text) level of the log write request.
 * @param fmtp A printf style format string
 * @param ap variable argument list consistent with the fmtp format string
 * @return non-zero if text_level >= logging level (printed to log).
 */
int ulppk_vlog(const char* func, const char* file, int line, ULPPK_LOG_LEVEL text_level, const char* fmtp, va_list ap) {
	int	prefix_len;
	char text_out[LEN_ULPPK_LOG];
	int syslog_pri;

	get_log_vars();
	if ( text_level >= log_level ) {
	 	prefix_len = sprintf(text_out, "pid [%d] %s/%s/%d\t %s", getpid(), file, func, line, log_level_text[text_level]);
	 	vsnprintf(text_out + prefix_len, sizeof(text_out)-prefix_len, fmtp, ap);

		syslog_pri = syslog_priority(text_level);
		route_log(syslog_pri, text_out);
		return (TRUE);
	}

// Kill this process with SIGQUIT to force a core dump
	if (text_level == ULPPK_LOG_CRASH_ERROR) {
		kill(getpid(), SIGQUIT);
	}
	return (FALSE);

}


/**
 * Logs the start of an application.
 *
 * @param argc Argument count as passed to main()
 * @param argv Argument array as passed to main()
 * @return Always returns 0.
 */
int log_app_start(int argc, char* argv[]) {
	int idx = 0;
	char out_str[LEN_ULPPK_LOG];
	char* out_ptr;
	int out_len = 0;

	out_ptr = out_str;
	out_len = sprintf(out_ptr, "%s(", argv[0]);
	out_ptr += out_len;

	for (idx = 1; idx < argc; idx++) {
		if (idx == 1) {
			out_len = sprintf(out_ptr, "%s", argv[idx]);
		}else{
			out_len = sprintf(out_ptr, ", %s", argv[idx]);
		}
		out_ptr += out_len;
	}

	sprintf(out_ptr, ")");

	ULPPK_LOG(ULPPK_LOG_INFO, "%s", out_str);

	return 0;
}

