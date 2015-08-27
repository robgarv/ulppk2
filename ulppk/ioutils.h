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
#ifndef _IOUTILS_H
#define _IOUTILS_H

/**
 * @file ioutils.h
 *
 * @brief Simple low level (file descriptor) i/o utilities.
 */
#include <stdio.h>
#include <unistd.h>

size_t ioutils_writen(int fd, const void *vptr, size_t n);
size_t ioutils_readn(int fd, void *vptr, size_t n);
size_t ioutils_writef(int fd, const char* fmtp, ...);
size_t ioutils_readln(int fd, char* buff, size_t buffsize);

size_t ioutils_file_size(char* filepath);
int ioutils_is_directory(FILE* f_log, const char* path);
int ioutils_is_regular_file(FILE* f_log, const char* file);
char* ioutils_makefullpath(char* buff, const char* dirpath, const char* fn);
int ioutils_remove_file(char* filepath);
#endif

