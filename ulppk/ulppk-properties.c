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
#include <config.h>
#include <stdio.h>

char ulppk_version_label[512];

char* ulppk_version() {
	sprintf(ulppk_version_label, "%s | %s | %s", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_STRING);

	return ulppk_version_label;
}

int ulppk_debug_trace_state() {

#ifdef ULPPK_DEBUG
	return 1;
#else
	return 0;
#endif
}
