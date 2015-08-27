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
 * @file sysconfig.c
 *
 * @brief System configuration support.
 *
 * sysconfig provides a facility for management of environment variables,
 * parsing of system initialization files (.ini files), establishing
 * logging, etc.
 *
 */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include <appenv.h>
#include <diagnostics.h>
#include <mmdeque.h>
#include <mmpool.h>
#include <mmfor.h>
#include <sysconfig.h>
#include <ulppk_log.h>
#include <pathinfo.h>

static int env_registered = 0;
static int logging_set = 0;

/**
 * Registers environment variables necessary for ULPPK components. Establishes
 * default values. In most cases, you won't want to let it go at that. Either
 * establish these through the shell, or override with .ini files values by defining
 * them in the [environment] section of your .ini files.
 *
 * These include:
 * <ul>
 * <li>SYSCONFIG_APPNAME -- Name of the application</li>
 * <li>SYSCONFIG_CORE_DIR -- Directory into which to write core files</li>
 * <li>SYSCONFIG_DATA_DIR -- Data directory</li>
 * <li>SYSCONFIG_ETC -- etc directory (location of ini files and misc configuration data)</li>
 * <li>SYSCONFIG_INI_FILE_NAME -- Name of the ini file</li>
 * <li>MMDQ_DIR_PATH -- Directory for storing application's memory mapped deques</li>
 * <li>MMPOOL_ENV_DATA_DIR -- Directory for storing application's memory mapped buffer pools</li>
 * </ul>
 *
 */
void sysconfig_register_env() {
	appenv_register_env_var(SYSCONFIG_APPNAME, DEFAULT_APPNAME);
	appenv_register_env_var(SYSCONFIG_CORE_DIR, DEFAULT_CORE_DIR);
	appenv_register_env_var(SYSCONFIG_DATA_DIR, DEFAULT_DATA_DIR);
	appenv_register_env_var(SYSCONFIG_ETC,DEFAULT_SYSCONFIG_ETC);
	appenv_register_env_var(SYSCONFIG_INI_FILE_NAME, DEFAULT_SYSCONFIG_INIFILE_NAME);
	appenv_register_env_var(MMDQ_DIR_PATH, DEFAULT_MMQDIR_DIR);
	appenv_register_env_var(MMPOOL_ENV_DATA_DIR, DEFAULT_MMPOOL_DATA);
}

/**
 * Compose an inifile name from the base name of the program.
 * @param Base name string (.e.g "myapp")
 * @return ini file name (e.g. "myapp.ini")
 */
static char * inifilename(const char* basename) {
	char* buff;

	buff = (char*)calloc(strlen(basename) + strlen(".ini") + 2, sizeof(char));
	strcpy(buff, basename);
	strcat(buff, ".ini");
	return buff;
}

/**
 * Search for the application's settings (.ini) file according to the following
 * conventions:
 *
 * 1) Search for <appname>.ini in the search directory list
 * 2) Check environment variables for an alternative ini file name
 * 		and search directory list.
 *
 * 	Thus the inifile name can be be specified through the call to sysconfig_parse_inifile
 * 	or by using the environemnt variable SYSCONFIG_INI_FILE_NAME and/or SYSCONFIG_ETC.
 */
static char* find_inifile(const char* appname, const char* etcdir) {
	char* fpath;
	char* fname;
	char* searchdirs[] = {
			".",		// current working directory
			".",		// this slot will be overwritten with SYSCONFIG_ETC
			"/usr/local/etc",
			"/etc",
			NULL,
			NULL
	};
	char* dirp;
	struct stat statbuff;
	int isfile;
	int ix;

	if ((appname != NULL) && (strlen(appname) > 0)) {
		// Read environment variable SYSCONFIG_ETC and add to the list of directories
		// to search (at the top).
		searchdirs[1] = (char*)etcdir;		// write etcdir to search list index 1
		fname = inifilename(appname);
		dirp = searchdirs[0];
		ix = 0;
		while (dirp != NULL) {
			fpath = pathinfo_append2path(dirp, fname);
			isfile = pathinfo_is_file(fpath);
			if (isfile) {
				return fpath;
			}
			free(fpath);
			ix++;
			dirp = searchdirs[ix];
		}
		free(fname);
	}

	// If that didn't work, try SYSCONFIG_INI_FILE
	fname = appenv_read_env_var(SYSCONFIG_INI_FILE_NAME);
	if (NULL == fname) {
		syslog(LOG_ERR | LOG_LOCAL0, "Environment variable %s not established ... fatal error", SYSCONFIG_INI_FILE_NAME);
		kill(getpid(), SIGQUIT);
	}
	dirp = searchdirs[0];
	ix = 0;
	while (dirp != NULL) {
		fpath = pathinfo_append2path(dirp, fname);
		isfile = pathinfo_is_file(fpath);
		if (isfile) {
			return fpath;
		}
		free(fpath);
		ix++;
		dirp = searchdirs[ix];
	}
	// No ini file found ... return NULL
	return NULL;
}

