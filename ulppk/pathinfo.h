/*
 *****************************************************************

<GPL>

Copyright: © 2001-2012 Robert C Garvey

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

/*
 * pathinfo.h
 *
 *  Created on: Dec 3, 2012
 *      Author: robgarv
 *
 */

#ifndef PATHINFO_H_
#define PATHINFO_H_

#include <sys/types.h>
#include <sys/stat.h>

typedef struct _PATHINFO_STRUCT {
	char* fullpath;		// full pathname
	char* filename;		// filename (w/extension e.g. foo.txt)
	char* basename;		// filename (w/o extension e.g foo)
	char* extension;	// extension part of file name (e.g. txt)
	char* dirpath;
	int isdir;
} PATHINFO_STRUCT;

PATHINFO_STRUCT* pathinfo_parse_filepath(const char* path);
void pathinfo_release(PATHINFO_STRUCT* pathinfop);
char* pathinfo_tostring(PATHINFO_STRUCT* pathinfop);
int pathinfo_get_mode(const char* pathname, mode_t* modep);
int pathinfo_is_dir(const char* pathname);
int pathinfo_is_file(const char* pathname);
int pathinfo_is_link(const char* pathname);
int pathinfo_is_char_special(const char* pathname);
int pathinfo_is_block_special(const char* pathname);
int pathinfo_is_pipe(const char* pathname);
int pathinfo_is_socket(const char* pathname);
char* pathinfo_append2path(const char* pelement1, const char* pelement2);

#endif /* PATHINFO_H_ */
