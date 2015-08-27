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
/**
 * @file trap.c
 *
 *      Debug support tool. Basically allows you to attach to a running process.
 *
 *      The trapfile contains name value pairs. A value of on turns on a trap ... a value of
 *      off turns off a trap. Text following a '#' is ignored.
 *
 *      condition1=on	# condition 1 trap is on
 *      condition2=foo	# condition 2 trap is off
 *      condition3=off	# condition 3 trap if off
 *
 *      A trap state is checked by passing the trap name (like "condition1") to the trap_test
 *      function or the trap_attach_debugger function.
 *
 *      The file is re-read each time trap_test is called, so the trap file can
 *      be edited on the fly.
 *
 *      Typical use:
 *
 *      Edit code to add a call to trap_attach_debugger("mycondition", "my comment")
 *      Monitor the log file with tail -f'
 *      Edit the trapfile to set mycondition=on
 *      When log file starts showing trap commentary, attach the debugger to the
 *      process identified in the log file trap output. Set a break point inside the loop.
 *      Continue execution. Let the breakpoint trigger.
 *      Edit trapfile again to set mycondition=off.
 *      Set other breakpoints after call to trap_attach_debugger as needed. Continue execution.
 *
 *      Usually, one does not deliver code with traps still present in the source.
 *      But you can ... just be careful with the trapfile.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <trap.h>
#include <ioutils.h>
#include <ulppk_log.h>
#include <sysconfig.h>

static char* trapfile = NULL;

static char* find_trap(const char* trap_name);
static int parse_trap_rec(char* trap_rec);

/**
 *
 * @brief Get the trap file name from the system initialization
 * file. The function presumes sysconfig_parse_inifile has already
 * occurred.
 *
 * The trapfile is specified in the [debug] section
 * [debug]
 * trapfile=/full/path/to/trap/file
 *
 */
char* trap_read_trapfile(INIFILE_NODE* inifilep) {
	char* filename;
	filename = sysconfig_read_inifile_string(inifilep, "loglevel", "trapfile", "NULL");
	if (strcasecmp(filename, "NULL") == 0) {
		filename = NULL;
	}
	return filename;
}

/*
 * Set the file location of the trap file.
 * @param filepath -- full path to the trap text file
 * @return -- 0 on success, non-zero on failure
 */
int trap_set_flag_file(char* filepath) {
	int retval = 0;
	if (filepath == NULL) {
		ULPPK_LOG(ULPPK_LOG_DEBUG, "No trapfile is set. Debug trapping disabled");
		return 1;
	}
	if (ioutils_is_regular_file(NULL, filepath)) {
		trapfile = filepath;
		ULPPK_LOG(ULPPK_LOG_DEBUG, "Setting trapfile to [%s]", trapfile);
	} else {
		trapfile = NULL;
		retval = 1;
		ULPPK_LOG(ULPPK_LOG_DEBUG, "Specified trapfile [%s] not accessible.", filepath);
	}
	return retval;
}

/**
 * @brief Return true if the named flag in the trap settings file
 * is on.
 * @param trap_name -- name of the trap flah
 * @return 1 if the trap is on, 0 if not found or not on
 */
int trap_test(const char* trap_name) {
	char* trap_rec;
	int flag = 0;

	trap_rec = find_trap(trap_name);
	if (trap_rec != NULL) {
		flag = parse_trap_rec(trap_rec);
		if (flag) {
			ULPPK_LOG(ULPPK_LOG_DEBUG, "TRAP [%s] is SET.", trap_name);
		} else {
			ULPPK_LOG(ULPPK_LOG_DEBUG, "TRAP [%s] is NOT SET.", trap_name);
		}
	} else {
		ULPPK_LOG(ULPPK_LOG_DEBUG, "TRAP [%s] is NOT SET (no file).", trap_name);
	}
	return flag;
}

/**
 * @brief Hold up processing until the developer can attach the debugger to the
 * blocked process. This function loops until the variable debugflag becomes
 * non-zero. In gdb, this can be done by attaching to the process and
 * entering the command "print debugflag=1"
 *
 * The comment provided is written to the log file along with a TRAP
 * advisoring given the pid of the blocked process.
 *
 * @param trap_name -- name of the trap. If the trap is on, the blocking
 *   loop is engaged.
 * @param comment -- comment to deposit in log file. Usually the program name.
 */
void trap_attach_debugger(const char* trap_name, const char* comment) {
	static int debugflag = 0;
	pid_t pid;
	unsigned int count = 0;

	if (trap_test(trap_name)) {
		pid = getpid();
		ULPPK_LOG(ULPPK_LOG_WARN,
				"TRAP: Debug Trap Enabled: %s", comment);
		ULPPK_LOG(ULPPK_LOG_WARN,
				"TRAP: Attach debugger to process %d and set debugflag to 1", pid);
		while (!debugflag){
			sleep(1);
			count++;
			if ((count % 15) == 0) {
				ULPPK_LOG(ULPPK_LOG_WARN,
						"TRAP: Debug Trap Enabled: %s", comment);
				ULPPK_LOG(ULPPK_LOG_WARN,
						"TRAP: Attach debugger to process %d and set debugflag to 1", pid);
			}
		}
	}
}

static char* strip_commentary(char* rec) {
	int copy = 1;
	int i;

	for (i = 0; i < strlen(rec); i++) {
		if (rec[i] == '#') {
			copy = 0;
		}
		if (!copy) {
			rec[i] = 0;
		}
	}
	return rec;
}

static char* find_trap(const char* trap_name) {
	FILE* f;
	char* rec;
	static char recbuff[128];

	if (NULL == trapfile) {
		return NULL;
	}
	f = fopen(trapfile, "r");
	if (NULL == f) {
		ULPPK_LOG(ULPPK_LOG_ERROR, "Cannot open tranpfile for reading: [%s], errno [%d]/[%s]",
				trapfile, errno, strerror(errno));
		return NULL;
	}
	rec = fgets(recbuff, sizeof(recbuff), f);
	while (NULL != rec) {
		strip_commentary(rec);
		if (strstr(rec, trap_name) != NULL) {
			fclose(f);
			return rec;
		}
		rec = fgets(recbuff, sizeof(recbuff), f);
	}
	fclose(f);
	return NULL;
}
static int parse_trap_rec(char* trap_rec) {
	char delim[] = {' ', '\t', '=', '\n', '\0'};
	char* tokenp;
	int flag = 0;

	tokenp = strtok(trap_rec, delim);
	if (tokenp != NULL) {
		// Found the trap name of interest
		tokenp = strtok(NULL, delim);
		if ((tokenp != NULL) && (strcasecmp(tokenp, "on") == 0)) {
			flag = 1;
		}
	}
	return flag;
}