/**
 * Set up system logging.
 *
 * log_dest specifies the log destination and
 * is one of the following values:
 * <ul>
 *	<li>ULPPK_LOGMODE_CONSOLE 1</li>
 *	<li>ULPPK_LOGMODE_SYSLOG 2</li>
 * 	<li>ULPPK_LOGMODE_ALL 3</li>
 * </ul>
 *
 * @param log_dest (see ulppk_log.h for details)
 * @param appname Name of the application (or arbitrary identification string)
 * @param options -- See syslog(3). Meaningless if log_dest is	ULPPK_LOGDEST_CONSOLE
 * @param facility -- See syslog(3). Meaningless if log_dest is	ULPPK_LOGDEST_CONSOLE
 *
 */
int sysconfig_set_logging(int log_dest, const char* appname, int options, int facility) {
	logging_set = 1;
	return ulppk_log_set_logconfig(log_dest, appname, options, facility);
}

/**
 * Parse the ini file. The function presumes the following environment variables have been set:
 * SYSCONFIG_ETC -- path to configuration settings directory
 * SYSCONFIG_INI_FILE_NAME -- ini file name
 *
 *
 * appname is just the application name. If null uses value in environment
 * variable SYSCONFIG_APPNAME.
 *
 * appname is used to form a ini file name that overrides the SYSCONFIG_INI_FILE_NAME
 * setting, if the file exists. The ini file in this case is named [appname].ini If that
 * file is not found, then the value of SYSCONFIG_INI_FILE_NAME is used.
 *
 * @param appname -- application name
 * @return pointer to root inifile node on succes, NULL on failure
 */
