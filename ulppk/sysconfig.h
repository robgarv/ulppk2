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


#ifndef SYSCONFIG_H_
#define SYSCONFIG_H_

/**
 * @file sysconfig.h
 *
 * @brief Definitions for sysconfig module.
 *
 * sysconfig provides a facility for management of environment variables,
 * parsing of system initialization files (.ini files), establishing
 * logging, etc.
 *
 */

#include <ifile.h>

/*
 * System configuration utility.
 * Support system initialization settings (.ini file)
 * parsing. Establishes variables necessary to:
 * 	Support logging and core dump facilities.
 * 	Support memory mapped I/O based facilities.
 *
 * 	The following environment variables should be
 * 	set prior to running an application that uses
 * 	this facility:
 *
 * 	SYSCONFIG_APPNAME -- name of the application, defaults to ulppk_app
 * 	SYSCONFIG_DATA_DIR -- path to directory containing app specific data
 * 	SYSCONFIG_ETC -- defaults to /usr/local/etc
 * 	SYSCONFIG_INI_FILE_NAME -- defaults to ulppkapp.ini
 * 	MMDQ_DIR_PATH -- Memory mapped deque directory -- defaults to /var/ulppk
 *  MMPOOL_ENV_DATA_DIR -- memory mapped buffer pool directory --- defaults to /var/ulppk
 */

/*
 * Syslog facilities used by ULPPK.
 */
typedef enum {
	LOG_APPS = 0,
	LOG_WEBAPPS
} ULPPK_LOG_FACILITIES;

#define SYSCONFIG_APPNAME "SYSCONFIG_APPNAME"
#define SYSCONFIG_CORE_DIR "SYSCONFIG_CORE_DIR"
#define SYSCONFIG_DATA_DIR "SYSCONFIG_DATA_DIR"
#define SYSCONFIG_ETC "SYSCONFIG_ETC"
#define SYSCONFIG_INI_FILE_NAME "SYSCONFIG_INI_FILE_NAME"
#define SYSCONFIG_LOG_DIR "SYSCONFIG_LOG_DIR"
#define DEFAULT_SYSCONFIG_ETC "/usr/local/etc"
#define DEFAULT_SYSCONFIG_INIFILE_NAME "ulppkapp.ini"
#define DEFAULT_DATA_DIR "/var/ulppk"
#define DEFAULT_LOG_DIR "/var/ulppk"
#define DEFAULT_CORE_DIR "/var/ulppk"
#define DEFAULT_MMQDIR_DIR "/var/ulppk"
#define DEFAULT_MMPOOL_DATA "/var/ulppk"
#define DEFAULT_APPNAME "ulppk_app"

#ifdef __cplusplus
extern "C" {
#endif

int sysconfig_set_logging(int log_dest, const char* appname, int options, int facility);
void sysconfig_register_env();
INIFILE_NODE* sysconfig_parse_inifile(const char* appname);
char* sysconfig_read_inifile_string(INIFILE_NODE* inifilep, const char* section, const char* name, const char* defaultval);
int sysconfig_read_inifile_int(INIFILE_NODE* inifilep, const char* section, const char* name, int defaultval);
int sysconfig_set_user();
int sysconfig_set_logging(int log_dest, const char* appname, int options, int facility);

#ifdef __cplusplus
}
#endif

#endif /* SYSCONFIG_H_ */