INIFILE_NODE* sysconfig_parse_inifile(const char* appname) {
	INIFILE_NODE* inifilep;
	INIFILE_NODE* varp;

	char* inifile_name;
	char* etcdir_path;
	char* inifilepath;
	int pathlen;
	char* fmtp;
	int retstat;
	struct stat statbuff;
	char* ld_lib_path = NULL;

	FILE* fp;

	// Obtain the application name
	if (NULL == appname) {
		appname = getenv("SYSCONFIG_APPNAME");
	}
	if ((NULL == appname) || (strlen(appname) == 0)) {
		appname = DEFAULT_APPNAME;
	}

	if (!logging_set) {
		// Logging not previously set up by a call to sysconfig_set_logging
		// so setup logging with defaults.
		//
		// Open the system log. Note that we cannot use ULPPK_LOG or ULPPK_CRASH
		// until the .INI file has been successfully parsed. Nor is it
		// safe to log to stderr (we may not have a console at this point!)
		openlog(appname, LOG_PID, LOG_LOCAL0);
	}

	if (env_registered) {
		return if_get_root();
	}
	env_registered = 1;
	sysconfig_register_env();

	// Set/force SYSCONFIG_APPNAME to the value we determined  above.
	appenv_set_env_var(SYSCONFIG_APPNAME, appname);

	etcdir_path = appenv_read_env_var(SYSCONFIG_ETC);
	if (NULL == etcdir_path) {
		syslog(LOG_ERR, "Environment variable %s not established ... fatal error", SYSCONFIG_ETC);
		kill(getpid(), SIGQUIT);
	}
	inifile_name = appenv_read_env_var(SYSCONFIG_INI_FILE_NAME);
	if (NULL == inifile_name) {
		syslog(LOG_ERR, "Environment variable %s not established ... fatal error", SYSCONFIG_INI_FILE_NAME);
		kill(getpid(), SIGQUIT);
	}

	pathlen = strlen(etcdir_path);
	inifilepath = find_inifile(appname, etcdir_path);
	if (inifilepath == NULL) {
		char* label;

		if ((appname == NULL) || (strlen(appname) ==0)) {
			syslog(LOG_ERR, "Unable to access inifile  %s", SYSCONFIG_INI_FILE_NAME);
		} else {
			syslog(LOG_ERR, "Unable to access inifile %s.ini or %s", appname, SYSCONFIG_INI_FILE_NAME);
		}
		kill(getpid(), SIGQUIT);
	}
	fp = fopen(inifilepath, "r");
	if (NULL == fp) {
		syslog(LOG_ERR, "Cannot open inifile %s", inifilepath);
		kill(getpid(), SIGQUIT);
	}
	if(!if_parse_inifile(fp, inifilepath)) {
		ULPPK_LOG(ULPPK_LOG_DEBUG, "Parsed inifile %s", inifilepath);
	} else {
		syslog(LOG_ERR, "Parse error: Could not parse inifile path %s", inifilepath);
		kill(getpid(), SIGQUIT);
	}

	// OK to use ULPPK_LOG and ULPPK_CRASH from here forward ...
	if (!logging_set) {
		// Close syslog and call sysconfig_set_logging

	}
	inifilep = if_get_root();
	if (NULL == inifilep) {
		ULPPK_CRASH( "if_get_root returned NULL after parse of inifile %s", inifilepath);
	}

	// Now set ulppk memory mapped environment using .ini parameters, if provided.
	// (.ini settings have precedence over environment settings)
	// Set the buffer pool and deque data directories to the ulppk_data
	// directory.

	varp = if_get_element(inifilep, "environment", "data_dir");
	if (NULL == varp) {
		if (appenv_read_env_var(SYSCONFIG_DATA_DIR) == NULL) {
			ULPPK_LOG(ULPPK_LOG_WARN,  "Inifile: %s Unable to access environment section, data_dir element", inifilepath);
		}
	} else {
		appenv_set_env_var(MMDQ_DIR_PATH, varp->element.szvalue);
	}
	varp = if_get_element(inifilep, "environment", "log_dir");
	if (NULL == varp) {
		if (appenv_read_env_var(SYSCONFIG_LOG_DIR) == NULL) {
			ULPPK_LOG(ULPPK_LOG_WARN,  "Inifile: %s Unable to access environment section, log_dir element", inifilepath);
		}
	} else {
		appenv_set_env_var(SYSCONFIG_LOG_DIR, varp->element.szvalue);
	}

	varp = if_get_element(inifilep, "environment", "core_dir");
	if (NULL == varp) {
		if (appenv_read_env_var(SYSCONFIG_CORE_DIR) == NULL) {
			ULPPK_LOG(ULPPK_LOG_WARN,  "Inifile: %s Unable to access environment section, core_dir element", inifilepath);
		}
	} else {
		appenv_set_env_var(SYSCONFIG_CORE_DIR, varp->element.szvalue);
	}

	varp = if_get_element(inifilep, "environment", "mmdq_dir");
	if (NULL == varp) {
		if (appenv_read_env_var(MMDQ_DIR_PATH) == NULL) {
			ULPPK_LOG(ULPPK_LOG_WARN,  "Inifile: %s Unable to access environment section, mmdq_dir element", inifilepath);
		}
	} else {
		appenv_set_env_var(MMDQ_DIR_PATH, varp->element.szvalue);
	}
	varp = if_get_element(inifilep, "environment", "mmpool_env_data_dir");
	if (NULL == varp) {
		if (appenv_read_env_var(MMPOOL_ENV_DATA_DIR) == NULL) {
			ULPPK_LOG(ULPPK_LOG_WARN, "Inifile: %s Unable to access environment section, mmpool_env_data_dir element", inifilepath);
		}
	} else {
		appenv_set_env_var(MMPOOL_ENV_DATA_DIR, varp->element.szvalue);
	}

	// Set LD_LIBRARY_PATH environment variable if provided. This is an override
	// to the calling shell's setting ... not an append
	varp = if_get_element(inifilep, "environment", "ld_library_path");
	if (NULL != varp) {
		char* env_varp;

		// Override is provided in the .ini file
		appenv_set_env_var("LD_LIBRARY_PATH", varp->element.szvalue);
		env_varp = getenv("LD_LIBRARY_PATH");
		if (NULL == env_varp) {
			ULPPK_LOG(ULPPK_LOG_ERROR, "Attempt to override LD_LIBRARY_PATH fails: %s",
					varp->element.szvalue);
		} else {
			ULPPK_LOG(ULPPK_LOG_INFO, "LD_LIBRARY_PATH override: %s",
					varp->element.szvalue);
		}
	} else {
		char* env_varp;

		env_varp = getenv("LD_LIBRARY_PATH");
		if (NULL == env_varp) {
			ULPPK_LOG(ULPPK_LOG_INFO, "No LD_LIBRARY_PATH override. LD_LIBRARY_PATH not set");
		} else {
			ULPPK_LOG(ULPPK_LOG_INFO, "Using inherited LD_LIBRARY_PATH: %s", env_varp);
		}
	}
	if (inifilepath != NULL) {
		free(inifilepath);
	}
	return inifilep;
}

/**
 * Read an integer variable from the .INI file. A default value is provided by the caller
 * to enforce if the ini file variable is not found of is incorrectly formed. Errors
 * accessing the ini file are logged as warnings.
 *
 * @param inifilep -- ptr to root node of inifile parse tree
 * @param section -- section name (without brackets, so "foo" not "[foo]")
 * @param name -- name of the variable
 * @param defaultval -- the default value to return if error reading ini file variable
 * @return Intger value of named variable.
 */
int sysconfig_read_inifile_int(INIFILE_NODE* inifilep, const char* section, const char* name, int defaultval) {
	long lval = 0;
	INIFILE_NODE* inivarp;

	inivarp = if_get_element(inifilep, section, name);
	if (NULL == inivarp) {
		ULPPK_LOG(ULPPK_LOG_WARN, "Inifile variable [%s] %s cannot be read! Enforcing default.", section, name);
		return defaultval;
	}
	if (if_get_intval(inivarp, &lval)) {
		ULPPK_LOG(ULPPK_LOG_WARN, "Inifile variable [%s] %s is not a value elememt! Fix the .ini file! Enforcing default.",
				section, name);
		return defaultval;
	}
	return (int)lval;
}

/**
 * Read a string variable from the .INI file. A default value is provided by the caller
 * to enforce if the ini file variable is not found of is incorrectly formed. Errors
 * accessing the ini file are logged as warnings. All returned strings are allocated from
 * the heap and must be subsequently freed when the calling process no longer requires them.
 *
 * @param inifilep -- ptr to root node of inifile parse tree
 * @param section -- section name (without brackets, so "foo" not "[foo]")
 * @param name -- name of the variable
 * @param defaultval -- the default value to return if error reading ini file variable
 * @return String value of named variable
 */
char* sysconfig_read_inifile_string(INIFILE_NODE* inifilep, const char* section, const char* name, const char* defaultval) {
	char* sval = NULL;
	INIFILE_NODE* inivarp;

	inivarp = if_get_element(inifilep, section, name);
	if (NULL == inivarp) {
		ULPPK_LOG(ULPPK_LOG_WARN, "Inifile variable [%s] %s cannot be read! Enforcing default.", section, name);
		return strdup(defaultval);
	}
	if (if_get_strval(inivarp, &sval)) {
		ULPPK_LOG(ULPPK_LOG_WARN, "Inifile variable [%s] %s is not a value elememt! Fix the .ini file! Enforcing default.",
				section, name);
		return strdup(defaultval);
	}
	return strdup(sval);
}
/**
 * Set user and group ID to for the calling process.
 * Calling parameter user specifies the user to set
 * to. Note that group ID is obtained by accessing password
 * data.
 * @param user User name
 * @return -- 0 on success, non-zero on error.
 */
int sysconfig_set_user(char* user) {
	struct passwd* passwdp;
	struct group* group_p;
	char* group;
	uid_t uid;
	gid_t gid;
	int retval = 0;

	// Get the passwd structure for user

	errno = 0;
	passwdp = getpwnam(user);
	if (NULL == passwdp) {
		ULPPK_LOG(ULPPK_LOG_ERROR,
				"Unable to access user %s in /etc/passwd! Is %s user defined?", user, user);
		return 1;
	}
	// Got the passwd struct for given user. Get the user ID and group ID
	uid = passwdp->pw_uid;
	gid = passwdp->pw_gid;
	group_p = getgrgid(gid);
	if (NULL == group_p) {
		ULPPK_LOG(ULPPK_LOG_ERROR,
				"Unable to access group ID  %d in /etc/group! Is group %gid defined?", gid, gid);
		return 1;
	}
	group = group_p->gr_name;
	ULPPK_LOG(ULPPK_LOG_DEBUG, "User %s uid [%d]  Group %s gid [%d]", user, group, uid, gid);

	// Now do a setgid and setuid. The order of these operations is important.
	// If we change user ID first, we won't have permission to change group ID ...
	if (setgid(gid)) {
		ULPPK_LOG(ULPPK_LOG_ERROR,
				"Unable to set group ID to $group/%d: errno [%d]/[%s]", group, gid,
				errno, strerror(errno));
		return 1;
	}
	ULPPK_LOG(ULPPK_LOG_DEBUG, "Changed group ID to %s/[%d]", group, gid);
	if (setuid(uid)) {
		ULPPK_LOG(ULPPK_LOG_ERROR,
				"Unable to set user ID to %s/%d: errno [%d]/[%s]", user, uid,
				errno, strerror(errno));
		return 1;
	}
	ULPPK_LOG(ULPPK_LOG_DEBUG, "Changed uid to %s/[%d]", user, uid);
	return 0;
}

